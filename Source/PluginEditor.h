#pragma once
#include <JuceHeader.h>
#include <memory>
#include "PluginProcessor.h"
#include "ui/Controls.h"
#include "ui/SpaceBrowser.h"
#include "ui/SceneView.h"
#include <SkellCore/GuidedTour.h>

class SkellAfarAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit SkellAfarAudioProcessorEditor(SkellAfarAudioProcessor&);
    ~SkellAfarAudioProcessorEditor() override;
    void resized() override;
    void dismissTourForScreenshot() { if(guidedTour)guidedTour->dismiss(); }
    bool replayTourForScreenshot() {
        showGuidedTour(false);
        return guidedTour && guidedTour->isShowing();
    }
private:
    class Surface;
    SkellAfarAudioProcessor& processor;
    afar::ui::AfarLookAndFeel look;
    std::unique_ptr<Surface> surface;
    std::unique_ptr<skellcore::ui::GuidedTourController> guidedTour;
    void showGuidedTour(bool firstRun);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkellAfarAudioProcessorEditor)
};
