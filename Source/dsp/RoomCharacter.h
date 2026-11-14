#pragma once
// Per-space acoustic "personality" layered on the shared FDN: periodic flutter echo between parallel
// walls, pipe/box resonances, low/high tilt of the room field, late-field pre-delay and shimmer, and a
// distinct far-wall slap echo. Allocation happens only in prepare(); process() is realtime-safe.
#include "SceneModel.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace afar {
class RoomCharacter {
public:
    struct Out { float left = 0.f, right = 0.f; };

    void prepare(double sampleRate) {
        sr = std::clamp(sampleRate, 8000.0, 384000.0);
        for (auto& b : pre) b.prepare(static_cast<std::size_t>(sr * .2) + 8);
        for (auto& b : flut) b.prepare(static_cast<std::size_t>(sr * .4) + 8);
        for (auto& b : dry) b.prepare(static_cast<std::size_t>(sr * 1.2) + 8);
        const double twoPi = 2.0 * 3.141592653589793;
        pole = static_cast<float>(1.0 - std::exp(-1.0 / (sr * .012)));
        lowA = static_cast<float>(1.0 - std::exp(-twoPi * 220.0 / sr));
        highA = static_cast<float>(1.0 - std::exp(-twoPi * std::min(4000.0, sr * .2) / sr));
        flutA = static_cast<float>(1.0 - std::exp(-twoPi * std::min(5500.0, sr * .22) / sr));
        slapA = static_cast<float>(1.0 - std::exp(-twoPi * 1500.0 / sr));
        reset();
    }
    void reset() noexcept {
        for (auto& b : pre) b.reset();
        for (auto& b : flut) b.reset();
        for (auto& b : dry) b.reset();
        flutLp = {}; lowLp = {}; hiLp = {}; slapLp = {};
        for (auto& row : svfLow) row = {};
        for (auto& row : svfBand) row = {};
        flutDelayT = flutDelaySm = 1.f; flutFbT = flutFbSm = 0.f;
        resF = {0.f, 0.f}; resFT = {0.f, 0.f}; resGT = resGSm = 0.f;
        lowT = lowSm = highT = highSm = 0.f;
        preT = preSm = 0.f; modT = modSm = 0.f; modPhase = 0.f;
        slapDelayT = slapDelaySm = 1.f; slapGT = slapGSm = 0.f;
    }
    // Control-rate target update (cheap; call every few dozen samples).
    void setTarget(const SpaceCharacter& c) noexcept {
        const float s = static_cast<float>(sr);
        if (c.flutterMs > 0.f) flutDelayT = std::clamp(c.flutterMs * .001f * s, 2.f, s * .38f);
        flutFbT = std::clamp(c.flutterFb, 0.f, .78f);
        if (c.resonHz1 > 0.f) {
            resFT[0] = 2.f * std::sin(pi * std::min(c.resonHz1, s * .2f) / s);
            resFT[1] = 2.f * std::sin(pi * std::min(c.resonHz2 > 0.f ? c.resonHz2 : c.resonHz1 * 2.f, s * .2f) / s);
        }
        resGT = std::max(0.f, std::pow(10.f, c.resonDb / 20.f) - 1.f);
        lowT = c.lowDb; highT = c.highDb;
        preT = std::clamp(c.preMs * .001f * s, 0.f, s * .15f);
        modT = std::clamp(c.mod, 0.f, 1.f);
        if (c.slapMs > 0.f) slapDelayT = std::clamp(c.slapMs * .001f * s, 2.f, s * 1.15f);
        slapGT = std::clamp(c.slapGain, 0.f, 1.f);
    }
    // dry: un-attenuated voice excitation; early/late: scene reflection and FDN fields.
    // reflect: scene reflectivity (already audibility-gated); gate: audibility gate for the slap echo.
    Out process(float dryL, float dryR, float earlyL, float earlyR, float lateL, float lateR,
                float reflect, float gate) noexcept {
        flutDelaySm += pole * (flutDelayT - flutDelaySm);
        flutFbSm += pole * (flutFbT - flutFbSm);
        resF[0] += pole * (resFT[0] - resF[0]);
        resF[1] += pole * (resFT[1] - resF[1]);
        resGSm += pole * (resGT - resGSm);
        lowSm += pole * (lowT - lowSm);
        highSm += pole * (highT - highSm);
        preSm += pole * (preT - preSm);
        modSm += pole * (modT - modSm);
        slapDelaySm += pole * (slapDelayT - slapDelaySm);
        slapGSm += pole * (slapGT - slapGSm);
        modPhase += 2.f * pi * .31f / static_cast<float>(sr);
        if (modPhase > 2.f * pi) modPhase -= 2.f * pi;
        const float modSamples = modSm * static_cast<float>(sr) * .0015f * std::sin(modPhase);
        const float d[2] {dryL, dryR}, e[2] {earlyL, earlyR}, l[2] {lateL, lateR};
        const float flutMix = std::min(1.f, flutFbSm * 2.f);
        // Features that are off (target and smoothed value ~0) are skipped; their buffers/states are
        // kept clean so nothing stale plays when they fade in.
        const bool useFlut = flutFbSm > 1.0e-4f || flutFbT > 1.0e-4f;
        const bool useReso = resGSm > 1.0e-4f || resGT > 1.0e-4f;
        const bool useSlap = slapGSm > 1.0e-4f || slapGT > 1.0e-4f;
        const bool usePre = preSm > .5f || preT > .5f || modSm > 1.0e-3f || modT > 1.0e-3f;
        float out[2] {};
        for (int c = 0; c < 2; ++c) {
            dry[c].push(d[c]);
            pre[c].push(l[c]);
            const float late = usePre ? pre[c].tap(std::max(1.f, preSm + modSamples + 2.f)) : l[c];
            float room = e[c] + late;
            // periodic flutter between parallel walls
            if (useFlut) {
                const float fl = flut[c].tap(flutDelaySm);
                flutLp[c] += flutA * (fl - flutLp[c]);
                flut[c].push(room + d[c] * reflect * .6f + flutFbSm * flutLp[c]);
                room += fl * flutMix;
            } else {
                flut[c].push(0.f);
                flutLp[c] = 0.f;
            }
            // two band resonances (pipe / box / shaft modes), unity-peak band-pass added on top
            if (useReso) {
                float reso = 0.f;
                for (int k = 0; k < 2; ++k) {
                    svfLow[k][c] += resF[k] * svfBand[k][c];
                    const float high = room - svfLow[k][c] - resQ * svfBand[k][c];
                    svfBand[k][c] += resF[k] * high;
                    reso += svfBand[k][c];
                }
                room += resGSm * resQ * reso;
            } else {
                svfLow[0][c] = svfLow[1][c] = svfBand[0][c] = svfBand[1][c] = 0.f;
            }
            // tilt of the room field
            lowLp[c] += lowA * (room - lowLp[c]);
            room += lowSm * lowLp[c];
            hiLp[c] += highA * (room - hiLp[c]);
            room += highSm * (room - hiLp[c]);
            // distinct far-wall slap, darkened
            if (useSlap) {
                slapLp[c] += slapA * (dry[c].tap(slapDelaySm) - slapLp[c]);
                room += slapLp[c] * slapGSm * gate;
            } else {
                slapLp[c] = 0.f;
            }
            dry[c].advance(); pre[c].advance(); flut[c].advance();
            out[c] = std::isfinite(room) ? std::clamp(room, -16.f, 16.f) : 0.f;
        }
        return {out[0], out[1]};
    }

private:
    struct Delay {
        void prepare(std::size_t n) { data.assign(std::max<std::size_t>(4, n), 0.f); w = 0; }
        void reset() noexcept { std::fill(data.begin(), data.end(), 0.f); w = 0; }
        void push(float v) noexcept { data[w] = std::isfinite(v) ? v : 0.f; }
        float tap(float delay) const noexcept {
            const float n = static_cast<float>(data.size());
            float p = static_cast<float>(w) - std::clamp(delay, 0.f, n - 3.f);
            if (p < 0.f) p += n;
            if (p >= n) p -= n;   // p + n can round up to exactly n in float
            const auto i = static_cast<std::size_t>(p) < data.size() ? static_cast<std::size_t>(p) : 0;
            const auto j = i + 1 == data.size() ? 0 : i + 1;
            const float f = p - static_cast<float>(i);
            return data[i] * (1.f - f) + data[j] * f;
        }
        void advance() noexcept { if (++w == data.size()) w = 0; }
        std::vector<float> data; std::size_t w = 0;
    };
    static constexpr float resQ = .125f;   // 1/Q, Q = 8
    double sr = 48000.0;
    Delay pre[2], flut[2], dry[2];
    float pole = .01f, lowA = .03f, highA = .4f, flutA = .5f, slapA = .2f;
    std::array<float, 2> flutLp {}, lowLp {}, hiLp {}, slapLp {};
    std::array<float, 2> svfLow[2] {}, svfBand[2] {};
    std::array<float, 2> resF {0.f, 0.f}, resFT {0.f, 0.f};
    float flutDelayT = 1.f, flutDelaySm = 1.f, flutFbT = 0.f, flutFbSm = 0.f;
    float resGT = 0.f, resGSm = 0.f, lowT = 0.f, lowSm = 0.f, highT = 0.f, highSm = 0.f;
    float preT = 0.f, preSm = 0.f, modT = 0.f, modSm = 0.f, modPhase = 0.f;
    float slapDelayT = 1.f, slapDelaySm = 1.f, slapGT = 0.f, slapGSm = 0.f;
};
}
