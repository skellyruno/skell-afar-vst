#pragma once
// TEST-ONLY behavioural stand-in for the shared SkellCore BandFDN.
// Actual v0.7.0 implementation is fetched by CMake for the VST3/Standalone
// and by headless-real-core.yml for release verification.
#include <SkellCore/Core.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
namespace skellcore {
template <std::size_t N>
class BandFDN {
    static_assert(N == 4 || N == 8 || N == 16, "Supported test sizes");
public:
    void prepare(double rate, double = 24., double = 80.) { fx_.prepare(rate); reset(); }
    void reset() noexcept { fx_.reset(); }
    void setTrebleReferenceHz(double) noexcept {}
    void setParameters(float low, float high, float wet) noexcept {
        wet_ = std::clamp(wet, 0.f, 1.f);
        const float damping = std::clamp(1.f - high/std::max(.15f,low), 0.f, 1.f);
        fx_.setParameters(low, damping, 1.f);
    }
    std::array<float,2> processFrame(float l, float r) noexcept {
        auto x=fx_.processFrame(l,r);
        // Uncalibrated test tail: use the actual SkellCore implementation for DSP benchmarks.
        constexpr float scale = .42f;
        return {(1.f-wet_)*l + wet_*scale*x[0],
                (1.f-wet_)*r + wet_*scale*x[1]};
    }
private:
    FDNReverb fx_;
    float wet_ = 0.25f;
};
using FDNReverb8 = BandFDN<8>;
using FDNReverb16 = BandFDN<16>;
} // namespace skellcore
