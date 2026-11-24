#include "force_asserts.h"
#include <vector>
#include "../Source/dsp/ThroatTexture.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

int main() {
    afar::ThroatTexture texture;
    for (double sr : {8000.0,44100.0,48000.0,96000.0,192000.0}) {
        texture.prepare(sr);
        // Default CLEAN is exact pass-through, including stereo phase polarity.
        auto f=texture.process(.24f,-.24f);
        assert(f.left==.24f && f.right==-.24f);
        texture.setAmount(1.f);
        float energy=0;
        for (int i=0;i<static_cast<int>(sr*1.2);++i) {
            const float sample=.18f*std::sin(2.f*3.14159265358979323846f*180.f*static_cast<float>(i)/static_cast<float>(sr));
            f=texture.process(sample,sample);
            assert(std::isfinite(f.left) && std::isfinite(f.right));
            energy+=std::abs(f.left-sample);
        }
        assert(energy>0.3f); // colored voice changes, but stays numerically stable
        texture.prepare(sr);
        texture.setAmount(.33f);
        float silenceMax=0;
        for(int i=0;i<static_cast<int>(sr);++i) {
            f=texture.process(0.f,0.f);
            silenceMax=std::max(silenceMax,std::abs(f.left));
        }
        assert(silenceMax < 1.e-6f); // no artificial fixed noise floor
    }
    const auto a=afar::ThroatTexture::morphWeights(.3333333f);
    const auto b=afar::ThroatTexture::morphWeights(.6666667f);
    assert(a[1]>.999f && b[2]>.999f);
    const auto safe=afar::ThroatTexture::morphWeights(std::numeric_limits<float>::quiet_NaN());
    assert(safe[0]==1.f);
    {   // Regression: breath noise stays band-shaped (was white: ~79% of added energy above 5 kHz).
        const double sr=48000.0;
        texture.prepare(sr);texture.setAmount(1.f);
        float lp1=0.f,lp2=0.f,total=0.f,high=0.f;
        const float pole=static_cast<float>(1.0-std::exp(-2.0*3.141592653589793*5000.0/sr));
        for(int i=0;i<static_cast<int>(sr*2);++i) {
            const float x=.2f*std::sin(2.f*3.14159265358979323846f*180.f*static_cast<float>(i)/static_cast<float>(sr));
            const float d=texture.process(x,x).left-x;
            lp1+=pole*(d-lp1);lp2+=pole*(lp1-lp2);
            if(i>static_cast<int>(sr*.5)){total+=d*d;const float h=d-lp2;high+=h*h;}
        }
        assert(total>0.f && high/total<.5f);
    }
    {   // Regression: ADAA tanh leaves less out-of-harmonic (aliased) energy than plain tanh for a hot 5.2 kHz tone.
        const double sr=44100.0,f=5200.0,pi=3.141592653589793;const int n=static_cast<int>(sr);
        std::vector<float> plain(static_cast<std::size_t>(n)),adaa(static_cast<std::size_t>(n));
        afar::ThroatTexture::AdaaTanh a;
        for(int i=0;i<n;++i){
            const float x=4.f*.9f*std::sin(static_cast<float>(2.0*pi*f*i/sr)),w=.5f-.5f*std::cos(static_cast<float>(2.0*pi*i/(n-1)));
            plain[static_cast<std::size_t>(i)]=std::tanh(x)*w;adaa[static_cast<std::size_t>(i)]=a.process(x)*w;   // Hann-windowed
        }
        auto outOfHarmonic=[&](const std::vector<float>& v){
            double legit=0,other=0;
            for(double fr=100.0;fr<21900.0;fr+=100.0){
                const double wv=2*pi*fr/sr,c=2*std::cos(wv);double s0=0,s1=0,s2=0;
                for(float x:v){s0=x+c*s1-s2;s2=s1;s1=s0;}
                const double pw=s1*s1+s2*s2-c*s1*s2;
                const bool isH=std::abs(fr-5200.0)<250.0||std::abs(fr-15600.0)<250.0;
                (isH?legit:other)+=pw;
            }
            return other/(legit+other);
        };
        const double o0=outOfHarmonic(plain),o1=outOfHarmonic(adaa);
        assert(o1<o0*.6);   // measured ~7 dB (0.2x); require at least 2.2 dB
    }
    std::puts("PASS: throat macro, clean identity, silence, 5 sample rates, bounded output");
}
