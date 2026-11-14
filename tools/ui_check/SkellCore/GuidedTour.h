#pragma once
// Local structural-only JUCE mock used by tools/ui_check/check.sh.
// This is NOT the production SkellCore Guided Tour implementation.
#include <JuceHeader.h>
#include <vector>
namespace skellcore { namespace ui {
struct GuidedTourStep {
    template<class... Args> GuidedTourStep(Args&&...) {}
};
struct GuidedTourPalette {
    juce::Colour a,b,c,d,e;
};
class GuidedTourController {
public:
    GuidedTourController(juce::Component&, const char*, int) {}
    void setPalette(const GuidedTourPalette&) {}
    void setSteps(const std::vector<GuidedTourStep>&) {}
    void startIfFirstRun() {}
    void replay() {}
};
}}
