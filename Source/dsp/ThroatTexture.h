#pragma once
// Skell-Afar source-domain throat colour (C++17, no JUCE dependency).
// A controlled approximation of breath/creak/grit, NOT true vocal-tract resynthesis.
// One automatable macro: CLEAN [0] -> AIRY [1/3] -> RASPY [2/3] -> GRITTY [1].
// Filters/gates act on voice-derived bands, not on the entire wet scene output.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace afar {
class ThroatTexture {
public:
    struct Stereo { float left, right; };
    void prepare(double rate) noexcept {
        const auto sr = std::clamp(std::isfinite(rate) ? rate : 48000.0, 8000.0, 384000.0);
        envelopeAttack = std::exp(-1.0 / (sr * .004));
        envelopeRelease = std::exp(-1.0 / (sr * .085));
        lowPole = static_cast<float>(1.0-std::exp(-2.0 * 3.141592653589793 * 240.0 / sr));
        highPole = static_cast<float>(1.0-std::exp(-2.0 * 3.141592653589793 * 3100.0 / sr));
        texturePole = static_cast<float>(1.0-std::exp(-2.0 * 3.141592653589793 * 36.0 / sr));
        macroPole = static_cast<float>(1.0-std::exp(-1.0 / (sr * .025)));
        reset();
    }
    void reset() noexcept { state = {}; macro = target = 0.f; rng = 0x42fe209bu; }
    void setAmount(float amount) noexcept {
        target = std::clamp(std::isfinite(amount) ? amount : 0.f,0.f,1.f);
    }
    Stereo process(float l,float r) noexcept {
        l = std::isfinite(l) ? l : 0.f;
        r = std::isfinite(r) ? r : 0.f;
        macro += macroPole * (target - macro);
        if (macro <= 1.0e-6f && target == 0.f) return {l,r};
        // Constant-CPU deterministic linked noise avoids decorrelated stereo hiss.
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        const float noise = (static_cast<float>(rng >> 8) * (1.f / 8388608.f)) - 1.f;
        const auto weights = morphWeights(macro);
        return {processOne(l,state[0],noise,weights),processOne(r,state[1],noise,weights)};
    }
    static std::array<float,4> morphWeights(float amount) noexcept {
        const float v = std::clamp(std::isfinite(amount) ? amount : 0.f,0.f,1.f)*3.f;
        const int section = std::min(2,static_cast<int>(v));
        const float fraction = std::clamp(v-static_cast<float>(section),0.f,1.f);
        // Equal power is inappropriate for strongly correlated morphs; linear weights
        // ensure continuity, no extra level spike, and exact CLEAN at zero.
        std::array<float,4> w {};
        w[static_cast<std::size_t>(section)] = 1.f-fraction;
        w[static_cast<std::size_t>(section+1)] = fraction;
        return w;
    }
private:
    struct Channel { float env=0.f, low=0.f, smoothedHigh=0.f, creak=0.f; };
    float processOne(float x, Channel& c, float noise,const std::array<float,4>& w) noexcept {
        x=std::clamp(x,-8.f,8.f);
        const float amplitude=std::abs(x);
        const float envPole=amplitude>c.env?static_cast<float>(envelopeAttack):static_cast<float>(envelopeRelease);
        c.env=envPole*c.env+(1.f-envPole)*amplitude;
        // Voice envelope gate is nearly zero on silence, softly increasing with
        // audible speech. Noise never runs at a fixed ambient level.
        const float gate=std::clamp((c.env-.0012f)*42.f,0.f,1.f);
        c.low += lowPole*(x-c.low);
        c.smoothedHigh += highPole*(x-c.smoothedHigh);
        const float mid=c.smoothedHigh-c.low;
        const float upper=x-c.smoothedHigh;
        const float breath=noise * gate * std::min(.18f,c.env * .40f);
        const float airy=x + upper*.12f*gate + breath;
        // Generate nonlinear throat harmonics only from the 240Hz..3.1kHz band.
        // Fundamental and high consonant detail bypass the saturation branch.
        const float drive=mid*2.2f;
        const float softSat=std::tanh(drive) * .455f;
        const float raspHarmonics=(softSat-mid)*.34f*gate;
        c.creak += texturePole*(std::abs(mid)-c.creak);
        const float creak=std::clamp(c.creak*5.5f,0.f,1.f);
        const float rasp=x+raspHarmonics+breath*.38f;
        // Extra low-frequency crackle is voice-locked, not an always-on oscillator.
        const float gritHarmonics=(std::tanh(mid*4.f)*.245f-mid)*.65f*gate;
        const float gritty=x+gritHarmonics + mid*(creak*.11f*gate)
                        + breath*.85f + noise*gate*std::min(.032f,c.env*.095f);
        const float y=w[0]*x+w[1]*airy+w[2]*rasp+w[3]*gritty;
        // Soft bounded output; do not clip clean or quiet source at zero amount.
        return std::clamp(y,-16.f,16.f);
    }
    std::array<Channel,2> state {};
    double envelopeAttack=.99,envelopeRelease=.999;
    float lowPole=.04f,highPole=.31f,texturePole=.004f,macroPole=.0004f;
    float macro=0.f,target=0.f;
    std::uint32_t rng=0x42fe209bu;
};
} // namespace afar
