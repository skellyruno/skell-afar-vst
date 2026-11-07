#pragma once
// Test-only substitute: intentionally NOT linked into release builds.
// Smoke-tests SceneDSP finite behavior without downloading JUCE/SkellCore.
#include <array>
namespace skellcore {
struct FDNReverb {
 void prepare(double) {} void reset() {} void setParameters(float,float,float) {}
 std::array<float,2> processFrame(float a,float b) noexcept {return {a*.2f,b*.2f};}
};
struct TruePeakLimiter {
 enum class Quality { Economy,Standard };
 void prepare(double,int,Quality) {} void reset() {} void setCeilingDb(float) {}
 int latencySamples() const noexcept {return 0;}
 void processFrame(const float* in,float* out,int n) noexcept {for(int c=0;c<n;++c)out[c]=in[c];}
};
}
