// Independent ambience beds + Presence Hold. Uses the test-only SkellCore stub (see tests/stub).
#include "../Source/dsp/SceneDSP.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <vector>

static double rms(const std::vector<float>& v,size_t from=0){double s=0;for(size_t i=from;i<v.size();++i)s+=v[i]*v[i];return std::sqrt(s/std::max<size_t>(1,v.size()-from));}
static double goertzel(const std::vector<float>& v,double f,double sr){
    const double w=2*3.141592653589793*f/sr,c=2*std::cos(w);double s0=0,s1=0,s2=0;
    for(float x:v){s0=x+c*s1-s2;s2=s1;s1=s0;} return (s1*s1+s2*s2-c*s1*s2)/v.size();}

// silent input, ambience only
static std::vector<float> ambienceRender(int scene,const afar::SceneControls& p,int frames=96000){
    afar::SceneDSP e;e.prepare(48000.,2,0);e.setSeed(777);e.setScene(scene);
    std::vector<float> out;out.reserve(static_cast<size_t>(frames));
    for(int i=0;i<frames;++i){auto f=e.processFrame({0.f,0.f},p,1.f,0.f);out.push_back(f.left);}
    return out;
}
static std::vector<float> voiceRender(const afar::SceneControls& p,int frames=96000){
    afar::SceneDSP e;e.prepare(48000.,2,0);e.setSeed(777);e.setScene(p.scene);
    std::uint32_t r=12345u;std::vector<float> out;
    for(int i=0;i<frames;++i){r=r*1664525u+1013904223u;const float x=(static_cast<float>(r>>8)/8388608.f-1.f)*.1f;
        auto f=e.processFrame({x,x},p,1.f,0.f);out.push_back(f.left);}
    return out;
}
int main(){
    afar::SceneControls p;p.ambienceDb=-10.f;p.scene=5;   // graveyard: native bed is not wind
    const auto native=ambienceRender(5,p);
    const double rNative=rms(native,24000);
    afar::SceneControls off=p;off.beds.fill(-60.f);
    const double rOff=rms(ambienceRender(5,off),24000);
    afar::SceneControls windOnly=off;windOnly.beds[0]=12.f;   // add wind to a scene that doesn't have it
    const double rWind=rms(ambienceRender(5,windOnly),24000);
    afar::SceneControls trimmed=p;for(auto& b:trimmed.beds)b=-32.f;  // -12 dB trim of all native beds
    const double rTrim=rms(ambienceRender(5,trimmed),24000);
    std::fprintf(stderr,"rms native %.2e  all-off %.2e  wind-added %.2e  trim-12dB %.2e (ratio %.2f)\n",rNative,rOff,rWind,rTrim,rTrim/rNative);
    assert(rNative>1.0e-4);   // audible, not buried (~-80 dBFS before calibration)
    assert(rOff<1.0e-9);                      // every bed off => silence
    assert(rWind>1.0e-6);                     // a non-native bed can be added
    assert(rTrim/rNative>.15 && rTrim/rNative<.35);   // about -12 dB (0.25)
    // determinism with the same seed
    assert(ambienceRender(5,p,20000)==ambienceRender(5,p,20000));
    // every bed alone is finite, non-silent when raised, and bounded
    for(size_t i=0;i<8;++i){
        afar::SceneControls only=off;only.beds[i]=12.f;
        const auto r=ambienceRender(5,only,48000);
        for(float x:r)assert(std::isfinite(x)&&std::abs(x)<1.f);
        std::fprintf(stderr,"bed %zu alone rms %.2e\n",i+1,rms(r,12000));
        assert(rms(r,12000)>1.0e-7);
    }
    // Presence Hold: restores 2-5 kHz at distance, bounded overall change
    afar::SceneControls v;v.scene=8;v.distance=60.f;v.ambienceDb=-60.f;v.mix=1.f;
    const auto a=voiceRender(v);v.presence=1.f;const auto b=voiceRender(v);
    auto band=[&](const std::vector<float>& x){double s=0;for(double f:{2200.,3000.,3800.,4600.})s+=goertzel(x,f,48000.);return s;};
    const double gainDb=10*std::log10(band(b)/band(a)),totalDb=20*std::log10(rms(b,12000)/rms(a,12000));
    std::fprintf(stderr,"presence=1 at 60 m: 2-5 kHz %+.1f dB, overall %+.1f dB\n",gainDb,totalDb);
    assert(gainDb>3.5 && totalDb<6.0);
    v.presence=0.f;v.distance=.3f;const auto c0=voiceRender(v);v.presence=1.f;const auto c1=voiceRender(v);
    std::fprintf(stderr,"presence=1 at 0.3 m: 2-5 kHz %+.1f dB\n",10*std::log10(band(c1)/band(c0)));
    std::puts("PASS: independent beds (off/add/trim/determinism/bounds) and presence hold");
}
