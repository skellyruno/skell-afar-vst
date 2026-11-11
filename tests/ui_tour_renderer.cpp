// Actual Skell Afar JUCE editor; no mockup or audio-interface dependency.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <SkellCore/HeadlessUIRenderer.h>
#include <cstdio>

int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    const auto path = juce::File::getCurrentWorkingDirectory().getChildFile(
        argc >= 3 && juce::String(argv[1])=="--output"
            ? juce::String::fromUTF8(argv[2]) : "ui-tour-screenshots");
    SkellAfarAudioProcessor processor;
    processor.prepareToPlay(48000.,512);
    SkellAfarAudioProcessorEditor editor(processor);
    editor.dismissTourForScreenshot();
    skellcore::ui::headless::Options options;
    options.project = "Skell Afar real editor guided tutorial";
    options.scenes = {
        {"compact-normal",825,525,0,0,false},
        {"compact-tour",825,525,0,1,false},
        {"default-normal",1100,700,0,0,false},
        {"default-tour",1100,700,0,1,false},
        {"large-normal",1650,1050,0,0,false},
        {"large-tour",1650,1050,0,1,false}
    };
    int tourFailures = 0;
    options.configure = [&](const skellcore::ui::headless::Scene& scene) {
        editor.dismissTourForScreenshot();
        if(scene.mode==1 && !editor.replayTourForScreenshot()) {
            std::fprintf(stderr,"Skell Afar tour did not start on real editor\\n");
            ++tourFailures;
        }
    };
    options.mustDiffer = {
        {"compact-normal","compact-tour"},
        {"default-normal","default-tour"},
        {"large-normal","large-tour"}
    };
    const auto report = skellcore::ui::headless::render(editor,path,options);
    for (const auto& err : report.errors)
        std::fprintf(stderr,"Skell Afar UI: %s\\n",err.toRawUTF8());
    editor.dismissTourForScreenshot();
    processor.releaseResources();
    return report.passed() && report.captured==6 && tourFailures==0 ? 0 : 1;
}
