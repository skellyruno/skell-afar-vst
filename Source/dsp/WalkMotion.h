#pragma once
// Transport-tempo following walk envelope. No allocations, locks or host calls on the audio thread.
// Unlike a one-way sawtooth, each motion returns smoothly at its loop boundary.
#include <algorithm>
#include <cmath>

namespace afar {
class WalkMotion {
public:
    void prepare(double sampleRate) noexcept {
        sr=std::clamp(sampleRate, 8000.0, 384000.0);
        levelPole=static_cast<float>(1.0-std::exp(-1.0/(sr*.11))); // 110 ms enable/disable ramp
        pathPole=static_cast<float>(1.0-std::exp(-1.0/(sr*.016))); // 16 ms anti-zipper filter
        distancePole=static_cast<float>(1.0-std::exp(-1.0/(sr*.065))); // 65 ms logarithmic glide
        reset();
    }
    void reset() noexcept {
        phaseBeats=0.0; enabled=false; level=0.f; smoothedPath=targetPath=1.f;
        smoothDistanceLog=std::log(3.f); validDistance=false; samplesUntilUpdate=0;
    }
    void beginBlock(bool on, bool hostTempoSync, double hostBpm) noexcept {
        if (on && !enabled) { phaseBeats=0.0; samplesUntilUpdate=0; }
        enabled=on;
        bpm=(hostTempoSync && std::isfinite(hostBpm)) ? std::clamp(hostBpm,20.0,400.0) : 120.0;
    }
    float next(float baseDistance, int path, float lengthBars, float scale) noexcept {
        baseDistance=std::clamp(baseDistance,.05f,200.f);
        const float length=std::clamp(lengthBars,1.f,16.f);
        level+=levelPole*((enabled?1.f:0.f)-level);
        if(enabled) {
            if(samplesUntilUpdate--<=0) {
                const float phase=static_cast<float>(phaseBeats/(4.0*static_cast<double>(length)));
                targetPath=shape(path,phase);
                samplesUntilUpdate=31;
            }
            smoothedPath+=pathPole*(targetPath-smoothedPath);
            phaseBeats+=bpm/(60.0*sr);
            const double period=4.0*static_cast<double>(length);
            if(phaseBeats>=period) phaseBeats-=period;
        }
        const float travelled=std::clamp(.3f+smoothedPath*std::max(.2f,baseDistance)*6.f*
                                                 std::clamp(scale,.1f,2.f),.05f,200.f);
        const float desiredLog=(1.f-level)*std::log(baseDistance)+level*std::log(travelled);
        if(!validDistance) { smoothDistanceLog=std::log(baseDistance); validDistance=true; }
        smoothDistanceLog+=distancePole*(desiredLog-smoothDistanceLog);
        return std::clamp(std::exp(smoothDistanceLog),.05f,200.f);
    }
    static float shape(int path, float phase) noexcept {
        // Periodic, C1-continuous even at phase=0/1. Far=1, near=0.
        phase-=std::floor(phase);
        constexpr float twoPi=6.2831853071795864769f;
        const float c=std::cos(twoPi*phase);
        switch(path) {
            case 0: return .5f+.5f*c; // Approach then smoothly return
            case 1: return .5f-.5f*c; // Retreat then smoothly return
            case 2: { // Pass-by: farther at both ends, close in the middle
                const float x=std::abs(2.f*phase-1.f);
                return x*x*(3.f-2.f*x);
            }
            case 3: return .5f+.5f*std::cos(2.f*twoPi*phase); // Pacing, twice per cycle
            default: {const float retreat=.5f-.5f*c;return retreat*retreat;} // Run away
        }
    }
private:
    double sr=48000.,bpm=120.,phaseBeats=0.;
    float levelPole=0.f,pathPole=0.f,distancePole=0.f;
    float level=0.f,smoothedPath=1.f,targetPath=1.f,smoothDistanceLog=0.f;
    int samplesUntilUpdate=0;
    bool enabled=false,validDistance=false;
};
} // namespace afar
