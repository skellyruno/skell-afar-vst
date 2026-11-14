#include "PluginEditor.h"
#include <SkellCore/UI.h>
#include "ui/DetailPanel.h"
#include <BinaryData.h>
#include <array>
#include <functional>
#include <memory>

namespace {
struct Preset {
    const char* name; int scene; float effort,distance,ambience; int mic;
    float angle,occlusion; int occlusionType; float size,presence,texture;   // texture < 0: keep the user's setting
};
constexpr std::array<Preset,45> factory {{
    {"INIT / Natural 3 m",5,0.5f,3.f,-30.f,7,0.f,0.f,0,1.f,0.f,0.f},
    {"Shout Down The Tunnel",0,0.92f,25.f,-25.f,7,0.f,0.f,0,1.f,0.35f,0.65f},
    {"Whisper From Dark End",0,0.1f,80.f,-17.f,0,0.f,0.f,0,1.3f,0.85f,0.33f},
    {"Passing Car Voice",0,0.73f,24.f,-18.f,0,60.f,0.f,0,1.f,0.2f,0.f},
    {"Whisper In The Pipe",1,0.12f,19.f,-15.f,0,0.f,0.f,0,0.8f,0.7f,0.33f},
    {"Drip Call",1,0.61f,10.f,-10.f,1,25.f,0.f,0,1.f,0.2f,0.1f},
    {"Subway Announcement",2,0.76f,30.f,-10.f,1,0.f,0.f,0,1.f,0.3f,0.f},
    {"Cave Echo Cry",3,0.91f,27.f,-23.f,7,0.f,0.f,0,1.2f,0.2f,0.7f},
    {"Bunker Radio",4,0.62f,6.f,-24.f,6,0.f,0.f,0,1.f,0.1f,0.2f},
    {"Graveyard Call",5,0.83f,43.f,-17.f,7,0.f,0.f,0,1.f,0.3f,0.5f},
    {"Midnight Whisper",5,0.09f,12.f,-16.f,0,10.f,0.f,0,1.f,0.5f,0.33f},
    {"Stone Echo",5,0.67f,35.f,-22.f,7,90.f,0.f,0,1.f,0.f,0.f},
    {"Forest Whisper 30 m",6,0.11f,30.f,-15.f,7,0.f,0.3f,4,1.f,0.8f,0.33f},
    {"Lost In The Trees",6,0.65f,90.f,-18.f,1,120.f,0.5f,4,1.4f,0.4f,0.f},
    {"Canyon Shout",7,1.f,130.f,-21.f,7,0.f,0.f,0,1.f,0.4f,0.8f},
    {"Valley Answer",7,0.8f,155.f,-18.f,7,0.f,0.f,0,1.3f,0.5f,0.f},
    {"Distant Voice Memo",8,0.55f,48.f,-17.f,0,0.f,0.f,0,1.f,0.3f,0.f},
    {"Neighbor's Yard",8,0.62f,25.f,-22.f,0,30.f,0.65f,2,1.f,0.f,0.f},
    {"Footsteps Closer",9,0.65f,20.f,-19.f,7,0.f,0.f,0,1.f,0.f,0.f},
    {"Running Away",9,0.8f,30.f,-19.f,0,150.f,0.f,0,1.f,0.2f,0.15f},
    {"Far Adlib Throw",7,0.86f,69.f,-27.f,7,0.f,0.f,0,1.f,0.35f,0.f},
    {"Haunted Whisper Layer",5,0.13f,56.f,-22.f,1,0.f,0.f,0,1.2f,0.6f,0.45f},
    {"Crowd Across The Field",8,0.78f,110.f,-12.f,7,0.f,0.f,0,1.f,0.5f,0.f},
    {"Empty Pool Shout",13,0.93f,7.f,-28.f,7,0.f,0.f,0,1.f,0.f,0.7f},
    {"Locked In The Bathroom",15,0.2f,2.2f,-26.f,0,90.f,0.55f,2,1.f,0.f,0.2f},
    {"Voice Behind The Door",4,0.45f,2.5f,-30.f,0,0.f,0.8f,1,0.7f,0.f,0.f},
    {"Elevator Ghost",17,0.18f,12.f,-24.f,0,0.f,0.f,0,1.f,0.7f,0.33f},
    {"Dropped Down The Shaft",17,0.85f,35.f,-26.f,7,0.f,0.f,0,1.3f,0.2f,0.5f},
    {"Crypt Sermon",16,0.6f,14.f,-20.f,7,0.f,0.f,0,1.f,0.3f,0.2f},
    {"Catacomb Whisper",16,0.08f,18.f,-22.f,0,0.f,0.f,0,1.1f,0.9f,0.33f},
    {"Cathedral Prayer",12,0.4f,22.f,-24.f,7,0.f,0.f,0,1.2f,0.3f,0.1f},
    {"Cathedral Scream",12,1.f,15.f,-26.f,7,0.f,0.f,0,1.f,0.f,0.85f},
    {"Snowbound Whisper",18,0.12f,10.f,-28.f,0,0.f,0.f,0,1.f,0.4f,0.33f},
    {"Lost In The Blizzard",18,0.88f,160.f,-12.f,0,0.f,0.f,0,1.f,0.8f,0.4f},
    {"Arena Announcer",19,0.85f,40.f,-18.f,1,0.f,0.f,0,1.f,0.4f,0.f},
    {"Stadium Chant From Far",19,0.95f,120.f,-14.f,7,0.f,0.f,0,1.2f,0.7f,0.5f},
    {"Parking Garage Chase",10,0.78f,28.f,-22.f,7,45.f,0.f,0,1.f,0.f,0.25f},
    {"Stairwell Echo Up",11,0.55f,9.f,-24.f,7,20.f,0.f,0,1.f,0.f,0.1f},
    {"Warehouse Rehearsal",14,0.58f,14.f,-27.f,4,0.f,0.f,0,1.f,0.f,0.f},
    {"Subway Last Train",2,0.3f,60.f,-14.f,0,0.f,0.f,0,1.2f,0.5f,0.15f},
    {"Alley Argument",9,0.85f,8.f,-21.f,1,70.f,0.3f,1,1.f,0.f,0.55f},
    {"Sewer Monster Growl",1,0.95f,6.f,-16.f,7,0.f,0.f,0,0.8f,0.f,1.f},
    {"Underwater Pool Voice",13,0.35f,4.f,-24.f,0,0.f,0.7f,5,1.f,0.f,0.f},
    {"Midnight Phone Call",5,0.3f,1.5f,-30.f,0,0.f,0.f,0,1.f,0.f,0.2f},
    {"Walkie Talkie Field Radio",8,0.65f,70.f,-22.f,6,0.f,0.f,0,1.f,0.f,0.1f}
}};
void resetParam(SkellAfarAudioProcessor& processor,const char* id) {
    if(auto* p=processor.parameters.getParameter(id)) {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->getDefaultValue());
        p->endChangeGesture();
    }
}
void setParam(SkellAfarAudioProcessor& processor,const char* id,float value) {
    if(auto* p=processor.parameters.getParameter(id)) {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(value));
        p->endChangeGesture();
    }
}
}

class SkellAfarAudioProcessorEditor::Surface final : public juce::Component, private juce::Timer {
public:
    Surface(SkellAfarAudioProcessor& p,std::function<void(float)> scaleCallback,
            std::function<void()> tourCallback)
      : processor(p),onScale(std::move(scaleCallback)),onShowTour(std::move(tourCallback)),browser(),sceneView(p),scenePad(p),meters(p),detail(p) {
        look=std::make_unique<afar::ui::AfarLookAndFeel>();
        setLookAndFeel(look.get());
        bg=juce::ImageFileFormat::loadFrom(BinaryData::bg_panel_png,static_cast<std::size_t>(BinaryData::bg_panel_pngSize));
        logo=juce::ImageFileFormat::loadFrom(BinaryData::logo_skull_png,static_cast<std::size_t>(BinaryData::logo_skull_pngSize));
        addAndMakeVisible(browser);
        browser.onSelect=[this](int id){setParam(processor,"space",static_cast<float>(id));};
        browser.setSelectedSpace(static_cast<int>(processor.parameters.getRawParameterValue("space")->load()));
        addAndMakeVisible(sceneView);addAndMakeVisible(scenePad);addAndMakeVisible(meters);
        effort.setComponentID("effort");distance.setComponentID("distance");
        for(auto* slider:{&effort,&distance}) {
            slider->setSliderStyle(juce::Slider::LinearHorizontal);
            slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,76,26);
            slider->setColour(juce::Slider::textBoxTextColourId,afar::ui::Theme::bone);
            addAndMakeVisible(*slider);
        }
        effort.setTooltip("Vocal effort, whisper to shout. Changes level, breath and presence.");
        distance.setTooltip("Distance from voice to microphone, 0.05 to 200 m (logarithmic).");
        attach(effort,"effort");attach(distance,"distance");
        throatTexture.setComponentID("throatTexture");
        throatTexture.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        throatTexture.setTextBoxStyle(juce::Slider::TextBoxBelow,false,62,17);
        throatTexture.setDoubleClickReturnValue(true,0.0);
        throatTexture.setTooltip("CLEAN 0% | AIRY 33% | RASPY 67% | GRITTY 100%. Morphs source-domain texture before propagation and room effects.");
        addAndMakeVisible(throatTexture);
        attach(throatTexture,"throatTexture");
        const char* ids[]={"angle","occlusionAmount","size","ambience","presenceHold","width","mix","output"};
        const char* titles[]={"ANGLE","OCCLUSION","SIZE","AMBIENCE","PRESENCE","WIDTH","MIX","OUTPUT"};
        const char* tips[]={"Voice direction: 0 degrees faces the mic, 180 faces away.",
            "How much of the selected material (see MORE) blocks the voice.",
            "Scales room dimensions and echo timing.",
            "Master level of the ambience beds (set each bed under MORE).",
            "Presence Hold: keeps speech clarity (2-5 kHz) up at distance; eases off on loud passages.",
            "Stereo width of the processed signal.","Dry / wet balance.","Output gain before the final limiter."};
        for(int i=0;i<8;++i){
            knobs[static_cast<std::size_t>(i)].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knobs[static_cast<std::size_t>(i)].setTextBoxStyle(juce::Slider::TextBoxBelow,false,63,16);
            knobs[static_cast<std::size_t>(i)].setTooltip(tips[i]);
            knobLabels[static_cast<std::size_t>(i)].setText(titles[i],juce::dontSendNotification);
            knobLabels[static_cast<std::size_t>(i)].setJustificationType(juce::Justification::centred);
            knobLabels[static_cast<std::size_t>(i)].setFont(juce::Font(juce::FontOptions(9.6f)));
            knobLabels[static_cast<std::size_t>(i)].setColour(juce::Label::textColourId,afar::ui::Theme::muted);
            addAndMakeVisible(knobs[static_cast<std::size_t>(i)]);
            addAndMakeVisible(knobLabels[static_cast<std::size_t>(i)]);
            attach(knobs[static_cast<std::size_t>(i)],ids[i]);
        }
        setupChoice(mic,{"PHONE","HANDHELD","SHOTGUN","LAVALIER","STUDIO","DICTAPHONE","WALKIE","IDEAL"},"micType");
        setupChoice(recorded,{"WHISPER","SOFT","NORMAL","LOUD","AUTO"},"recordedAs");
        setupChoice(walkPath,{"APPROACH","RETREAT","PASS BY","PACING","RUN AWAY"},"walkPath");
        mic.setTooltip("Virtual microphone coloration (approximate, not a calibrated model of real devices).");
        recorded.setTooltip("How the voice was originally recorded; Effort compensates for it.");
        walkPath.setTooltip("Path the voice travels, in time with the host tempo (needs WALK on).");
        walkEnabled.setButtonText("WALK");addAndMakeVisible(walkEnabled);
        buttonAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"walkOn",walkEnabled));
        walkLength.setSliderStyle(juce::Slider::LinearHorizontal);
        walkLength.setTextBoxStyle(juce::Slider::TextBoxRight,false,44,22);
        addAndMakeVisible(walkLength);attach(walkLength,"walkLength");
        power.setButtonText("ON");addAndMakeVisible(power);
        buttonAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"power",power));
        delayToggle.setTooltip("Remove the physical arrival delay so the voice stays on the beat.");
        delayToggle.setButtonText("BEAT ALIGNED");addAndMakeVisible(delayToggle);
        buttonAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"removeDelay",delayToggle));
        levelMatch.setButtonText("LEVEL MATCH");addAndMakeVisible(levelMatch);
        levelMatch.setTooltip("Keeps the processed voice at the loudness of your input, so distance and effort change the perspective, not the volume (far whispers stay audible). Amount: MORE panel.");
        buttonAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"levelMatch",levelMatch));
        muteAmbience.setButtonText("MUTE AMB");addAndMakeVisible(muteAmbience);
        buttonAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"muteAmbience",muteAmbience));
        addAndMakeVisible(presetList);
        presetList.onChange=[this]{
            const int id=presetList.getSelectedId();
            if(id>=1 && id<=static_cast<int>(factory.size())) loadPreset(factory[static_cast<std::size_t>(id-1)]);
            else if(id>=1000 && id-1000<static_cast<int>(userPresetFiles.size())) loadUserPreset(userPresetFiles[static_cast<std::size_t>(id-1000)]);
        };
        refreshPresetList(processor.editorState.getProperty("preset","INIT / Natural 3 m").toString());
        prevPreset.setButtonText("<");nextPreset.setButtonText(">");
        addAndMakeVisible(prevPreset);addAndMakeVisible(nextPreset);
        prevPreset.onClick=[this]{stepPreset(-1);};
        nextPreset.onClick=[this]{stepPreset(1);};
        settings.setButtonText("SET");addAndMakeVisible(settings);
        settings.onClick=[this]{
            juce::PopupMenu m;
            m.addItem(1,juce::String::fromUTF8("Small \xc2\xb7 75%"));m.addItem(2,juce::String::fromUTF8("Medium \xc2\xb7 100%"));
            m.addItem(3,juce::String::fromUTF8("Large \xc2\xb7 125%"));m.addItem(4,juce::String::fromUTF8("XL \xc2\xb7 150%"));
            m.addSeparator();
            m.addItem(5,"Help / Show Tutorial");
            // Never capture `this` inside a nested lambda initializer: MSVC binds
            // that `this` to the outer closure, rather than to Surface.
            const juce::Component::SafePointer<Surface> safeView(this);
            m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settings),
                [safeView](int choice) {
                    if (auto* view=safeView.getComponent()) {
                        constexpr float factors[]{.75f,1.f,1.25f,1.5f};
                        if(choice>=1 && choice<=4) view->onScale(factors[choice-1]);
                        else if(choice==5 && view->onShowTour) view->onShowTour();
                    }
                });
        };
        save.setButtonText("SAVE");addAndMakeVisible(save);
        save.onClick=[this]{saveCurrentPreset();};
        randomise.setButtonText("DICE");addAndMakeVisible(randomise);
        randomise.onClick=[this]{const int ix=juce::Random::getSystemRandom().nextInt(static_cast<int>(factory.size()));
            presetList.setSelectedItemIndex(ix,juce::sendNotificationSync);};
        moreButton.setButtonText("MORE");addAndMakeVisible(moreButton);
        moreButton.setTooltip("Ambience beds, air model, routing and engine options.");
        moreButton.onClick=[this]{toggleDetail();};
        detail.onClose=[this]{toggleDetail();};
        // Keep the detail panel attached to the real JUCE component tree.
        addAndMakeVisible(detail);
        detail.setVisible(false);
        toggleAB.setTooltip("Switch between two complete plugin states (A / B).");
        save.setTooltip("Save the current settings as a named user preset.");
        randomise.setTooltip("Load a random factory preset.");
        toggleAB.setButtonText("A");addAndMakeVisible(toggleAB);
        toggleAB.onClick=[this]{
            // Two full-state slots: store the active slot, then switch (first switch clones the current state).
            slotState[activeSlot]=processor.parameters.copyState();
            const int other=1-activeSlot;
            if(!slotState[other].isValid()) slotState[other]=slotState[activeSlot].createCopy();
            processor.parameters.replaceState(slotState[other].createCopy());
            activeSlot=other;
            toggleAB.setButtonText(activeSlot==0?"A":"B");
        };
        startTimerHz(20);
    }
    std::vector<skellcore::ui::GuidedTourStep> tourSteps() {
        std::vector<skellcore::ui::GuidedTourStep> steps;
        steps.emplace_back(&presetList,"START WITH A PRESET",
            "Pick a location or sound to get started, then tailor it to your vocal.");
        steps.emplace_back(&browser,"CHOOSE A SPACE",
            "Choose where your vocal lives: each space changes its acoustic character.");
        steps.emplace_back(&effort,"VOCAL EFFORT",
            "Move from whisper to shout to set the source performance.");
        steps.emplace_back(&distance,"DISTANCE",
            "Move your voice nearer or farther from the microphone in metres.");
        steps.emplace_back(&sceneView,"VISUALIZE THE SPACE",
            "Use the visual scene and position controls to understand the environment.");
        steps.emplace_back(&moreButton,"MORE DETAILS",
            "MORE opens ambience beds, routing and fine acoustic settings.");
        steps.emplace_back(&detail,"ADVANCED SOUND DESIGN",
            "Adjust the optional detail controls, then select BACK to return.",
            [this]{if(!detail.isVisible())toggleDetail();});
        steps.emplace_back(&save,"SAVE YOUR SOUND",
            "Save the settings as a named preset. Replay this guide from SET.");
        return steps;
    }
    skellcore::ui::GuidedTourPalette tourPalette() const {
        return {afar::ui::Theme::green,afar::ui::Theme::panelRaised,
                afar::ui::Theme::bone,afar::ui::Theme::muted,
                juce::Colour(0xd9000000)};
    }
    ~Surface() override {
        stopTimer();
        setLookAndFeel(nullptr);
        // Child attachments release before controls in their containing editor.
    }
    void paint(juce::Graphics& g) override {
        using namespace afar::ui;
        g.fillAll(Theme::background);
        if(bg.isValid())g.drawImageAt(bg,0,0);
        g.setColour(Theme::panelRaised);g.fillRect(0,0,getWidth(),Theme::topBar);
        g.setColour(Theme::greenDim);g.drawHorizontalLine(Theme::topBar,0.f,static_cast<float>(getWidth()));
        g.setColour(Theme::green.withAlpha(.72f));
        g.fillRoundedRectangle(318.f,54.f,474.f,2.f,1.f);
        g.fillRoundedRectangle(825.f,54.f,247.f,2.f,1.f);
        {   // skull glow follows the output level
            const float lv=std::clamp(processor.meterPeak.load(std::memory_order_relaxed),0.f,1.f);
            juce::ColourGradient glow(Theme::green.withAlpha(.05f+.32f*lv),76.f,29.f,Theme::green.withAlpha(0.f),76.f,30.f,true);
            g.setGradientFill(glow);g.fillEllipse(46.f,0.f,60.f,57.f);
        }
        if(logo.isValid())g.drawImage(logo,53,5,47,47,0,0,96,96);
        g.setColour(Theme::greenLight);g.setFont(titleFont(21.f));
        g.drawText("SKELL-AFAR",111,7,264,29,juce::Justification::centredLeft);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText("FEEL THE DISTANCE",113,33,260,14,juce::Justification::centredLeft);
        panel(g,{314.f,417.f,489.f,144.f});
        sectionTitle(g,"EFFORT",327.f,422.f,200.f);
        {
            const float ev=processor.parameters.getRawParameterValue("effort")->load();
            const char* word=ev<.15f?"WHISPER":ev<.38f?"SOFT":ev<.62f?"NORMAL":ev<.85f?"LOUD":"SHOUT";
            g.setColour(Theme::ice.interpolatedWith(Theme::fire,ev));g.setFont(titleFont(10.f));
            g.drawText(word,392.f,422.f,90.f,19.f,juce::Justification::centredLeft);
        }
        const float target=processor.parameters.getRawParameterValue("effort")->load();
        const int as=static_cast<int>(processor.parameters.getRawParameterValue("recordedAs")->load());
        g.setColour(Theme::muted);
        g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText("~"+juce::String(afar::effortSpl(target),0)+juce::String::fromUTF8(" dB SPL @ 1 m \xc2\xb7 estimated"),560,423,224,16,juce::Justification::centredRight);
        if(std::abs(target-afar::recordedEffort(as))>.30f){
            g.setColour(Theme::fire);
            g.setFont(juce::Font(juce::FontOptions(9.f)));
            g.drawText("LARGE CHANGE: ARTICULATION MAY NOT MATCH",395,471,225,14,juce::Justification::centred);
        }
        g.setColour(Theme::ice);g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText("WHISPER",337,470,180,16,juce::Justification::centredLeft);
        g.setColour(Theme::fire);g.drawText("SHOUT",626,470,72,16,juce::Justification::centredRight);
        sectionTitle(g,"DISTANCE",327.f,490.f,200.f);
        const auto metres=processor.parameters.getRawParameterValue("distance")->load();
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText(juce::String(afar::arrivalMilliseconds(metres),1)+" ms arrival",614,491,170,16,juce::Justification::centredRight);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText("IN YOUR EAR",337,541,170,16,juce::Justification::centredLeft);
        g.setColour(Theme::violet);g.drawText("FAR AWAY",619,541,79,16,juce::Justification::centredRight);
        panel(g,{820.f,346.f,260.f,215.f});
        sectionTitle(g,"WALK PATH",834.f,355.f,140.f,14.f);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText("PATH PRESET",832,384,210,13,juce::Justification::centredLeft);
        g.drawText("LENGTH (BARS)",832,437,230,13,juce::Justification::centredLeft);
        g.setColour(Theme::greenDim.withAlpha(.6f));g.drawHorizontalLine(486,832.f,1068.f);
        {   // walk path preview: distance over time (up = farther), lit when WALK is on
            const juce::Rectangle<float> r(990.f,399.f,76.f,28.f);
            g.setColour(Theme::background);g.fillRoundedRectangle(r,5.f);
            g.setColour(Theme::greenDim);g.drawRoundedRectangle(r,5.f,1.f);
            const int wp=static_cast<int>(processor.parameters.getRawParameterValue("walkPath")->load());
            juce::Path curve;
            for(int i=0;i<=40;++i) {
                const float ph=static_cast<float>(i)/40.f;
                float v=ph;
                switch(wp) {
                    case 0: v=1.f-ph;break;
                    case 1: v=ph;break;
                    case 2: v=std::abs(2.f*ph-1.f);break;
                    case 3: v=.5f+.5f*std::sin(4.f*afar::pi*ph);break;
                    default: v=ph*ph;break;
                }
                const float px=r.getX()+6.f+(r.getWidth()-12.f)*ph,py=r.getBottom()-5.f-(r.getHeight()-10.f)*v;
                if(i==0) curve.startNewSubPath(px,py); else curve.lineTo(px,py);
            }
            g.setColour(processor.parameters.getRawParameterValue("walkOn")->load()>.5f?Theme::green:Theme::greenDim.brighter(.5f));
            g.strokePath(curve,juce::PathStrokeType(1.6f));
        }
        panel(g,{314.f,575.f,774.f,115.f});
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(9.5f)));
        g.drawText("MIC DEVICE",829,583,112,14,juce::Justification::centredLeft);
        g.drawText("RECORDED AS",955,583,116,14,juce::Justification::centredLeft);
        const float texture=processor.parameters.getRawParameterValue("throatTexture")->load();
        const juce::String textureStage=texture<.16f?"CLEAN":texture<.50f?"AIRY":texture<.83f?"RASPY":"GRITTY";
        g.setColour(Theme::greenLight);g.setFont(titleFont(10.f));
        g.drawText("THROAT TEXTURE",322,581,86,13,juce::Justification::centred);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(9.f)));
        g.drawText(textureStage,329,672,71,13,juce::Justification::centred);
        // The macro's four landmarks are intentionally printed on the control,
        // rather than adding four more knobs to a crowded bottom strip.
        const float trackX=328.f,trackY=597.f,trackW=70.f;
        const juce::Colour stages[]{Theme::bone,Theme::ice,Theme::greenLight,Theme::fire};
        for(int i=0;i<4;++i){
            g.setColour(stages[i].withAlpha(i==std::min(3,static_cast<int>(texture*3.99f))?1.f:.36f));
            g.fillRoundedRectangle(trackX+i*trackW/4.f,trackY,trackW/4.f-2.f,2.5f,1.2f);
        }
    }
    void resized() override {
        browser.setBounds(afar::ui::Theme::browserX,afar::ui::Theme::browserY,afar::ui::Theme::browserW,afar::ui::Theme::browserH);
        sceneView.setBounds(320,72,480,332);
        scenePad.setBounds(820,72,260,260);
        effort.setBounds(326,440,465,34);
        distance.setBounds(326,509,465,34);
        walkEnabled.setBounds(985,354,88,22);
        walkPath.setBounds(832,399,150,28);
        walkLength.setBounds(832,452,234,24);
        meters.setBounds(833,492,232,55);
        delayToggle.setBounds(829,638,131,22);
        muteAmbience.setBounds(966,638,106,22);
        levelMatch.setBounds(829,661,160,22);
        throatTexture.setBounds(326,599,75,72);
        for(int i=0;i<8;++i){
            const int x=408+i*50;
            knobs[static_cast<std::size_t>(i)].setBounds(x,602,49,69);
            knobLabels[static_cast<std::size_t>(i)].setBounds(x-6,672,61,12);
        }
        mic.setBounds(828,600,112,27);
        recorded.setBounds(950,600,122,27);
        settings.setBounds(10,12,37,30);
        moreButton.setBounds(858,13,78,28);
        detail.setBounds(314,72,774,489);
        prevPreset.setBounds(388,13,24,28);
        presetList.setBounds(416,13,218,28);
        nextPreset.setBounds(638,13,24,28);
        save.setBounds(672,13,58,28);
        randomise.setBounds(736,13,55,28);
        toggleAB.setBounds(798,13,50,28);
        power.setBounds(1001,15,86,27);
    }
private:
    void timerCallback() override {
        browser.setSelectedSpace(static_cast<int>(processor.parameters.getRawParameterValue("space")->load()));
        const auto& pv=processor.parameters;
        const float e=pv.getRawParameterValue("effort")->load(),d=pv.getRawParameterValue("distance")->load();
        const float t=pv.getRawParameterValue("throatTexture")->load();
        const int as=static_cast<int>(pv.getRawParameterValue("recordedAs")->load());
        if(e!=lastEffort||d!=lastDistance||as!=lastRecorded){lastEffort=e;lastDistance=d;lastRecorded=as;repaint(314,417,489,144);}
        if(t!=lastTexture){lastTexture=t;repaint(317,574,93,115);}
        const int ws=static_cast<int>(pv.getRawParameterValue("walkPath")->load())*2+(pv.getRawParameterValue("walkOn")->load()>.5f?1:0);
        if(ws!=lastWalkState){lastWalkState=ws;repaint(985,395,90,36);}
        repaint(46,0,60,57);
    }
    void toggleDetail() {
        const bool show=!detail.isVisible();
        detail.setVisible(show);
        if(show) detail.toFront(false);
        moreButton.setButtonText(show?"BACK":"MORE");
    }
    void attach(juce::Slider& slider,const char* id) {
        sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,id,slider));
        formatSlider(slider,id); // after attaching: the attachment resets text functions
    }
    // Human-readable value text (was raw floats such as 3.0000017 / 0.0000000, truncated in the box).
    static void formatSlider(juce::Slider& s,const juce::String& id) {
        struct Fmt {double scale; int dec; const char* suffix; bool plus;};
        Fmt f {1.0,2,"",false};
        if(id=="effort"||id=="throatTexture"||id=="occlusionAmount"||id=="presenceHold"||id=="width"||id=="mix") f={100.0,0,"%",false};
        else if(id=="angle") f={1.0,0,"\xc2\xb0",false};
        else if(id=="size") f={1.0,2,"x",false};
        else if(id=="ambience") f={1.0,1," dB",false};
        else if(id=="output") f={1.0,1," dB",true};
        else if(id=="walkLength") f={1.0,0,"",false};
        const bool isDistance=(id=="distance");
        s.textFromValueFunction=[f,isDistance](double v) {
            if(isDistance) return juce::String(v,v<10.0?2:(v<100.0?1:0))+" m";
            double shown=v*f.scale;
            if(std::abs(shown)<std::pow(10.0,-f.dec)*.5) shown=0.0;
            auto text=juce::String(shown,f.dec);
            if(f.plus && shown>0.0) text="+"+text;
            return text+juce::String::fromUTF8(f.suffix);
        };
        s.valueFromTextFunction=[f](const juce::String& t) {
            return static_cast<double>(t.retainCharacters("0123456789.-+").getFloatValue())/f.scale;
        };
        s.updateText();
    }
    void setupChoice(juce::ComboBox& box,std::initializer_list<const char*> entries,const char* id) {
        for(auto* name:entries)box.addItem(name,box.getNumItems()+1);
        addAndMakeVisible(box);
        boxAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.parameters,id,box));
    }
    void loadPreset(const Preset& p) {
        for(auto* id:{"angle","occlusionAmount","occlusionType","size"}) resetParam(processor,id);
        setParam(processor,"space",static_cast<float>(p.scene));
        setParam(processor,"effort",p.effort);
        setParam(processor,"distance",p.distance);
        setParam(processor,"ambience",p.ambience);
        setParam(processor,"micType",static_cast<float>(p.mic));
        setParam(processor,"mix",1.f);
        setParam(processor,"angle",p.angle);
        setParam(processor,"occlusionAmount",p.occlusion);
        setParam(processor,"occlusionType",static_cast<float>(p.occlusionType));
        setParam(processor,"size",p.size);
        setParam(processor,"presenceHold",p.presence);
        if(p.texture>=0.f) setParam(processor,"throatTexture",p.texture);
        processor.editorState.setProperty("preset",p.name,nullptr);
    }
    static juce::File presetFolder() {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Skell Audio").getChildFile("Skell-Afar").getChildFile("Presets");
    }
    void refreshPresetList(const juce::String& select) {
        presetList.clear(juce::dontSendNotification);
        for(std::size_t i=0;i<factory.size();++i) presetList.addItem(factory[i].name,static_cast<int>(i)+1);
        userPresetFiles.clear();
        const auto dir=presetFolder();
        if(dir.isDirectory()) {
            auto files=dir.findChildFiles(juce::File::findFiles,false,"*.xml");
            std::sort(files.begin(),files.end(),[](const juce::File& x,const juce::File& y){return x.getFileName().compareNatural(y.getFileName())<0;});
            if(!files.isEmpty()) {
                presetList.addSeparator();
                presetList.addSectionHeading("USER PRESETS");
                for(const auto& f:files) {
                    userPresetFiles.push_back(f);
                    presetList.addItem(f.getFileNameWithoutExtension(),1000+static_cast<int>(userPresetFiles.size())-1);
                }
            }
        }
        presetList.setText(select,juce::dontSendNotification);
        if(presetList.getSelectedId()==0 && presetList.getText().isEmpty()) presetList.setSelectedId(1,juce::dontSendNotification);
    }
    void stepPreset(int delta) {
        const int n=presetList.getNumItems();
        if(n<=0)return;
        const int cur=presetList.getSelectedItemIndex();
        const int next=cur<0?(delta>0?0:n-1):(cur+delta+n)%n;
        presetList.setSelectedItemIndex(next,juce::sendNotificationSync);
    }
    void loadUserPreset(const juce::File& file) {
        if(auto xml=juce::XmlDocument::parse(file)) {
            if(xml->hasTagName(processor.parameters.state.getType())) {
                processor.parameters.replaceState(juce::ValueTree::fromXml(*xml));
                processor.editorState.setProperty("preset",file.getFileNameWithoutExtension(),nullptr);
            }
        }
    }
    void writePreset(const juce::String& rawName) {
        const auto name=juce::File::createLegalFileName(rawName.trim());
        if(name.isEmpty())return;
        const auto dir=presetFolder();
        if(!dir.isDirectory() && !dir.createDirectory().wasOk())return;
        if(auto xml=processor.parameters.copyState().createXml()) {
            if(dir.getChildFile(name+".xml").replaceWithText(xml->toString())) {
                processor.editorState.setProperty("preset",name,nullptr);
                refreshPresetList(name);
            }
        }
    }
    void saveCurrentPreset() {
        auto current=presetList.getText();
        if(current.isEmpty() || current.startsWith("INIT")) current="My Preset";
        auto* window=new juce::AlertWindow("Save preset","Name your preset:",juce::MessageBoxIconType::NoIcon,this);
        window->addTextEditor("name",juce::File::createLegalFileName(current));
        window->addButton("Save",1,juce::KeyPress(juce::KeyPress::returnKey));
        window->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
        const juce::Component::SafePointer<Surface> safe(this);
        window->enterModalState(true,juce::ModalCallbackFunction::create([safe,window](int result){
            if(result==1 && safe.getComponent()!=nullptr) safe.getComponent()->writePreset(window->getTextEditorContents("name"));
        }),true);
    }
    SkellAfarAudioProcessor& processor;
    std::function<void(float)> onScale;
    std::function<void()> onShowTour;
    std::unique_ptr<afar::ui::AfarLookAndFeel> look;
    juce::Image bg,logo;
    afar::ui::SpaceBrowser browser;
    afar::ui::SceneView sceneView;
    afar::ui::ScenePad scenePad;
    afar::ui::MeterView meters;
    afar::ui::DetailPanel detail;
    juce::TextButton moreButton;
    juce::TooltipWindow tooltip {this,600};
    int lastWalkState=-1;
    juce::Slider effort,distance,walkLength,throatTexture;
    std::array<juce::Slider,8> knobs;
    std::array<juce::Label,8> knobLabels;
    juce::ComboBox mic,recorded,walkPath,presetList;
    juce::ToggleButton power,walkEnabled,delayToggle,muteAmbience,levelMatch;
    juce::TextButton save,settings,randomise,toggleAB;
    juce::ValueTree slotState[2];
    int activeSlot=0;
    std::vector<juce::File> userPresetFiles;
    juce::TextButton prevPreset,nextPreset;
    float lastEffort=-1.f,lastDistance=-1.f,lastTexture=-1.f;
    int lastRecorded=-1;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> boxAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;
};

SkellAfarAudioProcessorEditor::SkellAfarAudioProcessorEditor(SkellAfarAudioProcessor& p)
    :juce::AudioProcessorEditor(&p),processor(p) {
    surface=std::make_unique<Surface>(p,[this](float factor){
        const int w=juce::roundToInt(afar::ui::Theme::designW*factor);
        const int h=juce::roundToInt(afar::ui::Theme::designH*factor);
        setSize(w,h);
        processor.editorState.setProperty("scale",factor,nullptr);
    },[this]{showGuidedTour(false);});
    setLookAndFeel(&look);
    addAndMakeVisible(*surface);
    setResizable(true,true);
    setResizeLimits(825,525,1650,1050);
    getConstrainer()->setFixedAspectRatio(static_cast<double>(afar::ui::Theme::designW)/afar::ui::Theme::designH);
    const float factor=std::clamp(static_cast<float>(static_cast<double>(p.editorState.getProperty("scale",.75))),.75f,1.5f);
    setSize(juce::roundToInt(afar::ui::Theme::designW*factor),juce::roundToInt(afar::ui::Theme::designH*factor));
    guidedTour=std::make_unique<skellcore::ui::GuidedTourController>(*this,"skell-afar-vst",1);
    showGuidedTour(true);
}
SkellAfarAudioProcessorEditor::~SkellAfarAudioProcessorEditor(){
    guidedTour.reset();
    setLookAndFeel(nullptr);
}
void SkellAfarAudioProcessorEditor::showGuidedTour(bool firstRun) {
    if(!guidedTour || !surface)return;
    guidedTour->setPalette(surface->tourPalette());
    guidedTour->setSteps(surface->tourSteps());
    if(firstRun)guidedTour->startIfFirstRun();
    else guidedTour->replay();
}
void SkellAfarAudioProcessorEditor::dismissTourForScreenshot() {
    if (guidedTour) guidedTour->dismiss();
}
bool SkellAfarAudioProcessorEditor::replayTourForScreenshot() {
    if (!guidedTour || !surface) return false;
    guidedTour->setPalette(surface->tourPalette());
    guidedTour->setSteps(surface->tourSteps());
    return guidedTour->replay();
}
void SkellAfarAudioProcessorEditor::resized() {
    if(!surface)return;
    surface->setTransform(juce::AffineTransform());
    surface->setBounds(0,0,afar::ui::Theme::designW,afar::ui::Theme::designH);
    surface->setTransform(juce::AffineTransform::scale(static_cast<float>(getWidth())/afar::ui::Theme::designW));
}
