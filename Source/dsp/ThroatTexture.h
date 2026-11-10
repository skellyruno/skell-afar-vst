#pragma once
// Skell-Afar source-domain throat colour (C++17, no JUCE dependency).
// A controlled approximation of breath/creak/grit, NOT true vocal-tract resynthesis.
// One automatable macro: CLEAN [0] -> AIRY [1/3] -> RASPY [2/3] -> GRITTY [1].
// Filters/gates act on voice-derived bands, not on the entire wet scene output.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace afar {
class ThroatTexture {
public:
    struct Stereo { float left, right; };
    // First-order antiderivative anti-aliased tanh (Parker/Bilbao ADAA): same curve, far less
    // aliasing than the plain per-sample tanh, with no latency and no oversampling buffers.
    struct AdaaTanh {
        static float logCosh(float x) noexcept {
            const float a=std::abs(x);
            return a+std::log1p(std::exp(-2.f*a))-0.69314718f;
        }
        float process(float x) noexcept {
            float y;
            if(!valid) y=std::tanh(x);
            else {
                const float dx=x-prevX,f=logCosh(x);
                y=std::abs(dx)<1.0e-4f ? std::tanh(.5f*(x+prevX)) : (f-prevF)/dx;
            }
            prevX=x;prevF=logCosh(x);valid=true;
            return y;
        }
        void invalidate() noexcept { valid=false; }
        float prevX=0.f,prevF=0.f;bool valid=false;
    };
    void prepare(double rate) noexcept {
        const auto sr = std::clamp(std::isfinite(rate) ? rate : 48000.0, 8000.0, 384000.0);
        envelopeAttack = std::exp(-1.0 / (sr * .004));
        envelopeRelease = std::exp(-1.0 / (sr * .085));
        lowPole = static_cast<float>(1.0-std::exp(-2.0 * 3.141592653589793 * 240.0 / sr));
        highPole = static_cast<float>(1.0-std::exp(-2.0 * 3.141592653589793 * 3100.0 / sr));
        texturePole = static_cast<float>(1.0-std::exp(-2.0 * 3.141592653589793 * 36.0 / sr));
        raspPole = static_cast<float>(1.0-std::exp(-2.0 * 3.141592653589793 * std::min(7800.0,sr*.20) / sr));
        fryInc = static_cast<float>(52.0/sr);
        jitterPole = static_cast<float>(1.0-std::exp(-2.0 * 3.141592653589793 * 7.0 / sr));
        sibilancePole = static_cast<float>(1.0-std::exp(-1.0/(sr*.008)));
        macroPole = static_cast<float>(1.0-std::exp(-1.0 / (sr * .025)));
        // Breath noise is band-shaped (aspiration band ~0.7-5.5 kHz) instead of raw white
        // noise, which put ~79% of the added energy above 5 kHz as harsh hiss.
        noiseHpPole = static_cast<float>(1.0-std::exp(-2.0 * 3.141592653589793 * 700.0 / sr));
        noiseLpPole = static_cast<float>(1.0-std::exp(-2.0 * 3.141592653589793 * std::min(5500.0,sr*.22) / sr));
        noiseGain = static_cast<float>(std::clamp(std::sqrt(.5*sr/4800.0)*.8,1.0,3.0));
        reset();
    }
    void reset() noexcept { state = {}; macro = target = 0.f; rng = 0x42fe209bu; nLow = nLp1 = nLp2 = 0.f; fryPhase = jitter = 0.f; fryMod = 1.f; }
    void setAmount(float amount) noexcept {
        target = std::clamp(std::isfinite(amount) ? amount : 0.f,0.f,1.f);
    }
    Stereo process(float l,float r) noexcept {
        l = std::isfinite(l) ? l : 0.f;
        r = std::isfinite(r) ? r : 0.f;
        macro += macroPole * (target - macro);
        if (macro <= 1.0e-6f && target == 0.f) return {l,r};
        // Constant-CPU deterministic linked noise avoids decorrelated stereo hiss.
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        const float white = (static_cast<float>(rng >> 8) * (1.f / 8388608.f)) - 1.f;
        nLow += noiseHpPole * (white - nLow);
        nLp1 += noiseLpPole * ((white - nLow) - nLp1);
        nLp2 += noiseLpPole * (nLp1 - nLp2);
        const float noise = nLp2 * noiseGain;
        // Voice-locked 'fry': a jittered ~52 Hz glottal-pulse-shaped modulation of the harmonic
        // drive only (the dry fundamental is never modulated). Mean gain is 1.
        jitter += jitterPole * (white - jitter);
        fryPhase += fryInc * (1.f + .35f * jitter);
        fryPhase -= std::floor(fryPhase);
        const float tri = 1.f - std::abs(2.f * fryPhase - 1.f);
        fryMod = (tri * tri - (1.f / 3.f)) * 1.5f;   // zero-mean pulse train, ~[-.5, +1]
        const auto weights = morphWeights(macro);
        return {processOne(l,state[0],noise,weights),processOne(r,state[1],noise,weights)};
    }
    static std::array<float,4> morphWeights(float amount) noexcept {
        const float v = std::clamp(std::isfinite(amount) ? amount : 0.f,0.f,1.f)*3.f;
        const int section = std::min(2,static_cast<int>(v));
        const float position = std::clamp(v-static_cast<float>(section),0.f,1.f);
        // C1-smooth morph boundaries: eliminates a sudden change in the slope
        // when the user automates across AIRY and RASPY landmarks.
        const float fraction = position*position*(3.f-2.f*position);
        // Equal power is inappropriate for strongly correlated morphs; linear weights
        // ensure continuity, no extra level spike, and exact CLEAN at zero.
        std::array<float,4> w {};
        w[static_cast<std::size_t>(section)] = 1.f-fraction;
        w[static_cast<std::size_t>(section+1)] = fraction;
        return w;
    }
private:
    struct Channel { float env=0.f, low=0.f, smoothedHigh=0.f, creak=0.f;
        float raspFiltered=0.f, gritFiltered=0.f, highEnergy=0.f, midEnergy=0.f;
        AdaaTanh raspSat, gritSat; };
    float processOne(float x, Channel& c, float noise,const std::array<float,4>& w) noexcept {
        x=std::clamp(x,-8.f,8.f);
        const float amplitude=std::abs(x);
        const float envPole=amplitude>c.env?static_cast<float>(envelopeAttack):static_cast<float>(envelopeRelease);
        c.env=envPole*c.env+(1.f-envPole)*amplitude;
        // Voice envelope gate is nearly zero on silence, softly increasing with
        // audible speech. Noise never runs at a fixed ambient level.
        const float gate=std::clamp((c.env-.0012f)*42.f,0.f,1.f);
        c.low += lowPole*(x-c.low);
        c.smoothedHigh += highPole*(x-c.smoothedHigh);
        const float mid=c.smoothedHigh-c.low;
        const float upper=x-c.smoothedHigh;
        c.highEnergy+=sibilancePole*(std::abs(upper)-c.highEnergy);
        c.midEnergy+=sibilancePole*(std::abs(mid)-c.midEnergy);
        // Preserve fricatives/transients: keep vocal texture on the body band
        // but back it off when high-band consonants dominate that body band.
        const float consonant=std::clamp((c.highEnergy-c.midEnergy*1.8f)/(.015f+c.highEnergy),0.f,1.f);
        const float bodyGate=gate*(1.f-.78f*consonant);
        // Breath is masked by loud voiced peaks: scaling by instantaneous level keeps the added
        // noise from stacking on waveform crests (AIRY raised peaks ~1-2 dB over the input).
        const float breath=noise * gate * std::min(.18f,c.env * .40f) / (1.f+3.f*amplitude);
        const float airy=x + upper*.12f*gate + breath;
        // Generate nonlinear throat harmonics only from the 240Hz..3.1kHz band.
        // Fundamental and high consonant detail bypass the saturation branch.
        const bool raspOn=w[2]>1.0e-4f,gritOn=w[3]>1.0e-4f;
        const float drive=mid*2.2f*(1.f+.30f*fryMod);
        const float softSat=raspOn ? c.raspSat.process(drive)*.455f : (c.raspSat.invalidate(),mid);
        const float raspHarmonics=(softSat-mid)*.34f*bodyGate;
        c.raspFiltered+=raspPole*(raspHarmonics-c.raspFiltered);
        c.creak += texturePole*(std::abs(mid)-c.creak);
        const float creak=std::clamp(c.creak*5.5f,0.f,1.f);
        const float rasp=x+c.raspFiltered+breath*.38f;
        // Extra low-frequency crackle is voice-locked, not an always-on oscillator.
        const float gritSat=gritOn ? c.gritSat.process(mid*4.f*(1.f+.65f*fryMod))*.245f : (c.gritSat.invalidate(),mid);
        const float gritHarmonics=(gritSat-mid)*.65f*bodyGate;
        c.gritFiltered+=raspPole*(gritHarmonics-c.gritFiltered);
        const float gritty=x+c.gritFiltered + mid*(creak*.11f*bodyGate)
                        + breath*.85f + noise*gate*std::min(.032f,c.env*.095f);
        const float y=w[0]*x+w[1]*airy+w[2]*rasp+w[3]*gritty;
        // Soft bounded output; do not clip clean or quiet source at zero amount.
        return std::clamp(y,-16.f,16.f);
    }
    std::array<Channel,2> state {};
    double envelopeAttack=.99,envelopeRelease=.999;
    float lowPole=.04f,highPole=.31f,texturePole=.004f,macroPole=.0004f,raspPole=.25f,sibilancePole=.005f;
    float macro=0.f,target=0.f;
    float fryInc=.001f,jitterPole=.001f,fryPhase=0.f,jitter=0.f,fryMod=1.f;
    float noiseHpPole=.09f,noiseLpPole=.5f,noiseGain=1.6f,nLow=0.f,nLp1=0.f,nLp2=0.f;
    std::uint32_t rng=0x42fe209bu;
};
} // namespace afar
