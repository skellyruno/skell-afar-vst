#pragma once
#include "Theme.h"
#include "Controls.h"
#include "../PluginProcessor.h"
#include <array>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <memory>
#include <vector>

namespace afar::ui {
// Overlay with every parameter that has no place on the main surface: the eight independent
// ambience beds, air model, occlusion material, mic stereo pattern, routing toggles and engine options.
class DetailPanel final : public juce::Component {
public:
    std::function<void()> onClose;

    explicit DetailPanel(SkellAfarAudioProcessor& p) : processor(p) {
        setOpaque(true);
        static const char* bedTitles[8] {"WIND","WATER","DRIPS","TRAFFIC","ROOM","INSECTS","FOREST","DRONE"};
        static const char* bedTips[8] {
            "Wind bed. AUTO follows the scene; raise to add wind to any space.",
            "Water flow bed (sewer, pool).",
            "Procedural drips (sewer, cave, tunnel).",
            "Distant traffic-like rumble.",
            "Room tone: air hiss and a quiet 60 Hz hum.",
            "Insect chirps (night outdoor scenes).",
            "Forest rustle and wind in leaves.",
            "Low atmospheric drone."};
        for(std::size_t i=0;i<beds.size();++i)
            setupKnob(beds[i],bedTitles[i],"bed"+juce::String(static_cast<int>(i)+1)+"_level",Kind::Bed,bedTips[i]);
        setupKnob(air[0],"TEMP","temperature",Kind::Temp,"Air temperature: changes the speed of sound and the arrival delay.");
        setupKnob(air[1],"HUMIDITY","humidity",Kind::Humidity,"Relative humidity: changes high-frequency air absorption.");
        setupKnob(air[2],"ABSORPTION","airAbsorption",Kind::Absorption,"How strongly distance rolls off the highs (0 = none).");
        setupKnob(seed,"SEED","seed",Kind::Seed,"Ambience noise seed. With LOCK SEED on, renders repeat exactly.");
        setupKnob(walkScale,"WALK SCALE","walkScale",Kind::Scale,"Scales how far the Walk path travels.");
        setupCombo(occlusion,{"OPEN","DOORWAY","WALL","THICK WALL","FOLIAGE","WATER"},"occlusionType","Material between voice and mic (used with the OCCLUSION knob).");
        setupCombo(micStereo,{"MONO","XY","ORTF","BINAURAL"},"micStereo","Stereo pickup pattern of the virtual microphone.");
        setupCombo(quality,{"ECO","NORMAL","HIGH"},"quality","Engine quality. Applied when the plugin is (re)initialised by the host.");
        setupCombo(oversampling,{"OFF","2X","4X"},"oversampling","Reserved: not yet used by the DSP.");
        setupToggle(delta,"DELTA","delta","Listen to only what the processing removes (difference signal).");
        setupToggle(bypassEffort,"BYPASS EFFORT TIMBRE","bypassEffort","Keep the Effort slider's level/distance behaviour but skip its timbre changes.");
        setupToggle(walkSync,"WALK SYNC","walkSync","Follow the host's beat position. Off: free-running from playback start.");
        setupToggle(duck,"DUCK AMB UNDER VOICE","ambienceDuck","Lower ambience while the voice is loud.");
        setupToggle(lockSeed,"LOCK SEED","seedLock","Keep the same ambience noise on every render.");
        closeButton.setButtonText("CLOSE");
        addAndMakeVisible(closeButton);
        closeButton.onClick=[this]{ if(onClose) onClose(); };
    }

    void paint(juce::Graphics& g) override {
        const auto b=getLocalBounds().toFloat();
        g.fillAll(Theme::background);
        panel(g,b.reduced(1.f));
        sectionTitle(g,"DETAIL & ENGINE",20.f,15.f,320.f,15.f);
        panel(g,{16.f,52.f,500.f,190.f},8.f);
        panel(g,{528.f,52.f,230.f,190.f},8.f);
        panel(g,{16.f,254.f,742.f,222.f},8.f);
        sectionTitle(g,"AMBIENCE BEDS",28.f,60.f,300.f,12.f);
        sectionTitle(g,"AIR",540.f,60.f,150.f,12.f);
        sectionTitle(g,"ENGINE & ROUTING",28.f,262.f,300.f,12.f);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawFittedText("AUTO follows the scene. Turn a bed up to add it to any space, down to trim it; OFF mutes it. Master level is the AMBIENCE knob.",
                         28,204,480,30,juce::Justification::topLeft,2);
        g.drawFittedText("Temperature and humidity set air absorption and sound speed.",540,204,210,30,juce::Justification::topLeft,2);
        const char* captions[4] {"OCCLUSION MATERIAL","MIC STEREO PATTERN","QUALITY","OVERSAMPLING"};
        const int xs[4] {28,210,392,574};
        for(int i=0;i<4;++i) g.drawText(captions[i],xs[i],290,170,12,juce::Justification::centredLeft);
        g.drawFittedText("QUALITY and OVERSAMPLING are reserved: the scene engine has no oversampled stage yet "
                         "(Throat Texture already uses anti-aliased saturation). SEED + LOCK SEED make ambience renders repeat exactly.",
                         350,384,390,64,juce::Justification::topLeft,5);
    }

    void resized() override {
        closeButton.setBounds(getWidth()-92,12,76,26);
        for(std::size_t i=0;i<beds.size();++i) place(beds[i],28+static_cast<int>(i)*60,86,58);
        for(std::size_t i=0;i<air.size();++i) place(air[i],538+static_cast<int>(i)*72,86,68);
        occlusion.setBounds(28,304,170,28);micStereo.setBounds(210,304,170,28);
        quality.setBounds(392,304,170,28);oversampling.setBounds(574,304,170,28);
        delta.setBounds(28,342,150,24);bypassEffort.setBounds(190,342,200,24);
        walkSync.setBounds(400,342,130,24);duck.setBounds(540,342,200,24);
        place(seed,28,376,64);lockSeed.setBounds(100,398,130,24);place(walkScale,260,376,64);
    }

private:
    enum class Kind { Bed, Temp, Humidity, Absorption, Seed, Scale };
    struct Knob { juce::Slider slider; juce::Label label; };

    static void place(Knob& k,int x,int y,int w) {
        k.slider.setBounds(x,y,w,82);
        k.label.setBounds(x-2,y+84,w+4,12);
    }
    void setupKnob(Knob& k,const char* title,const juce::String& id,Kind kind,const char* tip) {
        k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,64,16);
        k.slider.setTooltip(tip);
        k.label.setText(title,juce::dontSendNotification);
        k.label.setJustificationType(juce::Justification::centred);
        k.label.setFont(juce::Font(juce::FontOptions(9.2f)));
        k.label.setColour(juce::Label::textColourId,Theme::muted);
        addAndMakeVisible(k.slider);addAndMakeVisible(k.label);
        sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,id,k.slider));
        formatKnob(k.slider,kind);   // after attaching: the attachment resets the text functions
    }
    static void formatKnob(juce::Slider& s,Kind kind) {
        s.textFromValueFunction=[kind](double v)->juce::String {
            switch(kind) {
                case Kind::Bed: {
                    if(v<=-59.9) return "OFF";
                    const double rel=v+20.0;
                    if(std::abs(rel)<.05) return "AUTO";
                    return juce::String(rel>0.0?"+":"")+juce::String(rel,1)+" dB";
                }
                case Kind::Temp: return juce::String(v,0)+juce::String::fromUTF8("\xc2\xb0")+"C";
                case Kind::Humidity: return juce::String(v,0)+"%";
                case Kind::Absorption: return juce::String(v*100.0,0)+"%";
                case Kind::Seed: return juce::String(static_cast<int>(std::lround(v)));
                case Kind::Scale: return juce::String(v,2)+"x";
            }
            return {};
        };
        s.valueFromTextFunction=[kind](const juce::String& t)->double {
            const auto u=t.trim().toUpperCase();
            const double x=static_cast<double>(u.retainCharacters("0123456789.-+").getFloatValue());
            if(kind==Kind::Bed) {
                if(u=="OFF" || u.startsWith("-INF")) return -60.0;
                if(u=="AUTO") return -20.0;
                return x-20.0;   // typed as dB relative to AUTO
            }
            return kind==Kind::Absorption ? x/100.0 : x;
        };
        s.updateText();
    }
    void setupCombo(juce::ComboBox& box,std::initializer_list<const char*> items,const char* id,const char* tip) {
        for(auto* name:items) box.addItem(name,box.getNumItems()+1);
        box.setTooltip(tip);
        addAndMakeVisible(box);
        boxAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.parameters,id,box));
    }
    void setupToggle(juce::ToggleButton& t,const char* text,const char* id,const char* tip) {
        t.setButtonText(text);t.setTooltip(tip);
        addAndMakeVisible(t);
        buttonAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,id,t));
    }

    SkellAfarAudioProcessor& processor;
    std::array<Knob,8> beds;
    std::array<Knob,3> air;
    Knob seed,walkScale;
    juce::ComboBox occlusion,micStereo,quality,oversampling;
    juce::ToggleButton delta,bypassEffort,walkSync,duck,lockSeed;
    juce::TextButton closeButton;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> boxAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;
};
}
