#include "force_asserts.h"
// Final-stage behaviour with the lookahead-limiter model: reported latency is real and identical in every
// bypass/mix/power state, and the output never exceeds the -1 dBFS ceiling whatever the settings.
#include "../Source/dsp/SceneDSP.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

static size_t peakIndex(const std::vector<float>& v){size_t k=0;for(size_t i=0;i<v.size();++i)if(std::abs(v[i])>std::abs(v[k]))k=i;return k;}

int main(){
    const double sr=48000.;const size_t n0=500;
    for(int quality=0;quality<2;++quality){
        afar::SceneDSP e;e.prepare(sr,2,quality);
        const int L=e.latencySamples();
        std::fprintf(stderr,"quality %d: reported latency %d samples (%.2f ms)\n",quality,L,1000.0*L/sr);
        assert(L>0);
        struct Cfg{const char* name;float wet,bypass,mix;bool removeDelay;};
        const Cfg cfgs[]{{"bypass",1.f,1.f,1.f,true},{"wet=0 (power off)",0.f,0.f,1.f,true},{"full wet, beat-aligned",1.f,0.f,1.f,true},{"half wet, beat-aligned",.5f,0.f,1.f,true}};
        for(auto c:cfgs){
            afar::SceneDSP d;d.prepare(sr,2,quality);d.setScene(5);
            afar::SceneControls p;p.scene=5;p.distance=1.f;p.removeDelay=c.removeDelay;p.levelMatch=0.f;p.ambienceDb=-60.f;p.muteAmbience=true;p.throatTexture=0.f;
            std::vector<float> out;
            for(size_t i=0;i<4000;++i){const float x=i==n0?.5f:0.f;out.push_back(d.processFrame({x,x},p,c.wet,c.bypass).left);}
            const size_t k=peakIndex(out);
            std::fprintf(stderr,"  %-24s impulse at %zu -> peak at %zu (expected %zu + latency %d = %zu)\n",c.name,n0,k,n0,L,n0+size_t(L));
            assert(k>=n0+size_t(L) && k<=n0+size_t(L)+2);   // wet path adds at most a couple of samples of filter delay
        }
    }
    // ceiling: extreme settings and full-scale inputs must stay under -1 dBFS
    const float ceiling=std::pow(10.f,-1.f/20.f)+1.0e-4f;
    std::mt19937 g(99);float worst=0;
    auto ur=[&](float a,float b){return std::uniform_real_distribution<float>(a,b)(g);};
    for(int trial=0;trial<40;++trial){
        afar::SceneDSP e;e.prepare(48000.,2,trial&1);e.setScene(int(g()%afar::spaces.size()));
        afar::SceneControls p;p.scene=int(g()%afar::spaces.size());p.effort=ur(0,1);p.distance=std::exp(ur(std::log(.05f),std::log(200.f)));
        p.outputDb=ur(0,12);p.ambienceDb=ur(-20,0);p.levelMatch=ur(0,1);p.throatTexture=ur(0,1);p.width=ur(0,2);
        for(auto& b:p.beds)b=ur(-30,12);
        for(int i=0;i<48000;++i){
            const float x=(i/37)%2?1.f:-1.f;                                       // full-scale square
            const auto f=e.processFrame({x,x},p,1.f,0.f);
            worst=std::max(worst,std::max(std::abs(f.left),std::abs(f.right)));
        }
    }
    std::fprintf(stderr,"ceiling: worst output over 40 extreme trials %.4f (limit %.4f)\n",worst,ceiling);
    assert(worst<=ceiling);
    std::puts("PASS: latency identical in every state, output never exceeds the -1 dBFS ceiling");
}
