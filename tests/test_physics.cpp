#include "force_asserts.h"
#include "../Source/dsp/SceneModel.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
    assert(afar::spaces.size() == 20);
    assert(std::abs(afar::distanceMeters(0.f) - .05f) < 1.e-5f);
    assert(std::abs(afar::distanceMeters(1.f) - 200.f) < .01f);
    assert(std::abs(afar::distanceMeters(afar::distanceNormalized(30.f)) - 30.f) < 0.01f);
    const float drop = 20.f * std::log10(afar::geometricDirectGain(10.f) / afar::geometricDirectGain(20.f));
    assert(std::abs(drop - 6.0206f) < .001f);
    assert(std::abs(afar::echoDelayMs(4.f) - 8.f / 343.f * 1000.f) < .001f);
    assert(afar::getSpace(5).isOpen);
    for (const auto& scene : afar::spaces) {
        assert(scene.rt60Seconds > 0.f && scene.widthMeters > 0.f);
        assert(std::isfinite(afar::directReverbDb(scene, 20.f)));
    }
    std::cout << "20 scene definitions; logarithmic distance; 6.02 dB/doubling; echo geometry OK\n";
}
