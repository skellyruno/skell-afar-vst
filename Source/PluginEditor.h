#pragma once
#include <JuceHeader.h>
#include <memory>
#if __has_include(<SkellCore/GuidedTour.h>)
  #include <SkellCore/GuidedTour.h>
  #define SKELL_AFAR_GUIDED_TOUR 1
#else
  #define SKELL_AFAR_GUIDED_TOUR 0
#endif
#include "PluginProcessor.h"
#include "ui/Controls.h"
#include "ui/SpaceBrowser.h"
#include "ui/SceneView.h"

class SkellAfarAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit SkellAfarAudioProcessorEditor(SkellAfarAudioProcessor&);
    ~SkellAfarAudioProcessorEditor() override;
    void resized() override;
    void dismissTourForScreenshot();
    bool replayTourForScreenshot();
private:
    class Surface;
    SkellAfarAudioProcessor& processor;
    afar::ui::AfarLookAndFeel look;
    std::unique_ptr<Surface> surface;
#if SKELL_AFAR_GUIDED_TOUR
    std::unique_ptr<skellcore::ui::GuidedTourController> guidedTour;
#endif
    void showGuidedTour(bool firstRun);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkellAfarAudioProcessorEditor)
};
