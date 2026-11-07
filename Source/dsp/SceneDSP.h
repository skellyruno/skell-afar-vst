#pragma once
#include "SceneModel.h"
#include "ThroatTexture.h"
#include <SkellCore/Core.h>
#include <array>
#include <vector>
#include <cmath>
#include <cstdint>

namespace afar {
inline float dbGain(float dB) noexcept { return std::pow(10.f, dB * .05f); }
inline float sanitize(float v) noexcept { return std::isfinite(v) ? std::clamp(v, -100.f, 100.f) : 0.f; }

// One fractional-delay ring per channel. prepare() is never called on the audio thread.
class DelayLine {
public:
    void prepare(std::size_t length) { data.assign(std::max<std::size_t>(4, length), 0.f); write = 0; }
    void reset() noexcept { std::fill(data.begin(), data.end(), 0.f); write = 0; }
    void push(float v) noexcept { data[write] = sanitize(v); }
    float tap(float delaySamples) const noexcept {
        if (data.empty()) return 0.f;
        const float n = static_cast<float>(data.size());
        float p = static_cast<float>(write) - std::clamp(delaySamples, 0.f, n - 3.f);
        if (p < 0.f) p += n;
        const auto i = static_cast<std::size_t>(p);
        const auto j = i+1 == data.size() ? 0 : i+1;
        const float frac = p - static_cast<float>(i);
        return data[i] * (1.f-frac) + data[j]*frac;
    }
    void advance() noexcept { if (++write == data.size()) write=0; }
private:
    std::vector<float> data;
    std::size_t write = 0;
};

struct SceneControls {
    float effort = .5f, distance = 3.f, angle = 0.f, occlusion = 0.f, throatTexture = 0.f;
    float size = 1.f, ambienceDb = -24.f, presence = 0.f, width = 1.f;
    float mix = 1.f, outputDb = 0.f, temperature = 20.f, humidity = 50.f, absorption = .7f, bedGain = 1.f;
    int scene=5, recordedAs=2, occlusionType=0, micType=7, micStereo=0;
    bool removeDelay = true, bypassEffort=false, muteAmbience=false, ambienceDuck=false, delta=false;
    std::array<float, 8> beds {};
};

struct Frame { float left=0, right=0; };
class SceneDSP {
public:
    void prepare(double sampleRate, int outputs, int quality) {
        sr = std::clamp(sampleRate, 8000.0, 384000.0);
        channels = std::clamp(outputs,1,2);
        const auto directSize = static_cast<std::size_t>(std::ceil(sr * 0.62)) + 8;
        const auto echoSize = static_cast<std::size_t>(std::ceil(sr * 3.3)) + 8;
        for (auto& l : direct) l.prepare(directSize);
        for (auto& l : reflections) l.prepare(echoSize);
        throat.prepare(sr);
        diffuse.prepare(sr);
        limiter.prepare(sr, channels, quality==0 ? skellcore::TruePeakLimiter::Quality::Economy
                                                   : skellcore::TruePeakLimiter::Quality::Standard);
        limiter.setCeilingDb(-1.f);
        controlPole=static_cast<float>(1.0-std::exp(-1.0/(sr*.0012)));
        echoPole=static_cast<float>(1.0-std::exp(-1.0/(sr*.006)));
        dripDecay=static_cast<float>(std::exp(-1.0/(sr*.040)));
        reset();
    }
    void reset() noexcept {
        for (auto& l : direct) l.reset();
        for (auto& l : reflections) l.reset();
        diffuse.reset(); limiter.reset(); throat.reset();
        sourceLow = {}; directLow = {}; micLow = {}; micHigh = {}; ambienceLow = {};
        voiceEnv=0.f; noiseState=currentSeed; sampleIndex=0;sceneMorph=1.f;
        sourceGainSmoothed=outputGainSmoothed=sourceGainTarget=outputGainTarget=1.f;
        ambientGainSmoothed=ambientGainTarget=0.f;
        cutoffAlphaSmoothed=cutoffAlphaTarget=.75f;
        micAlphaSmoothed=micAlphaTarget=.7f;
        earlyGainSmoothed=earlyGainTarget=0.f;
        echoTapSmoothed={0.f,0.f,0.f};echoTapTargets=echoTapSmoothed;
        lastDrr=lastDrrTarget=0.f;
        dripPhase=dripLevel=0.f;
    }
    int latencySamples() const noexcept { return limiter.latencySamples(); }
    void setScene(int id) noexcept {
        id=std::clamp(id,0,static_cast<int>(spaces.size())-1);
        if(id!=currentScene) { previousScene=currentScene;currentScene=id;sceneMorph=0.f; }
    }
    void setSeed(std::uint32_t seed) noexcept {
        seed=seed==0 ? 1u : seed;
        if(seed!=currentSeed) { currentSeed=seed;noiseState=seed; }
    }
    void setQuality(int q) noexcept { quality = q; }
    float intelligibility() const noexcept { return lastClarity; }
    float directReverb() const noexcept { return lastDrr; }
    float peak() const noexcept { return lastPeak; }
    void clearPeak() noexcept { lastPeak=0.f; }

    Frame processFrame(Frame input, const SceneControls& p, float wetAmount, float bypassAmount) noexcept {
        auto space=getSpace(currentScene);
        if(sceneMorph<1.f) {
            const auto& old=getSpace(previousScene);
            const float t=sceneMorph;
            const auto morph=[t](float from,float to){return from+(to-from)*t;};
            space.widthMeters=morph(old.widthMeters,space.widthMeters);
            space.lengthMeters=morph(old.lengthMeters,space.lengthMeters);
            space.heightMeters=morph(old.heightMeters,space.heightMeters);
            space.rt60Seconds=morph(old.rt60Seconds,space.rt60Seconds);
            space.reflectionGain=morph(old.reflectionGain,space.reflectionGain);
            space.diffuseGain=morph(old.diffuseGain,space.diffuseGain);
            space.upperRollOffHz=morph(old.upperRollOffHz,space.upperRollOffHz);
            space.firstEchoMeters=morph(old.firstEchoMeters,space.firstEchoMeters);
            sceneMorph=std::min(1.f,sceneMorph+1.f/static_cast<float>(sr*.3));
        }
        throat.setAmount(p.throatTexture);
        const auto textured = throat.process(input.left,input.right);
        const float source = .5f*(sanitize(textured.left)+sanitize(textured.right));
        const float recorded = recordedEffort(p.recordedAs);
        const float difference = std::clamp(p.effort - recorded, -1.f, 1.f);
        const float gainDb = std::clamp((effortSpl(p.effort) - effortSpl(recorded)) * .42f, -16.f, 16.f);
        // Update expensive scene/acoustic coefficients at control rate. A small
        // one-pole smoothing stage below prevents zipper noise at the 32-sample
        // boundaries, even during continuous walk automation.
        const bool update=(sampleIndex % 32U)==0U;
        if(update) {
            sourceGainTarget=dbGain(gainDb);
            outputGainTarget=dbGain(p.outputDb);
            ambientGainTarget=p.muteAmbience || p.ambienceDb<=-59.9f ? 0.f
                : dbGain(p.ambienceDb)*.02f*std::clamp(p.bedGain,0.f,4.f);
            const float face=std::clamp(p.angle/180.f,0.f,1.f);
            const float weather=1.f + std::clamp((20.f-p.temperature)*.006f,-.15f,.15f)
                                      + std::clamp((50.f-p.humidity)*.003f,-.15f,.15f);
            const float typeCut[] {19000.f,10500.f,3500.f,1600.f,4900.f,1100.f};
            const int occlusionType=std::clamp(p.occlusionType,0,5);
            const float obstruction=std::clamp(p.occlusion,0.f,1.f);
            const float highCut=std::min(
                std::clamp(space.upperRollOffHz*std::exp(-p.distance*.013f*p.absorption*weather),260.f,19000.f)*(1.f-face*.72f),
                19000.f+(typeCut[occlusionType]-19000.f)*obstruction);
            cutoffAlphaTarget=std::clamp(1.f-std::exp(-2.f*pi*highCut/static_cast<float>(sr)),0.f,1.f);
            const float micCut[] {3300.f,14500.f,10000.f,15500.f,18500.f,3900.f,2800.f,19000.f};
            micAlphaTarget=std::clamp(1.f-std::exp(-2.f*pi*micCut[std::clamp(p.micType,0,7)]/static_cast<float>(sr)),0.f,1.f);
            const float firstEcho=std::max(1.f,space.firstEchoMeters*std::clamp(p.size,.5f,2.f))*2.f/speedOfSound*static_cast<float>(sr);
            echoTapTargets[0]=std::clamp(firstEcho,1.f,static_cast<float>(sr*3.0));
            echoTapTargets[1]=std::clamp(firstEcho*1.61f+space.widthMeters*2.f/speedOfSound*static_cast<float>(sr),1.f,static_cast<float>(sr*3.1));
            echoTapTargets[2]=std::clamp(firstEcho*2.19f+space.lengthMeters/speedOfSound*static_cast<float>(sr),1.f,static_cast<float>(sr*3.2));
            const float echoDistance=space.firstEchoMeters+std::max(0.f,p.distance);
            const float echoSpl=effortSpl(p.effort)-20.f*std::log10(std::max(1.f,echoDistance));
            const float gate=std::clamp((echoSpl-24.f)/18.f,0.f,1.f);
            earlyGainTarget=space.reflectionGain*gate*std::sqrt(std::clamp(p.size,.5f,2.f));
            lastDrrTarget=directReverbDb(space,p.distance);
            // Most expensive SkellCore coefficient update: only once per 32 frames.
            diffuse.setParameters(space.rt60Seconds*p.size,space.isOpen ? .71f:.48f,1.f);
        }
        sourceGainSmoothed+=controlPole*(sourceGainTarget-sourceGainSmoothed);
        outputGainSmoothed+=controlPole*(outputGainTarget-outputGainSmoothed);
        ambientGainSmoothed+=controlPole*(ambientGainTarget-ambientGainSmoothed);
        cutoffAlphaSmoothed+=controlPole*(cutoffAlphaTarget-cutoffAlphaSmoothed);
        micAlphaSmoothed+=controlPole*(micAlphaTarget-micAlphaSmoothed);
        earlyGainSmoothed+=controlPole*(earlyGainTarget-earlyGainSmoothed);
        // Fractional reflection delays are smoothly moved instead of snapping
        // by up to a control block when SIZE/SPACE is automated.
        for(std::size_t k=0;k<echoTapSmoothed.size();++k)
            echoTapSmoothed[k]+=echoPole*(echoTapTargets[k]-echoTapSmoothed[k]);
        lastDrr+=.02f*(lastDrrTarget-lastDrr);
        voiceEnv=.997f*voiceEnv+.003f*std::abs(source);
        const float tiltAlpha = std::clamp(1500.f/static_cast<float>(sr),.001f,.7f);
        sourceLow[0] += tiltAlpha*(source-sourceLow[0]);
        float shaped=source;
        if (!p.bypassEffort) {
            if (difference < -.02f) {
                const float whisper=std::clamp(-difference*1.5f,0.f,.85f);
                const float airyNoise=nextNoise()*voiceEnv*.27f;
                shaped=(1.f-whisper*.25f)*source + whisper*(airyNoise+sourceLow[0]*.17f);
            } else if (difference > .02f) {
                const float shout=std::clamp(difference*1.45f,0.f,1.f);
                const float hp=source-sourceLow[0];
                shaped=std::tanh((source + hp*.24f*shout)*(1.f + shout*1.5f)) / (1.f+.4f*shout);
            }
        }
        shaped=sanitize(shaped*sourceGainSmoothed);
        // Directivity and occlusion: level loss + gentle low-pass. No phase-reset jumps.
        const float facing=std::clamp(p.angle/180.f,0.f,1.f);
        const float obstruction=std::clamp(p.occlusion,0.f,1.f);
        const int type=std::clamp(p.occlusionType,0,5);
        const float alpha=cutoffAlphaSmoothed;
        const float wallGain=1.f-obstruction*(type==2 ? .62f : type==3 ? .86f : .40f);
        const float directionGain=1.f-facing*.30f;
        const float directGain=geometricDirectGain(p.distance)*directionGain*wallGain;
        const float tof = p.removeDelay ? 0.f : p.distance * static_cast<float>(sr) / speedOfSound;
        // Reflections and diffuse field remain independent of the direct 1/r loss.
        float early[2] {};
        float nearSignal[2] {};
        const float in[2] {textured.left, channels == 2 ? textured.right : textured.left};
        for (int c=0;c<channels;++c) {
            const float excitation=sanitize(in[c] + (shaped-source));
            direct[c].push(excitation);
            reflections[c].push(excitation);
            float d=direct[c].tap(tof);
            directLow[c]+=alpha*(d-directLow[c]);
            d=directLow[c]*directGain;
            nearSignal[c]=sanitize(d);
            // 3 sparse scene taps. Distance of reflection objects, not the source distance.
            // Canyon tap is intentionally very late, box-room taps follow dimensions.
            early[c]=(reflections[c].tap(echoTapSmoothed[0])*.55f
                + reflections[c].tap(echoTapSmoothed[1])*.28f
                + reflections[c].tap(echoTapSmoothed[2])*.17f)*earlyGainSmoothed;
            direct[c].advance(); reflections[c].advance();
        }
        // Shared 4-line SkellCore FDN, used as the first live diffusion stage.
        const auto late = diffuse.processFrame(shaped, shaped);
        const float ambient=makeAmbience(space.ambience,p);
        const float lateGain = space.diffuseGain * (space.isOpen ? .48f: 1.f);
        const float voiceBoost=1.f+std::clamp(p.presence,0.f,1.f)*.55f;
        const float diffuseL=late[0]*lateGain;
        const float diffuseR=late[1]*lateGain;
        const float wetL=nearSignal[0]*voiceBoost+early[0]+diffuseL+ambient;
        const float wetR=channels==2 ? nearSignal[1]*voiceBoost + early[1] + diffuseR - ambient*.12f : wetL;
        // Stereo width applies to the reverberant field, not to the direct speech.
        const float mid=.5f*(wetL+wetR);
        const float patternWidth=p.micStereo==0 ? 0.f : p.micStereo==2 ? 1.3f : p.micStereo==3 ? 1.6f : 1.f;
        const float side=.5f*(wetL-wetR)*std::clamp(p.width*patternWidth,0.f,2.f);
        float output[2] {mid+side,mid-side};
        for (int c=0;c<channels;++c) {
            // Device coloration as one-pole response and input-stage soft clip.
            const int mic=std::clamp(p.micType,0,7);
            micLow[c] += micAlphaSmoothed*(output[c]-micLow[c]);
            output[c]=micLow[c];
            if (mic==0 || mic==5 || mic==6) output[c]=std::tanh(output[c]*1.15f);
            const float clean=c==0 ? input.left:input.right;
            // Blend inside the effect, and ramp to latency-matched original on bypass.
            const float active=clean+(output[c]-clean)*std::clamp(wetAmount,0.f,1.f);
            output[c]=active+(clean-active)*std::clamp(bypassAmount,0.f,1.f);
            if(p.delta) output[c]-=clean;
            output[c]=sanitize(output[c]*outputGainSmoothed);
        }
        float limited[2] {};
        limiter.processFrame(output,limited,channels);
        lastPeak=std::max(lastPeak,std::max(std::abs(limited[0]),std::abs(limited[channels-1])));
        const float clear=std::clamp((lastDrr+22.f)/45.f - std::abs(ambient)*3.f, 0.f,1.f);
        lastClarity=lastClarity*.995f+clear*.005f;
        ++sampleIndex;
        return {limited[0],channels==2 ? limited[1]:limited[0]};
    }
private:
    float nextNoise() noexcept {
        noiseState ^= noiseState<<13; noiseState ^= noiseState>>17; noiseState ^= noiseState<<5;
        return static_cast<float>((noiseState>>8) * (2.0/16777216.0) - 1.0);
    }
    float makeAmbience(AmbienceKind kind,const SceneControls& p) noexcept {
        // Even when muted keep RNG advancing: repeatable renders and a smooth
        // AMBIENCE/MUTE ramp from the cached scene-gain coefficient.
        float n=nextNoise();
        const float coeff=kind==AmbienceKind::Water ? .10f : kind==AmbienceKind::Room ? .015f : .004f;
        ambienceLow[0] += coeff*(n-ambienceLow[0]);
        const float wind=ambienceLow[0]*8.f;
        const float hiss=n*.35f;
        float bed=0.f;
        switch(kind) {
            case AmbienceKind::Water: {
                // Short procedural drip transients; no samples or IRs required.
                // Trigger by deterministic PRNG bits (roughly 1.5/s at 48 kHz),
                // then ring a decaying resonator with a bounded tail.
                if((noiseState & 0x7fffu)==0 && dripLevel<.004f) {
                    dripLevel=.22f;dripPhase=0.f;
                    dripHz=(noiseState & 0x10000u) ? 1517.f:1129.f;
                }
                float drip=0.f;
                if(dripLevel>1.e-5f) {
                    dripPhase+=2.f*pi*dripHz/static_cast<float>(sr);
                    if(dripPhase>2.f*pi)dripPhase-=2.f*pi;
                    drip=std::sin(dripPhase)*dripLevel;
                    dripLevel*=dripDecay;
                }
                bed=wind*.7f+hiss*.1f+drip;
                break;
            }
            case AmbienceKind::Forest: bed=wind*.45f+hiss*.08f; break;
            case AmbienceKind::Insects: bed=hiss*.1f+std::sin(2.f*pi*720.f*static_cast<float>(sampleIndex%static_cast<uint64_t>(sr))/static_cast<float>(sr))*.01f; break;
            case AmbienceKind::Traffic: bed=wind*.95f; break;
            case AmbienceKind::Drone: bed=wind*.25f; break;
            case AmbienceKind::Room: bed=hiss*.08f+wind*.13f; break;
            case AmbienceKind::Wind: bed=wind*.7f; break;
        }
        const float gain=ambientGainSmoothed;
        const float duck=p.ambienceDuck ? 1.f-.45f*std::clamp(voiceEnv*6.f,0.f,1.f) : 1.f;
        return sanitize(bed*gain*duck);
    }
    double sr=48000.; int channels=2, currentScene=5, quality=1;
    std::array<DelayLine,2> direct, reflections;
    ThroatTexture throat;
    skellcore::FDNReverb diffuse;
    skellcore::TruePeakLimiter limiter;
    std::array<float,2> sourceLow {}, directLow {}, micLow {}, micHigh {}, ambienceLow {};
    std::uint32_t noiseState=0x98712341u,currentSeed=0x98712341u;
    std::uint64_t sampleIndex=0;
    int previousScene=5;float sceneMorph=1.f;
    float voiceEnv=0.f, lastPeak=0.f, lastDrr=0.f, lastClarity=1.f;
    float sourceGainTarget=1.f,sourceGainSmoothed=1.f;
    float outputGainTarget=1.f,outputGainSmoothed=1.f;
    float ambientGainTarget=0.f,ambientGainSmoothed=0.f;
    float cutoffAlphaTarget=.75f,cutoffAlphaSmoothed=.75f;
    float micAlphaTarget=.7f,micAlphaSmoothed=.7f;
    float earlyGainTarget=0.f,earlyGainSmoothed=0.f,lastDrrTarget=0.f;
    std::array<float,3> echoTapTargets{},echoTapSmoothed{};
    float controlPole=.017f,echoPole=.003f,dripDecay=.9995f;
    float dripPhase=0.f,dripLevel=0.f,dripHz=1200.f;
};
} // namespace afar
