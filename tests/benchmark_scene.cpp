// Repeatable release-mode SceneDSP benchmark. No JUCE, no audio-device I/O.
#include "../Source/dsp/SceneDSP.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif
int main() {
#if defined(__SSE__)
    _mm_setcsr(_mm_getcsr()|0x8040u);
#endif
    using Clock=std::chrono::steady_clock;
    std::printf("rate,engine,motion,median_ns_per_frame,realtime_percent,checksum\n");
    for(double rate:{48000.,96000.}) for(int engine=0;engine<3;++engine) for(bool motion:{false,true}) {
        const int count=static_cast<int>(rate*2);
        std::vector<afar::Frame> input(static_cast<std::size_t>(count));
        for(int i=0;i<count;++i) {
            const float x=.12f*std::sin(afar::pi*2.f*173.f*static_cast<float>(i/rate));
            input[static_cast<std::size_t>(i)]={x,x*.85f};
        }
        std::array<double,3> durations{};
        double checksum=0.;
        for(auto& duration:durations) {
            afar::SceneDSP dsp;dsp.prepare(rate,2,1);dsp.setScene(12);
            afar::SceneControls p;p.scene=12;p.reverbEngine=engine;p.distance=6.f;
            p.levelMatch=.85f;p.ambienceDb=-34.f;p.throatTexture=.7f;p.effort=.35f;
            for(int i=0;i<count/2;++i) dsp.processFrame(input[static_cast<std::size_t>(i)],p,1.f,0.f);
            const auto begin=Clock::now();
            for(int i=0;i<count;++i) {
                if(motion) p.distance=6.f+34.f*static_cast<float>(i)/count;
                const auto out=dsp.processFrame(input[static_cast<std::size_t>(i)],p,1.f,0.f);
                if(!std::isfinite(out.left)||!std::isfinite(out.right)) return 1;
                checksum+=std::abs(out.left)+std::abs(out.right);
            }
            duration=std::chrono::duration<double>(Clock::now()-begin).count();
        }
        std::sort(durations.begin(),durations.end());
        std::printf("%.0f,%d,%d,%.3f,%.3f,%.6f\n",rate,engine,int(motion),
                    durations[1]*1.e9/count,durations[1]*rate*100./count,checksum);
    }
}
