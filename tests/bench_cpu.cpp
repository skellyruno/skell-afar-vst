// Manual CPU benchmark (not part of CTest): g++ -O2 -std=c++17 -Itests/stub tests/bench_cpu.cpp && ./a.out
// NOTE: with the test stub the FDN and limiter are trivial, so this measures SceneDSP itself (direct path,
// room character, throat texture, beds, level match); the real SkellCore FDN/limiter add to it.
#include "../Source/dsp/SceneDSP.h"
#include <chrono>
#include <cstdio>
#include <cstdint>
int main(){
    const double sr=48000.;const int n=int(sr*10);
    double worst=0,sum=0;int worstScene=0;
    for(int scene=0;scene<int(afar::spaces.size());++scene){
        afar::SceneDSP e;e.prepare(sr,2,1);e.setScene(scene);
        afar::SceneControls p;p.scene=scene;p.throatTexture=.7f;p.levelMatch=.85f;p.presence=.5f;p.distance=12.f;p.ambienceDb=-20.f;
        std::uint32_t r=1u;
        const auto t0=std::chrono::steady_clock::now();
        for(int i=0;i<n;++i){r=r*1664525u+1013904223u;const float x=(static_cast<float>(r>>8)/8388608.f-1.f)*.3f;e.processFrame({x,x},p,1.f,0.f);}
        const double s=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
        sum+=s;if(s>worst){worst=s;worstScene=scene;}
    }
    std::printf("average over %zu scenes: %.1f ns/frame (%.2f%% of one core)\n",afar::spaces.size(),sum/double(afar::spaces.size())/n*1e9,sum/double(afar::spaces.size())/10.0*100.0);
    std::printf("worst scene %d: %.1f ns/frame, %.2f%% of one core at 48 kHz stereo (10 s render in %.3f s)\n",worstScene,worst/n*1e9,worst/10.0*100.0,worst);
}
