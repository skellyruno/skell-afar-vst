#include "../Source/dsp/SceneDSP.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
    // Test-only SkellCore stub: validates native scene engine's real-time
    // invariants, not the JUCE wrapper or the production true-peak limiter.
    for(double sr : {44100.0,48000.0,96000.0}) {
        afar::SceneDSP dsp;
        dsp.prepare(sr,2,0);
        dsp.setSeed(12345);
        afar::SceneControls p;
        float maxAmplitude=0.f;
        for(int sample=0;sample<60000;++sample) {
            if(sample%3900==0) dsp.setScene((sample/3900)%15);
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
    std::puts("PASS: rapid scene changes, automation sweep, multirate stability and seeded deterministic ambience");
}
