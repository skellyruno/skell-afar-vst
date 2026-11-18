#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../Source/dsp/SceneDSP.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <vector>

namespace {
constexpr double kPi = 3.1415926535897932384626433832795;
constexpr int kReferenceTaps = 64;
constexpr int kReferencePhases = 16;

struct ReferenceTruePeak {
    std::array<std::array<double, kReferenceTaps>, kReferencePhases> coefficients {};

    ReferenceTruePeak() {
        for (int phase = 0; phase < kReferencePhases; ++phase) {
            const double fraction = static_cast<double>(phase) / kReferencePhases;
            double sum = 0.0;
            for (int tap = 0; tap < kReferenceTaps; ++tap) {
                const double offset = static_cast<double>(tap - (kReferenceTaps / 2 - 1)) - fraction;
                const double sinc = std::abs(offset) < 1.0e-12
                    ? 1.0 : std::sin(kPi * offset) / (kPi * offset);
                const double angle = 2.0 * kPi * tap / (kReferenceTaps - 1);
                const double window = 0.42 - 0.5 * std::cos(angle) + 0.08 * std::cos(2.0 * angle);
                coefficients[static_cast<std::size_t>(phase)][static_cast<std::size_t>(tap)] = sinc * window;
                sum += sinc * window;
            }
            for (double& value : coefficients[static_cast<std::size_t>(phase)])
                value /= sum;
        }
    }

    double measure(const std::vector<float>& samples) const {
        if (samples.size() < static_cast<std::size_t>(kReferenceTaps)) return 0.0;
        double peak = 0.0;
        const int firstCenter = kReferenceTaps / 2 - 1;
        const int lastCenter = static_cast<int>(samples.size()) - kReferenceTaps / 2 - 1;
        for (int center = firstCenter; center <= lastCenter; ++center) {
            const int firstSample = center - (kReferenceTaps / 2 - 1);
            for (int phase = 0; phase < kReferencePhases; ++phase) {
                double value = 0.0;
                const auto& coeff = coefficients[static_cast<std::size_t>(phase)];
                for (int tap = 0; tap < kReferenceTaps; ++tap)
                    value += coeff[static_cast<std::size_t>(tap)] *
                             samples[static_cast<std::size_t>(firstSample + tap)];
                peak = std::max(peak, std::abs(value));
            }
        }
        return peak;
    }
};

bool runCase(int sampleRate, double frequency, int quality,
             const ReferenceTruePeak& reference) {
    afar::SceneDSP dsp;
    dsp.prepare(static_cast<double>(sampleRate), 2, quality);
    const int latency = dsp.latencySamples();
    const int expectedLatency = std::max(48, static_cast<int>(sampleRate * 0.003));
    if (latency != expectedLatency) {
        std::cerr << "Afar PDC changed: " << sampleRate << " Hz, got "
                  << latency << ", expected " << expectedLatency << " samples\n";
        return false;
    }

    // Fully bypass Afar's creative path while retaining its real final limiter and
    // sample guard. This isolates the release-critical output ceiling integration.
    afar::SceneControls controls;
    controls.scene = 5;
    controls.effort = 0.5f;
    controls.recordedAs = 2;
    controls.distance = 1.0f;
    controls.removeDelay = true;
    controls.bypassEffort = true;
    controls.muteAmbience = true;
    controls.ambienceDb = -60.0f;
    controls.levelMatch = 0.0f;
    controls.outputDb = 0.0f;
    controls.throatTexture = 0.0f;
    controls.presence = 0.0f;
    controls.width = 0.0f;
    controls.beds.fill(-60.0f);

    constexpr int inputMilliseconds = 200;
    const int inputSamples = sampleRate * inputMilliseconds / 1000;
    const int outputSamples = inputSamples + latency + kReferenceTaps;
    std::vector<float> output(static_cast<std::size_t>(outputSamples), 0.0f);
    double samplePeak = 0.0;

    for (int i = 0; i < outputSamples; ++i) {
        float sample = 0.0f;
        if (i < inputSamples) {
            sample = static_cast<float>(1.2 * std::sin(2.0 * kPi * frequency * i / sampleRate));
            if (i % 887 == 0) sample = 3.0f;
        }
        const auto frame = dsp.processFrame({sample, sample}, controls, 0.0f, 1.0f);
        if (!std::isfinite(frame.left) || !std::isfinite(frame.right)) {
            std::cerr << "Non-finite Afar output: " << sampleRate << " Hz, " << frequency << " Hz\n";
            return false;
        }
        if (frame.left != frame.right) {
            std::cerr << "Stereo mismatch: " << sampleRate << " Hz, " << frequency << " Hz\n";
            return false;
        }
        output[static_cast<std::size_t>(i)] = frame.left;
        samplePeak = std::max(samplePeak, std::abs(static_cast<double>(frame.left)));
    }

    const double ceiling = std::pow(10.0, -1.0 / 20.0);
    if (samplePeak > ceiling * 1.00002) {
        std::cerr << "Afar sample peak exceeded -1 dBFS: " << sampleRate
                  << " Hz, " << frequency << " Hz, peak " << samplePeak << "\n";
        return false;
    }

    const double reconstructedPeak = reference.measure(output);
    const double overshootDb = 20.0 * std::log10(reconstructedPeak / ceiling);
    const double allowedOvershootDb = quality == 1 ? 0.04 : 0.12;
    if (!std::isfinite(overshootDb) || overshootDb > allowedOvershootDb) {
        std::cerr << "Afar inter-sample regression: " << sampleRate << " Hz, "
                  << frequency << " Hz, quality "
                  << (quality == 1 ? "Standard" : "Economy")
                  << ", overshoot " << overshootDb << " dB, allowed "
                  << allowedOvershootDb << " dB\n";
        return false;
    }

    std::cout << sampleRate << " Hz / " << frequency << " Hz / "
              << (quality == 1 ? "Standard" : "Economy")
              << ": Afar reconstructed overshoot " << overshootDb << " dB; latency "
              << latency << " samples\n";
    return true;
}
} // namespace

int main() {
    const ReferenceTruePeak reference;
    for (int sampleRate : {44100, 48000, 96000}) {
        const std::array<double, 3> frequencies {{
            7000.0, 13517.0, 0.435 * static_cast<double>(sampleRate)
        }};
        for (double frequency : frequencies) {
            for (int quality : {0, 1}) {
                if (!runCase(sampleRate, frequency, quality, reference)) return 1;
            }
        }
    }
    std::cout << "PASS: Afar real-SkellCore limiter integration matrix\n";
    std::cout << "NOTE: regression budgets are not BS.1770 certification.\n";
    return 0;
}
