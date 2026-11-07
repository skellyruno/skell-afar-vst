#include "PluginEditor.h"
#include <SkellCore/UI.h>
#include <BinaryData.h>
#include <array>
#include <functional>
#include <memory>

namespace {
struct Preset {
    const char* name; int scene; float effort,distance,ambience; int mic;
};
constexpr std::array<Preset,24> factory {{
    {"INIT / Natural 3 m",5,.5f,3.f,-30.f,7},
    {"Shout Down The Tunnel",0,.92f,25.f,-25.f,7},
    {"Whisper From Dark End",0,.10f,80.f,-17.f,0},
    {"Passing Car Voice",0,.73f,24.f,-18.f,0},
    {"Whisper In The Pipe",1,.12f,19.f,-15.f,0},
    {"Drip Call",1,.61f,10.f,-10.f,1},
    {"Subway Announcement",2,.76f,30.f,-10.f,1},
    {"Cave Echo Cry",3,.91f,27.f,-23.f,7},
    {"Bunker Radio",4,.62f,6.f,-24.f,6},
    {"Graveyard Call",5,.83f,43.f,-17.f,7},
    {"Midnight Whisper",5,.09f,12.f,-16.f,0},
    {"Stone Echo",5,.67f,35.f,-22.f,7},
    {"Forest Whisper 30 m",6,.11f,30.f,-15.f,7},
    {"Lost In The Trees",6,.65f,90.f,-18.f,1},
    {"Canyon Shout",7,1.f,130.f,-21.f,7},
    {"Valley Answer",7,.8f,155.f,-18.f,7},
    {"Distant Voice Memo",8,.55f,48.f,-17.f,0},
    {"Neighbor's Yard",8,.62f,25.f,-22.f,0},
    {"Footsteps Closer",9,.65f,20.f,-19.f,7},
    {"Running Away",9,.80f,30.f,-19.f,0},
    {"Far Adlib Throw",7,.86f,69.f,-27.f,7},
    {"Haunted Whisper Layer",5,.13f,56.f,-22.f,1},
    {"Crowd Across The Field",8,.78f,110.f,-12.f,7},
    {"Empty Pool Shout",13,.93f,7.f,-28.f,7}
}};
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
    Surface(SkellAfarAudioProcessor& p,std::function<void(float)> scaleCallback)
      : processor(p),onScale(std::move(scaleCallback)),browser(),sceneView(p),scenePad(p),meters(p) {
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
            slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,100,28);
            slider->setColour(juce::Slider::textBoxTextColourId,afar::ui::Theme::bone);
            addAndMakeVisible(*slider);
        }
        attach(effort,"effort");attach(distance,"distance");
        throatTexture.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        throatTexture.setTextBoxStyle(juce::Slider::TextBoxBelow,false,62,17);
        throatTexture.setDoubleClickReturnValue(true,0.0);
        throatTexture.setTooltip("ONE MACRO: Clean > Airy > Raspy > Gritty. Adjusts source texture before space and microphone effects.");
        addAndMakeVisible(throatTexture);
        attach(throatTexture,"throatTexture");
        const char* ids[]={"angle","occlusionAmount","size","ambience","presenceHold","width","mix","output"};
        const char* titles[]={"ANGLE","OCCLUSION","SIZE","AMBIENCE","PRESENCE","WIDTH","MIX","OUTPUT"};
        for(int i=0;i<8;++i){
            knobs[static_cast<std::size_t>(i)].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knobs[static_cast<std::size_t>(i)].setTextBoxStyle(juce::Slider::TextBoxBelow,false,63,16);
            knobLabels[static_cast<std::size_t>(i)].setText(titles[i],juce::dontSendNotification);
            knobLabels[static_cast<std::size_t>(i)].setJustificationType(juce::Justification::centred);
            knobLabels[static_cast<std::size_t>(i)].setFont(juce::Font(juce::FontOptions(9.2f)));
            knobLabels[static_cast<std::size_t>(i)].setColour(juce::Label::textColourId,afar::ui::Theme::muted);
            addAndMakeVisible(knobs[static_cast<std::size_t>(i)]);
            addAndMakeVisible(knobLabels[static_cast<std::size_t>(i)]);
            attach(knobs[static_cast<std::size_t>(i)],ids[i]);
        }
        setupChoice(mic,{"PHONE","HANDHELD","SHOTGUN","LAVALIER","STUDIO","DICTAPHONE","WALKIE","IDEAL"},"micType");
        setupChoice(recorded,{"WHISPER","SOFT","NORMAL","LOUD","AUTO"},"recordedAs");
        setupChoice(walkPath,{"APPROACH","RETREAT","PASS BY","PACING","RUN AWAY"},"walkPath");
        walkEnabled.setButtonText("WALK");addAndMakeVisible(walkEnabled);
        buttonAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"walkOn",walkEnabled));
        walkLength.setSliderStyle(juce::Slider::LinearHorizontal);
        walkLength.setTextBoxStyle(juce::Slider::TextBoxRight,false,50,20);
        addAndMakeVisible(walkLength);attach(walkLength,"walkLength");
        power.setButtonText("ON");addAndMakeVisible(power);
        buttonAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"power",power));
        delayToggle.setButtonText("BEAT ALIGNED");addAndMakeVisible(delayToggle);
        buttonAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"removeDelay",delayToggle));
        muteAmbience.setButtonText("MUTE AMB");addAndMakeVisible(muteAmbience);
        buttonAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"muteAmbience",muteAmbience));
        for(const auto& entry:factory) presetList.addItem(entry.name,presetList.getNumItems()+1);
        addAndMakeVisible(presetList);
        presetList.setSelectedItemIndex(0,juce::dontSendNotification);
        presetList.onChange=[this]{
            const int ix=presetList.getSelectedItemIndex();
            if(ix>=0 && ix<static_cast<int>(factory.size()))loadPreset(factory[static_cast<std::size_t>(ix)]);
        };
        settings.setButtonText("SET");addAndMakeVisible(settings);
        settings.onClick=[this]{
            juce::PopupMenu m;
            m.addItem(1,"Small · 75%");m.addItem(2,"Medium · 100%");
            m.addItem(3,"Large · 125%");m.addItem(4,"XL · 150%");
            m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settings),[this, safe=juce::Component::SafePointer<Surface>(this)](int choice){
                if(!safe)return;
                const float factors[]{.75f,1.f,1.25f,1.5f};
                if(choice>=1 && choice<=4)onScale(factors[choice-1]);
            });
        };
        save.setButtonText("SAVE");addAndMakeVisible(save);
        save.onClick=[this]{saveCurrentPreset();};
        randomise.setButtonText("DICE");addAndMakeVisible(randomise);
        randomise.onClick=[this]{const int ix=juce::Random::getSystemRandom().nextInt(static_cast<int>(factory.size()));
            presetList.setSelectedItemIndex(ix,juce::sendNotificationSync);};
        toggleAB.setButtonText("A/B");addAndMakeVisible(toggleAB);
        toggleAB.onClick=[this]{
            if(!alternate.isValid()) {alternate=processor.parameters.copyState();toggleAB.setButtonText("B");return;}
            auto current=processor.parameters.copyState();processor.parameters.replaceState(alternate);
            alternate=current;toggleAB.setButtonText(toggleAB.getButtonText()=="A"?"B":"A");
        };
        startTimerHz(20);
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
        if(logo.isValid())g.drawImage(logo,53,5,47,47,0,0,96,96);
        g.setColour(Theme::greenLight);g.setFont(titleFont(21.f));
        g.drawText("SKELL-AFAR",111,7,264,29,juce::Justification::centredLeft);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText("FEEL THE DISTANCE",113,33,260,14,juce::Justification::centredLeft);
        panel(g,{314.f,417.f,489.f,144.f});
        g.setColour(Theme::bone);g.setFont(titleFont(13.f));
        g.drawText("EFFORT",327,423,280,16,juce::Justification::centredLeft);
        const float target=processor.parameters.getRawParameterValue("effort")->load();
        const int as=static_cast<int>(processor.parameters.getRawParameterValue("recordedAs")->load());
        g.setColour(Theme::muted);
        g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText("~"+juce::String(afar::effortSpl(target),0)+" dB SPL @ 1 m · estimated",560,423,224,16,juce::Justification::centredRight);
        if(std::abs(target-afar::recordedEffort(as))>.30f){
            g.setColour(Theme::fire);
            g.drawText("LARGE EFFORT CHANGE: ARTICULATION MAY NOT MATCH",440,475,344,14,juce::Justification::centredRight);
        }
        g.setColour(Theme::ice);g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText("WHISPER",329,470,180,16,juce::Justification::centredLeft);
        g.setColour(Theme::fire);g.drawText("SHOUT",714,470,72,16,juce::Justification::centredRight);
        g.setColour(Theme::bone);g.setFont(titleFont(13.f));
        g.drawText("DISTANCE",327,491,280,16,juce::Justification::centredLeft);
        const auto metres=processor.parameters.getRawParameterValue("distance")->load();
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText(juce::String(afar::arrivalMilliseconds(metres),1)+" ms arrival",614,491,170,16,juce::Justification::centredRight);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText("IN YOUR EAR",329,541,170,16,juce::Justification::centredLeft);
        g.setColour(Theme::violet);g.drawText("FAR AWAY",708,541,79,16,juce::Justification::centredRight);
        panel(g,{820.f,346.f,260.f,215.f});
        g.setColour(Theme::bone);g.setFont(titleFont(14.f));
        g.drawText("WALK PATH",834,356,229,17,juce::Justification::centredLeft);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText("PATH PRESET",832,404,210,13,juce::Justification::centredLeft);
        g.drawText("LENGTH (BARS)",832,459,230,13,juce::Justification::centredLeft);
        panel(g,{314.f,575.f,774.f,115.f});
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(9.5f)));
        g.drawText("MIC DEVICE",829,599,112,17,juce::Justification::centredLeft);
        g.drawText("RECORDED AS",955,599,116,17,juce::Justification::centredLeft);
        const float texture=processor.parameters.getRawParameterValue("throatTexture")->load();
        const juce::String textureStage=texture<.16f?"CLEAN":texture<.50f?"AIRY":texture<.83f?"RASPY":"GRITTY";
        g.setColour(Theme::greenLight);g.setFont(titleFont(10.f));
        g.drawText("THROAT TEXTURE",320,578,90,13,juce::Justification::centred);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(9.f)));
        g.drawText(textureStage,329,673,71,13,juce::Justification::centred);
    }
    void resized() override {
        browser.setBounds(afar::ui::Theme::browserX,afar::ui::Theme::browserY,afar::ui::Theme::browserW,afar::ui::Theme::browserH);
        sceneView.setBounds(320,72,480,332);
        scenePad.setBounds(820,72,260,260);
        effort.setBounds(326,440,465,34);
        distance.setBounds(326,509,465,34);
        walkEnabled.setBounds(834,379,93,25);
        walkPath.setBounds(832,420,234,29);
        walkLength.setBounds(832,477,234,24);
        meters.setBounds(833,504,232,53);
        delayToggle.setBounds(836,569,131,20); // intentionally near transport strip
        muteAmbience.setBounds(969,569,115,20);
        throatTexture.setBounds(326,592,75,76);
        for(int i=0;i<8;++i){
            const int x=408+i*50;
            knobs[static_cast<std::size_t>(i)].setBounds(x,596,49,62);
            knobLabels[static_cast<std::size_t>(i)].setBounds(x-2,659,52,17);
        }
        mic.setBounds(828,619,112,27);
        recorded.setBounds(950,619,122,27);
        settings.setBounds(10,12,37,30);
        presetList.setBounds(388,13,275,28);
        save.setBounds(672,13,58,28);
        randomise.setBounds(736,13,55,28);
        toggleAB.setBounds(798,13,50,28);
        power.setBounds(1001,15,86,27);
    }
private:
    void timerCallback() override {
        browser.setSelectedSpace(static_cast<int>(processor.parameters.getRawParameterValue("space")->load()));
        repaint(317,574,93,115);
    }
    void attach(juce::Slider& slider,const char* id) {
        sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,id,slider));
    }
    void setupChoice(juce::ComboBox& box,std::initializer_list<const char*> entries,const char* id) {
        for(auto* name:entries)box.addItem(name,box.getNumItems()+1);
        addAndMakeVisible(box);
        boxAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.parameters,id,box));
    }
    void loadPreset(const Preset& p) {
        setParam(processor,"space",static_cast<float>(p.scene));
        setParam(processor,"effort",p.effort);
        setParam(processor,"distance",p.distance);
        setParam(processor,"ambience",p.ambience);
        setParam(processor,"micType",static_cast<float>(p.mic));
        setParam(processor,"mix",juce::String(p.name).startsWith("INIT")?0.f:1.f);
        processor.editorState.setProperty("preset",p.name,nullptr);
    }
    void saveCurrentPreset() {
        auto location=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Skell Audio").getChildFile("Skell-Afar").getChildFile("Presets");
        if (!location.isDirectory() && !location.createDirectory().wasOk())return;
        const auto presetName=juce::File::createLegalFileName(presetList.getText().isEmpty()?"User Preset":presetList.getText());
        if(auto xml=processor.parameters.copyState().createXml()) {
            const auto file=location.getChildFile(presetName+".xml");
            if(file.replaceWithText(xml->toString()))processor.editorState.setProperty("preset",presetName,nullptr);
        }
    }
    SkellAfarAudioProcessor& processor;
    std::function<void(float)> onScale;
    std::unique_ptr<afar::ui::AfarLookAndFeel> look;
    juce::Image bg,logo;
    afar::ui::SpaceBrowser browser;
    afar::ui::SceneView sceneView;
    afar::ui::ScenePad scenePad;
    afar::ui::MeterView meters;
    juce::Slider effort,distance,walkLength,throatTexture;
    std::array<juce::Slider,8> knobs;
    std::array<juce::Label,8> knobLabels;
    juce::ComboBox mic,recorded,walkPath,presetList;
    juce::ToggleButton power,walkEnabled,delayToggle,muteAmbience;
    juce::TextButton save,settings,randomise,toggleAB;
    juce::ValueTree alternate;
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
    });
    addAndMakeVisible(*surface);
    setResizable(true,true);
    setResizeLimits(825,525,1650,1050);
    getConstrainer()->setFixedAspectRatio(static_cast<double>(afar::ui::Theme::designW)/afar::ui::Theme::designH);
    const float factor=std::clamp(static_cast<float>(static_cast<double>(p.editorState.getProperty("scale",.75))),.75f,1.5f);
    setSize(juce::roundToInt(afar::ui::Theme::designW*factor),juce::roundToInt(afar::ui::Theme::designH*factor));
}
SkellAfarAudioProcessorEditor::~SkellAfarAudioProcessorEditor()=default;
void SkellAfarAudioProcessorEditor::resized() {
    if(!surface)return;
    surface->setTransform(juce::AffineTransform());
    surface->setBounds(0,0,afar::ui::Theme::designW,afar::ui::Theme::designH);
    surface->setTransform(juce::AffineTransform::scale(static_cast<float>(getWidth())/afar::ui::Theme::designW));
}
