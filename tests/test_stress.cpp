#include "force_asserts.h"
#include "../Source/dsp/SceneDSP.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

int main() {
    // Test-only SkellCore stub: validates native scene engine's real-time
    // invariants, not the JUCE wrapper or the production true-peak limiter.
    constexpr int sceneChangeInterval = 3900;
    constexpr int sceneCount = static_cast<int>(afar::spaces.size());
    constexpr int sampleCount = sceneChangeInterval * sceneCount;

    for(double sr : {44100.0,48000.0,96000.0}) {
        afar::SceneDSP dsp;
        dsp.prepare(sr,2,0);
        dsp.setSeed(12345);
        afar::SceneControls p;
        float maxAmplitude=0.f;
        // Run one full interval for every factory scene. Derive the count from
        // the catalog so future scenes cannot silently fall outside this gate.
        for(int sample=0;sample<sampleCount;++sample) {
            if(sample%sceneChangeInterval==0)
                dsp.setScene((sample/sceneChangeInterval)%sceneCount);
            p.effort=(sample%4200)/4200.f;
            p.distance=afar::distanceMeters((sample%9000)/9000.f);
            p.throatTexture=(sample%5000)/5000.f;
            p.angle=(sample%3600)/20.f;
            p.occlusion=static_cast<float>((sample%1000))/1000.f;
            p.size=.5f+static_cast<float>((sample%3000))/2000.f;
            p.micType=(sample/9000)%8;
            p.ambienceDb=-30.f+static_cast<float>(sample%1000)/100.f;
            p.removeDelay=(sample/7000)%2==0;
            const float source=.12f*std::sin(afar::pi*2.f*170.f*static_cast<float>(sample/sr));
            const auto out=dsp.processFrame({source,-source*.64f},p,.95f,0.f);
            assert(std::isfinite(out.left)&&std::isfinite(out.right));
            maxAmplitude=std::max(maxAmplitude,std::max(std::abs(out.left),std::abs(out.right)));
        }
        assert(maxAmplitude>1.e-5f && maxAmplitude<100.f);
    }
    // A short fractional tap must retain its resolution even deep in a large
    // ring (as used for long reflections at high host sample rates).
    {
        afar::DelayLine ring;
        ring.prepare(768008);
        for(int i=0;i<700000;++i) { ring.push(0.f); ring.advance(); }
        ring.push(1.f);
        assert(std::abs(ring.tap(.03125f)-.96875f)<1.e-6f);
        assert(ring.tap(0.f)==1.f && ring.tap(1.f)==0.f);
        ring.reset();
        ring.push(1.f);
        assert(std::abs(ring.tap(.25f)-.75f)<1.e-6f);
        assert(ring.tap(std::numeric_limits<float>::quiet_NaN())==1.f);
    }
    // Corrupt input from an upstream plugin must be equivalent to replacing only
    // the corrupt channel with silence, including after a loudness-match render.
    for(double rate:{44100.,48000.,96000.}) {
        afar::SceneDSP clean,damaged;
        clean.prepare(rate,2,1);damaged.prepare(rate,2,1);
        afar::SceneControls c;c.levelMatch=.85f;c.throatTexture=.7f;
        c.effort=.2f;c.ambienceDb=-20.f;c.ambienceDuck=true;
        for(int i=0;i<24000;++i) {
            const float x=.12f*std::sin(afar::pi*2.f*170.f*static_cast<float>(i/rate));
            const bool bad=i==1200||i==2400||i==3600;
            const float corrupt=i==1200?std::numeric_limits<float>::quiet_NaN():
                i==2400?std::numeric_limits<float>::infinity():-std::numeric_limits<float>::infinity();
            const auto a=clean.processFrame({bad?0.f:x,x},c,1.f,0.f);
            const auto b=damaged.processFrame({bad?corrupt:x,x},c,1.f,0.f);
            assert(std::isfinite(b.left)&&std::isfinite(b.right));
            assert(a.left==b.left && a.right==b.right);
        }
    }
    // Same inputs/seed/space produce bitwise identical procedural ambience.
    afar::SceneDSP a,b;
    a.prepare(48000,2,0);b.prepare(48000,2,0);
    a.setSeed(2881);b.setSeed(2881);
    afar::SceneControls p;
    p.scene=1;p.ambienceDb=-6.f;p.muteAmbience=false;
    a.setScene(1);b.setScene(1);
    for(int i=0;i<8192;++i){
        const auto x=a.processFrame({0.f,0.f},p,1.f,0.f);
        const auto y=b.processFrame({0.f,0.f},p,1.f,0.f);
        assert(x.left==y.left && x.right==y.right);
    }
    std::puts("PASS: all factory scenes, automation sweep, multirate stability and seeded deterministic ambience");
}
