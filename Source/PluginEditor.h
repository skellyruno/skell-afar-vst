#pragma once
#include <JuceHeader.h>
#include <memory>
#include <SkellCore/GuidedTour.h>
#include "PluginProcessor.h"
#include "ui/Controls.h"
#include "ui/SpaceBrowser.h"
#include "ui/SceneView.h"

class SkellAfarAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit SkellAfarAudioProcessorEditor(SkellAfarAudioProcessor&);
    ~SkellAfarAudioProcessorEditor() override;
    void resized() override;
    // Used by the actual-JUCE renderer, not a mock UI.
    void dismissTourForScreenshot();
    bool replayTourForScreenshot();
private:
    class Surface;
    SkellAfarAudioProcessor& processor;
    afar::ui::AfarLookAndFeel look;
    std::unique_ptr<Surface> surface;
    std::unique_ptr<skellcore::ui::GuidedTourController> guidedTour;
    void showGuidedTour(bool firstRun);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkellAfarAudioProcessorEditor)
};
