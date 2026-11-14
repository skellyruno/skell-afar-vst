#include "force_asserts.h"
#include "../Source/dsp/SceneDSP.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main(){
    constexpr int sr=48000;
    double worst=-180.0;
    for(int scene=0;scene<static_cast<int>(afar::spaces.size());++scene){
        afar::SceneDSP dsp; dsp.prepare(sr,2,0);dsp.setScene(scene);
        afar::SceneControls p; p.scene=scene;
        double energy=0.0;int count=0;
        for(int i=0;i<sr*2;++i){
            const auto y=dsp.processFrame({0.f,0.f},p,1.f,0.f);
            assert(std::isfinite(y.left) && std::isfinite(y.right));
            if(i>sr){energy+=.5*(double(y.left)*y.left+double(y.right)*y.right);++count;}
        }
        const auto rmsDb=10.0*std::log10(energy/count+1.e-24);
        worst=std::max(worst,rmsDb);
        assert(rmsDb<-48.0); // all 20 idle environments stay unobtrusive at defaults
    }
    // Physical arrival is optional, not an unavoidable preset-dependent vocal latency.
    for(int aligned=0;aligned<2;++aligned){
        afar::SceneDSP dsp;dsp.prepare(sr,2,0);dsp.setScene(18);
        afar::SceneControls p;p.scene=18;p.distance=120.f;p.ambienceDb=-60.f;
        p.muteAmbience=true;p.levelMatch=0.f;p.removeDelay=aligned!=0;
        int onset=-1;
        for(int i=0;i<sr;++i){
            const auto f=dsp.processFrame({i==1000?.5f:0.f,i==1000?.5f:0.f},p,1.f,0.f);
            if(onset<0 && std::abs(f.left)>.00002f)onset=i;
        }
        std::printf("Beat aligned %d: first audible transient at %d, reported limiter latency %d\n",aligned,onset,dsp.latencySamples());
        if(aligned)assert(onset>=1000+dsp.latencySamples() && onset<1000+dsp.latencySamples()+64);
        else assert(onset>1000+dsp.latencySamples()+5000); // direct TOF intentionally delayed when enabled
    }
    std::printf("PASS: all 20 default ambient beds below -48 dBFS rms (worst %.2f dBFS), arrival delay opt-in\n",worst);
}
