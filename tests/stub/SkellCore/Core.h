#pragma once
// Test-only stand-ins for the pinned SkellCore dependency; intentionally NOT linked into release builds.
// They are behavioural models, not the real DSP: a 4-line Hadamard FDN with per-line RT60 decay and
// damping, and a lookahead peak limiter that guarantees its ceiling and reports its latency. They let the
// headless tests exercise realistic tails, latency and ceiling behaviour without downloading SkellCore.
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
namespace skellcore {
class FDNReverb {
public:
    void prepare(double sampleRate) {
        sr = sampleRate;
        static constexpr int base[4] {1031, 1327, 1523, 1871};   // mutually prime delay lengths at 48 kHz
        for (int i = 0; i < 4; ++i) {
            len[i] = std::max(8, static_cast<int>(std::lround(base[i] * sr / 48000.0)));
            buf[i].assign(static_cast<std::size_t>(len[i]), 0.f);
        }
        setParameters(1.f, .5f, 1.f);
        reset();
    }
    void reset() {
        for (auto& b : buf) std::fill(b.begin(), b.end(), 0.f);
        pos = {}; damp = {};
    }
    // rt60 seconds, high-frequency damping amount, (size scale, unused by the model)
    void setParameters(float rt60, float damping, float) {
        const float rt = std::max(.05f, rt60);
        for (int i = 0; i < 4; ++i) gain[i] = std::pow(10.f, -3.f * static_cast<float>(len[i]) / (rt * static_cast<float>(sr)));
        dampA = std::clamp(1.f - damping * .6f, .05f, 1.f);
    }
    std::array<float, 2> processFrame(float a, float b) noexcept {
        float out[4], y[4];
        for (int i = 0; i < 4; ++i) {
            out[i] = buf[i][static_cast<std::size_t>(pos[i])];
            damp[i] += dampA * (out[i] - damp[i]);
            y[i] = damp[i] * gain[i];
        }
        const float h[4] {.5f * (y[0] + y[1] + y[2] + y[3]), .5f * (y[0] - y[1] + y[2] - y[3]),
                          .5f * (y[0] + y[1] - y[2] - y[3]), .5f * (y[0] - y[1] - y[2] + y[3])};
        const float in[4] {a, b, b, a};
        for (int i = 0; i < 4; ++i) {
            buf[i][static_cast<std::size_t>(pos[i])] = h[i] + .5f * in[i];
            if (++pos[i] == len[i]) pos[i] = 0;
        }
        return {.5f * (out[0] + out[2]), .5f * (out[1] + out[3])};
    }
private:
    double sr = 48000.0;
    int len[4] {}; std::vector<float> buf[4]; std::array<int, 4> pos {}; std::array<float, 4> damp {};
    float gain[4] {}, dampA = .7f;
};

class TruePeakLimiter {
public:
    enum class Quality { Economy, Standard };
    void prepare(double sampleRate, int numChannels, Quality q) {
        channels = std::clamp(numChannels, 1, 2);
        look = std::max(1, static_cast<int>(std::lround(sampleRate * (q == Quality::Standard ? .0015 : .0007))));
        const auto n = static_cast<std::size_t>(look) + 1;
        for (auto& c : audio) c.assign(n, 0.f);
        req.assign(n, 1.f); minG.assign(n, 1.f);
        reset();
    }
    void reset() {
        for (auto& c : audio) std::fill(c.begin(), c.end(), 0.f);
        std::fill(req.begin(), req.end(), 1.f); std::fill(minG.begin(), minG.end(), 1.f);
        w = 0; sum = static_cast<double>(look + 1);
    }
    void setCeilingDb(float db) { ceiling = std::pow(10.f, db / 20.f); }
    int latencySamples() const noexcept { return look; }
    void processFrame(const float* in, float* out, int n) noexcept {
        const auto size = static_cast<std::size_t>(look) + 1;
        float peak = 0.f;
        for (int c = 0; c < n; ++c) { audio[c][w] = in[c]; peak = std::max(peak, std::abs(in[c])); }
        req[w] = peak > ceiling ? ceiling / peak : 1.f;
        float m = 1.f;
        for (std::size_t i = 0; i < size; ++i) m = std::min(m, req[i]);       // min over the lookahead window
        sum += static_cast<double>(m) - static_cast<double>(minG[w]);
        minG[w] = m;
        const float g = static_cast<float>(sum / static_cast<double>(size));  // moving average over the window
        const std::size_t r = (w + 1) % size;                                 // oldest sample = `look` samples ago
        for (int c = 0; c < n; ++c) out[c] = audio[c][r] * g;
        w = r;
    }
private:
    int channels = 2, look = 1; float ceiling = .891f;
    std::vector<float> audio[2], req, minG; std::size_t w = 0; double sum = 0.0;
};
}
