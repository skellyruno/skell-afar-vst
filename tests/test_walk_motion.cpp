#include "force_asserts.h"
#include "../Source/dsp/WalkMotion.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

int main(){
    // All paths must loop without a sawtooth discontinuity: no teleport in the room field.
    for(int path=0;path<5;++path){
        const float a=afar::WalkMotion::shape(path,0.f);
        const float b=afar::WalkMotion::shape(path,1.f-1.e-5f);
        assert(std::abs(a-b)<1.e-3f);
        for(int i=0;i<10000;++i){
            const float x=afar::WalkMotion::shape(path,float(i)/10000.f);
            assert(std::isfinite(x) && x>=0.f && x<=1.f);
        }
    }
    afar::WalkMotion walk;
    walk.prepare(48000.);
    walk.beginBlock(false,true,120.);
    for(int i=0;i<2000;++i) assert(std::abs(walk.next(3.f,0,1.f,1.f)-3.f)<.01f);
    walk.beginBlock(true,true,120.);
    float prev=3.f,minD=200.f,maxD=0.f,peakStep=0.f;
    // 1 bar at 120 BPM = 2 s. Two full cycles, while the host is not playing.
    for(int i=0;i<4*48000;++i){
        const float d=walk.next(3.f,0,1.f,1.f);
        assert(std::isfinite(d) && d>=.05f && d<=200.f);
        minD=std::min(minD,d);maxD=std::max(maxD,d);
        peakStep=std::max(peakStep,std::abs(d-prev));prev=d;
    }
    assert(minD<3.f && maxD>13.f); // the movement progresses, rather than freezing at an unchanged PPQ
    assert(peakStep<.08f);          // no instantaneous jump even when the path repeats
    walk.beginBlock(false,true,120.);
    for(int i=0;i<48000;++i){
        const float d=walk.next(3.f,0,1.f,1.f);
        assert(std::isfinite(d) && std::abs(d-prev)<.08f);
        prev=d;
    }
    assert(std::abs(prev-3.f)<.02f); // returns smoothly to the manually controlled distance
    // Long running / host-automation fuzz: no NaNs or out-of-range distance.
    walk.beginBlock(true,true,120.);
    for(int i=0;i<240000;++i){
        if(i%2048==0)walk.beginBlock((i/2048)%3!=0,true,(i%7==0)?420.:80.);
        const float d=walk.next((i%351)*.57f+.05f,(i/71)%5,float(i%16)+1.f,.1f+(i%200)*.01f);
        assert(std::isfinite(d) && d>=.05f && d<=200.f);
    }
    std::printf("PASS: Walk enabled/disabled smoothly, all five paths loop continuously, peak step %.6f m, free-running clock\n",peakStep);
}
