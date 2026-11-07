#pragma once
#include <JuceHeader.h>
#include <memory>
#include "PluginProcessor.h"
#include "ui/Controls.h"
#include "ui/SpaceBrowser.h"
#include "ui/SceneView.h"

class SkellAfarAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit SkellAfarAudioProcessorEditor(SkellAfarAudioProcessor&);
    ~SkellAfarAudioProcessorEditor() override;
    void resized() override;
private:
    class Surface;
    SkellAfarAudioProcessor& processor;
    std::unique_ptr<Surface> surface;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkellAfarAudioProcessorEditor)
};
