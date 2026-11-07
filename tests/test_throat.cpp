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
    std::puts("PASS: throat macro, clean identity, silence, 5 sample rates, bounded output");
}
