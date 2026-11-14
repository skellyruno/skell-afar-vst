// Randomised robustness: every parameter over its full range, random scene switches, extreme inputs and
// sample rates. Output must stay finite and bounded; silence after signal must not stall on denormals.
#include "force_asserts.h"
#include "../Source/dsp/SceneDSP.h"
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>
#if defined(__SSE__) || defined(_M_X64)
 #include <xmmintrin.h>
#endif
static float urand(std::mt19937& g,float lo,float hi){return std::uniform_real_distribution<float>(lo,hi)(g);}
static afar::SceneControls randomControls(std::mt19937& g){
    afar::SceneControls p;
    p.effort=urand(g,0,1);p.distance=std::exp(urand(g,std::log(.05f),std::log(200.f)));p.angle=urand(g,0,180);
    p.occlusion=urand(g,0,1);p.throatTexture=urand(g,0,1);p.size=urand(g,.5f,2.f);p.ambienceDb=urand(g,-60,0);
    p.presence=urand(g,0,1);p.width=urand(g,0,2);p.mix=urand(g,0,1);p.outputDb=urand(g,-24,24);
    p.temperature=urand(g,-20,50);p.humidity=urand(g,0,100);p.absorption=urand(g,0,1);p.levelMatch=urand(g,0,1);
    p.scene=int(g()%afar::spaces.size());p.recordedAs=int(g()%5);p.occlusionType=int(g()%6);p.micType=int(g()%8);p.micStereo=int(g()%4);
    p.removeDelay=g()&1;p.bypassEffort=g()&1;p.muteAmbience=(g()%5)==0;p.ambienceDuck=g()&1;p.delta=(g()%6)==0;
    for(auto& b:p.beds)b=urand(g,-60,12);
    return p;
}
int main(){
    std::mt19937 g(20261111u);
    const double rates[]{22050.,44100.,48000.,96000.,192000.};
    double worstPeak=0;long frames=0;
    for(int trial=0;trial<60;++trial){
        const double sr=rates[trial%5];
        afar::SceneDSP e;e.prepare(sr,(trial&1)?2:1,trial%3==0?0:1);e.setSeed(uint32_t(g()));
        auto p=randomControls(g);e.setScene(p.scene);
        const int kind=trial%6;                      // 0 noise, 1 full-scale square, 2 impulses, 3 DC, 4 silence, 5 loud sine sweep
        float ph=0.f;uint32_t r=trial+1u;
        const int total=int(sr*2.0);
        for(int i=0;i<total;++i){
            if(i%int(sr*.25)==0){auto q=randomControls(g);if(g()%2)q.scene=p.scene;p=q;e.setScene(p.scene);}   // abrupt parameter jumps
            r=r*1664525u+1013904223u;const float n=static_cast<float>(r>>8)/8388608.f-1.f;
            float x=0.f;
            switch(kind){case 0:x=n;break;case 1:x=((i/40)&1)?1.f:-1.f;break;case 2:x=(i%997==0)?1.f:0.f;break;
                         case 3:x=.9f;break;case 4:x=0.f;break;default:ph+=6.2831853f*(50.f+i*.2f)/float(sr);x=std::sin(ph);}
            const auto f=e.processFrame({x,x*.5f},p,1.f,0.f);
            assert(std::isfinite(f.left)&&std::isfinite(f.right));
            worstPeak=std::max(worstPeak,double(std::max(std::abs(f.left),std::abs(f.right))));
            ++frames;
        }
    }
    std::fprintf(stderr,"fuzz: %ld frames, worst |out| %.2f (stub limiter; real limiter ceilings at -1 dBFS)\n",frames,worstPeak);
    // Pre-limiter bound: extreme settings (output +24 dB, ambience 0 dB, beds +12 dB) plus carry-over
    // across abrupt jumps can legitimately be loud; the real limiter ceilings at -1 dBFS.
    assert(worstPeak<64.0);
    // Denormal stall check. processBlock() enables flush-to-zero (juce::ScopedNoDenormals); mirror that
    // here and require silence after a loud burst to cost no more than normal processing. Without FTZ the
    // number is only reported (subnormal filter/delay states decay slowly and are several times slower).
    for(int ftz=0;ftz<2;++ftz){
#if defined(__SSE__) || defined(_M_X64)
        const unsigned oldCsr=_mm_getcsr();_mm_setcsr(ftz?(oldCsr|0x8040u):(oldCsr&~0x8040u));
#endif
        const double sr=48000.;afar::SceneDSP e;e.prepare(sr,2,1);e.setScene(12);
        afar::SceneControls p;p.scene=12;p.throatTexture=.7f;p.levelMatch=.85f;p.ambienceDb=-60.f;p.muteAmbience=true;
        std::uint32_t r=9u;
        auto run=[&](int n,bool sig){auto t0=std::chrono::steady_clock::now();for(int i=0;i<n;++i){r=r*1664525u+1013904223u;const float x=sig?(static_cast<float>(r>>8)/8388608.f-1.f)*.3f:0.f;e.processFrame({x,x},p,1.f,0.f);}
                                      return std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();};
        const double tSig=run(int(sr*2),true);
        double worstSilence=0;for(int k=0;k<10;++k)worstSilence=std::max(worstSilence,run(int(sr),false));
        std::fprintf(stderr,"denormal check (%s): 2 s signal %.3f s, worst 1 s of silence %.3f s\n",ftz?"FTZ on, as in the plugin":"FTZ off, info only",tSig,worstSilence);
        if(ftz) assert(worstSilence<tSig*3.0+.05);   // generous: shared CI runners are noisy
#if defined(__SSE__) || defined(_M_X64)
        _mm_setcsr(oldCsr);
#endif
    }
    std::puts("PASS: fuzz (60 trials x all params/scenes/rates/inputs), abrupt jumps, denormal stall");
}
