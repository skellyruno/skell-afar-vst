#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include "dsp/SceneDSP.h"

class SkellAfarAudioProcessor final : public juce::AudioProcessor {
public:
    SkellAfarAudioProcessor();
    ~SkellAfarAudioProcessor() override = default;
    void prepareToPlay(double sampleRate,int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override {return true;}
    const juce::String getName() const override {return "Skell-Afar";}
    bool acceptsMidi() const override {return false;}
    bool producesMidi() const override {return false;}
    bool isMidiEffect() const override {return false;}
    double getTailLengthSeconds() const override {return 7.0;}
    int getNumPrograms() override {return 1;}
    int getCurrentProgram() override {return 0;}
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override {return {};}
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState parameters;
    // Non-automatable editor state and future custom scene documents.
    juce::ValueTree editorState {"EditorState"};
    std::atomic<float> meterPeak {0.f}, meterClarity {0.8f}, meterDrr {0.f};
    std::atomic<float> meterDistance {3.f}, meterEffort {.5f};
private:
    float get(const char* key) const noexcept { return parameters.getRawParameterValue(key)->load(); }
    afar::SceneDSP engine;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> effortSmooth, mixSmooth, bypassSmooth;
    // The distance parameter is logarithmic; smoothing in metres produces a harsh
    // near-field sweep. Multiplicative smoothing tracks musical distance instead.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> distanceSmooth;
    double sampleRateHz=48000.,walkBeat=0.;
    int lastScene=-1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkellAfarAudioProcessor)
};
