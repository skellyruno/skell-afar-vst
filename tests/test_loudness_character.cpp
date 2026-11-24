// Loudness matching across scenes/effort/distance, and measurable distinctness of every space.
// Uses the test-only SkellCore stub (reverb tails are stand-in; compare spaces relative to each other).
#include "force_asserts.h"
#include "../Source/dsp/SceneDSP.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

static double rmsOf(const std::vector<float>& v,size_t from){double s=0;for(size_t i=from;i<v.size();++i)s+=double(v[i])*v[i];return std::sqrt(s/std::max<size_t>(1,v.size()-from));}
static double db(double x){return 20*std::log10(std::max(x,1e-12));}
static double goertzel(const std::vector<float>& v,double f,double sr){
    const double w=2*3.141592653589793*f/sr,c=2*std::cos(w);double s0=0,s1=0,s2=0;
    for(float x:v){s0=x+c*s1-s2;s2=s1;s1=s0;} return (s1*s1+s2*s2-c*s1*s2)/std::max<size_t>(1,v.size());}

// speech-like test signal: noise through a syllabic envelope (about 4 Hz), ~ -20 dBFS rms
static std::vector<float> speechLike(int frames,double sr){
    std::uint32_t r=987654u;std::vector<float> x(static_cast<size_t>(frames));float lp=0.f;
    for(int i=0;i<frames;++i){r=r*1664525u+1013904223u;const float n=(static_cast<float>(r>>8)/8388608.f-1.f);
        lp+=.12f*(n-lp);const float env=.55f+.45f*std::sin(float(2*3.141592653589793*4.0*i/sr));
        x[static_cast<size_t>(i)]=lp*env*.62f;}
    return x;
}
static std::vector<float> render(int scene,afar::SceneControls p,const std::vector<float>& in){
    afar::SceneDSP e;e.prepare(48000.,2,0);e.setSeed(5);e.setScene(scene);p.scene=scene;
    std::vector<float> out;out.reserve(in.size());
    for(float x:in){auto f=e.processFrame({x,x},p,1.f,0.f);out.push_back(f.left);}
    return out;
}
int main(){
    const double sr=48000.;const int frames=int(sr*6);
    const auto in=speechLike(frames,sr);const double inRms=rmsOf(in,size_t(sr*2));
    std::fprintf(stderr,"input rms %.1f dBFS\n",db(inRms));
    // ---- 1. loudness: matched vs unmatched, every scene, whisper-far to shout-near
    struct Case{float effort,distance;};
    const Case cases[]{{.05f,150.f},{.05f,30.f},{.5f,3.f},{.95f,.3f},{.95f,80.f}};
    double worstMatched=0,worstRaw=0;
    for(size_t sc=0;sc<afar::spaces.size();++sc){
        for(auto cs:cases){
            afar::SceneControls p;p.effort=cs.effort;p.distance=cs.distance;p.ambienceDb=-60.f;p.muteAmbience=true;p.recordedAs=2;
            p.levelMatch=0.f;const double raw=db(rmsOf(render(int(sc),p,in),size_t(sr*3))/inRms);
            p.levelMatch=1.f;const double matched=db(rmsOf(render(int(sc),p,in),size_t(sr*3))/inRms);
            worstRaw=std::max(worstRaw,std::abs(raw));worstMatched=std::max(worstMatched,std::abs(matched));
            if(std::abs(matched)>3.0) std::fprintf(stderr,"  scene %zu effort %.2f dist %.0f: raw %+.1f dB matched %+.1f dB\n",sc,cs.effort,cs.distance,raw,matched);
        }
    }
    std::fprintf(stderr,"worst |level change| raw %.1f dB, matched %.1f dB\n",worstRaw,worstMatched);
    // Measured: ~3.3 dB with the behavioural stub, 3.8 dB with the real SkellCore FDN (far whispers in small
    // rooms stay slightly quieter than dry, which is plausible). Limit set from the real-core measurement.
    assert(worstMatched<4.5);
    assert(worstRaw>worstMatched+6.0);   // matching must actually be doing something
    // ---- 2. idle ambience is quiet (no input): <= -40 dBFS at default settings
    for(size_t sc=0;sc<afar::spaces.size();++sc){
        afar::SceneControls p;p.ambienceDb=-24.f;p.levelMatch=1.f;
        const double a=db(rmsOf(render(int(sc),p,std::vector<float>(size_t(sr*3),0.f)),size_t(sr)));
        if(a>-40.0) std::fprintf(stderr,"  scene %zu idle ambience %.1f dBFS\n",sc,a);
        assert(a<-40.0);
    }
    // ---- 3. distinctness: feature vector per space from a noise burst through the raw (unmatched) path
    std::uint32_t r=42u;std::vector<float> burst(size_t(sr*3),0.f);
    for(size_t i=0;i<size_t(sr*.05);++i){r=r*1664525u+1013904223u;burst[i]=(static_cast<float>(r>>8)/8388608.f-1.f)*.5f;}
    std::vector<std::vector<double>> feat;
    for(size_t sc=0;sc<afar::spaces.size();++sc){
        afar::SceneControls p;p.distance=3.f;p.ambienceDb=-60.f;p.muteAmbience=true;p.levelMatch=0.f;
        const auto y=render(int(sc),p,burst);
        // decay: time for energy envelope (30 ms blocks, after the burst) to fall 20 dB below its post-burst peak
        const size_t blk=size_t(sr*.03);std::vector<double> e;
        for(size_t i=size_t(sr*.08);i+blk<y.size();i+=blk){double s=0;for(size_t k=0;k<blk;++k)s+=double(y[i+k])*y[i+k];e.push_back(s);}
        double peak=0;for(double v:e)peak=std::max(peak,v);double t20=0;
        for(size_t i=0;i<e.size();++i){if(e[i]<peak*.01){t20=i*.03;break;}t20=(i+1)*.03;}
        // spectral tilt of the tail (200..600 Hz vs 3-6 kHz) and mid-band resonance prominence
        std::vector<float> tail(y.begin()+long(sr*.15),y.begin()+long(sr*1.5));
        const double lo=goertzel(tail,300,sr)+goertzel(tail,500,sr)+1e-18,hi=goertzel(tail,3500,sr)+goertzel(tail,5500,sr)+1e-18;
        // periodicity of the room response (flutter): max normalized autocorrelation of the envelope-free tail for lags 10..250 ms
        double best=0;const size_t n=std::min<size_t>(tail.size(),size_t(sr*.5));
        double e0=0;for(size_t i=0;i<n;++i)e0+=double(tail[i])*tail[i];
        for(size_t lag=size_t(sr*.010);lag<size_t(sr*.250);lag+=8){double c=0;for(size_t i=0;i+lag<n;i+=2)c+=double(tail[i])*tail[i+lag];best=std::max(best,std::abs(c)*2/(e0+1e-18));}
        feat.push_back({t20/2.0,(std::log10(lo/hi))/2.0,best*3.0,db(rmsOf(y,size_t(sr*.15)))/20.0});
        std::fprintf(stderr,"%-16s t20 %.2fs tilt %+.1f dB flutter %.2f tail %.1f dBFS\n",afar::getSpace(int(sc)).name,t20,10*std::log10(lo/hi),best,db(rmsOf(y,size_t(sr*.15))));
    }
    double closest=1e9;size_t ca=0,cb=0;
    for(size_t a=0;a<feat.size();++a)for(size_t b=a+1;b<feat.size();++b){
        double d=0;for(size_t k=0;k<feat[a].size();++k)d+=(feat[a][k]-feat[b][k])*(feat[a][k]-feat[b][k]);d=std::sqrt(d);
        if(d<closest){closest=d;ca=a;cb=b;}}
    std::fprintf(stderr,"closest pair: %s vs %s distance %.3f\n",afar::getSpace(int(ca)).name,afar::getSpace(int(cb)).name,closest);
    assert(closest>.10);   // every space keeps its own fingerprint
    assert(afar::spaces.size()==20 && afar::characters.size()==afar::spaces.size());
    std::puts("PASS: loudness matching, idle ambience level, space distinctness");
}
