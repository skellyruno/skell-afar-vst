// Level-match safety: abrupt path changes (distance / scene / effort jumps) must not produce a loudness spike
// while the slow loudness estimate catches up.
#include "force_asserts.h"
#include "../Source/dsp/SceneDSP.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <vector>
static double rmsDb(const std::vector<float>& v,size_t a,size_t b){double s=0;for(size_t i=a;i<b;++i)s+=double(v[i])*v[i];return 10*std::log10(s/double(b-a)+1e-20);}
int main(){
    const double sr=48000.;
    struct Jump{int sceneA,sceneB;float distA,distB,effA,effB;};
    const Jump jumps[]{{8,8,150.f,.3f,.05f,.05f},{8,8,.3f,150.f,.9f,.9f},{5,12,3.f,3.f,.5f,.5f},{8,8,20.f,20.f,.05f,.95f},{0,18,2.f,160.f,.9f,.1f}};
    double worst=-99;
    for(auto j:jumps){
        afar::SceneDSP e;e.prepare(sr,2,1);e.setSeed(3);e.setScene(j.sceneA);
        afar::SceneControls p;p.effort=j.effA;p.distance=j.distA;p.ambienceDb=-60.f;p.muteAmbience=true;p.levelMatch=.85f;p.scene=j.sceneA;
        std::uint32_t r=7u;float lp=0.f;
        auto noise=[&]{r=r*1664525u+1013904223u;const float n=static_cast<float>(r>>8)/8388608.f-1.f;lp+=.15f*(n-lp);return lp*.12f;};
        std::vector<float> out;
        for(int i=0;i<int(sr*5);++i){const float x=noise();out.push_back(e.processFrame({x,x},p,1.f,0.f).left);}
        const double before=rmsDb(out,size_t(sr*3),size_t(sr*5));
        p.scene=j.sceneB;p.distance=j.distB;p.effort=j.effB;e.setScene(j.sceneB);
        out.clear();
        for(int i=0;i<int(sr*5);++i){const float x=noise();out.push_back(e.processFrame({x,x},p,1.f,0.f).left);}
        const double settled=rmsDb(out,size_t(sr*3),size_t(sr*5));
        double hottest=-99;   // loudest 20 ms window during the transition vs the settled level
        for(size_t i=0;i+960<size_t(sr*1.5);i+=480) hottest=std::max(hottest,rmsDb(out,i,i+960));
        std::fprintf(stderr,"jump scene %d->%d dist %.1f->%.1f eff %.2f->%.2f: before %.1f dB, settled %.1f dB, hottest transition window %+.1f dB over settled\n",
                     j.sceneA,j.sceneB,j.distA,j.distB,j.effA,j.effB,before,settled,hottest-settled);
        worst=std::max(worst,hottest-std::max(settled,before));
    }
    std::fprintf(stderr,"worst transition overshoot over max(before,settled): %+.1f dB\n",worst);
    assert(worst<9.0);
    std::puts("PASS: abrupt path changes stay controlled");
}
