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
    assert(afar::airTrebleGain(.3f,1.f,1.f)==1.f);
    assert(afar::airTrebleGain(200.f,0.f,1.f)==1.f);
    assert(afar::airTrebleGain(100.f,1.f,1.f)<afar::airTrebleGain(10.f,1.f,1.f));
    assert(afar::airTrebleGain(200.f,1.f,1.3f)>.24f);
    const auto tunnel=afar::reflectionWeights(afar::getSpace(0).scatter);
    const auto forest=afar::reflectionWeights(afar::getSpace(6).scatter);
    assert(tunnel[0]>forest[0] && forest[2]>tunnel[2]);
    for(const auto& scene:afar::spaces) {
        const auto weights=afar::reflectionWeights(scene.scatter);
        assert(std::abs(weights[0]+weights[1]+weights[2]-1.f)<1.e-6f);
    }
    assert(afar::roomDistanceGain(3.f,true)==1.f);
    assert(afar::roomDistanceGain(200.f,true)<afar::roomDistanceGain(200.f,false));
    std::cout << "20 scene definitions; logarithmic distance; 6.02 dB/doubling; echo geometry OK\n";
}
