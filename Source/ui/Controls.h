#pragma once
#include "Theme.h"
#include "../PluginProcessor.h"
#include <cmath>

namespace afar::ui {
class AfarLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    AfarLookAndFeel(){
        setColour(juce::Slider::thumbColourId,Theme::green);
        setColour(juce::Slider::textBoxTextColourId,Theme::bone);
        setColour(juce::Slider::textBoxBackgroundColourId,Theme::panel);
        setColour(juce::Slider::textBoxOutlineColourId,Theme::greenDim);
        setColour(juce::ComboBox::backgroundColourId,Theme::panelRaised);
        setColour(juce::ComboBox::textColourId,Theme::bone);
        setColour(juce::ComboBox::outlineColourId,Theme::greenDim);
        setColour(juce::PopupMenu::backgroundColourId,Theme::panel);
        setColour(juce::PopupMenu::textColourId,Theme::bone);
        setColour(juce::TextButton::buttonColourId,Theme::panelRaised);
        setColour(juce::TextButton::textColourOffId,Theme::bone);
        setColour(juce::ToggleButton::textColourId,Theme::bone);
        setColour(juce::Label::textColourId,Theme::bone);
        setColour(juce::ScrollBar::thumbColourId,Theme::greenDim.brighter(.55f));
        setColour(juce::ScrollBar::trackColourId,Theme::background);
        setColour(juce::TextEditor::highlightColourId,Theme::greenDim);
        setColour(juce::TextEditor::highlightedTextColourId,Theme::bone);
        setColour(juce::TextEditor::focusedOutlineColourId,Theme::green);
        setColour(juce::CaretComponent::caretColourId,Theme::green);
        setColour(juce::ComboBox::arrowColourId,Theme::greenLight);
        setColour(juce::ComboBox::focusedOutlineColourId,Theme::green);
        setColour(juce::PopupMenu::highlightedBackgroundColourId,Theme::greenDim);
        setColour(juce::PopupMenu::highlightedTextColourId,Theme::bone);
        setColour(juce::TextButton::buttonOnColourId,Theme::greenDim);
        setColour(juce::TooltipWindow::backgroundColourId,Theme::panelRaised);
        setColour(juce::TooltipWindow::textColourId,Theme::bone);
        setColour(juce::TooltipWindow::outlineColourId,Theme::greenDim);
    }
    // Deterministic horizontal-slider layout so painted range labels line up with the track.
    juce::Slider::SliderLayout getSliderLayout(juce::Slider& s) override {
        if(s.getSliderStyle()!=juce::Slider::LinearHorizontal || s.getTextBoxPosition()!=juce::Slider::TextBoxRight)
            return juce::LookAndFeel_V4::getSliderLayout(s);
        const auto b=s.getLocalBounds();
        const int tw=std::min(s.getTextBoxWidth(),b.getWidth()/2),th=std::min(s.getTextBoxHeight(),b.getHeight());
        juce::Slider::SliderLayout l;
        l.textBoxBounds={b.getRight()-tw,b.getCentreY()-th/2,tw,th};
        l.sliderBounds={b.getX(),b.getY(),b.getWidth()-tw-6,b.getHeight()};
        return l;
    }
    juce::Label* createSliderTextBox(juce::Slider& s) override {
        auto* l=juce::LookAndFeel_V4::createSliderTextBox(s);
        l->setFont(juce::Font(juce::FontOptions(s.getSliderStyle()==juce::Slider::LinearHorizontal?12.5f:10.5f)));
        l->setJustificationType(juce::Justification::centred);
        l->setBorderSize(juce::BorderSize<int>(0,2,0,2));
        l->setMinimumHorizontalScale(.75f);
        return l;
    }
    void drawToggleButton(juce::Graphics& g,juce::ToggleButton& b,bool highlighted,bool) override {
        const auto area=b.getLocalBounds().toFloat();
        const float s=std::min(16.f,area.getHeight()-4.f);
        const juce::Rectangle<float> box(area.getX()+2.f,area.getCentreY()-s*.5f,s,s);
        const bool on=b.getToggleState();
        g.setColour(on?Theme::green.withAlpha(.16f):Theme::background);g.fillRoundedRectangle(box,3.f);
        g.setColour(on?Theme::green:(highlighted?Theme::greenLight:Theme::greenDim.brighter(.4f)));
        g.drawRoundedRectangle(box,3.f,1.3f);
        if(on){
            juce::Path tick;
            tick.startNewSubPath(box.getX()+s*.24f,box.getCentreY());
            tick.lineTo(box.getX()+s*.44f,box.getBottom()-s*.27f);
            tick.lineTo(box.getRight()-s*.22f,box.getY()+s*.27f);
            g.strokePath(tick,juce::PathStrokeType(2.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        }
        g.setColour(b.isEnabled()?Theme::bone:Theme::muted);g.setFont(titleFont(11.5f));
        g.drawText(b.getButtonText(),juce::Rectangle<float>(box.getRight()+7.f,area.getY(),area.getRight()-box.getRight()-7.f,area.getHeight()),
                   juce::Justification::centredLeft,true);
    }
    void drawCornerResizer(juce::Graphics& g,int w,int h,bool over,bool dragging) override {
        g.setColour((over||dragging)?Theme::green:Theme::greenDim.brighter(.5f));
        for(int i=1;i<=3;++i) {
            const float o=static_cast<float>(i)*5.f;
            g.drawLine(static_cast<float>(w)-o,static_cast<float>(h)-1.f,static_cast<float>(w)-1.f,static_cast<float>(h)-o,1.5f);
        }
    }
    void drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,
                          float minPos,float maxPos,juce::Slider::SliderStyle style,
                          juce::Slider& slider) override {
        if(style!=juce::Slider::LinearHorizontal) {
            juce::LookAndFeel_V4::drawLinearSlider(g,x,y,w,h,pos,minPos,maxPos,style,slider);
            return;
        }
        if(w<24 || h<12) return;
        const auto track=juce::Rectangle<float>(static_cast<float>(x)+11.f,
            static_cast<float>(y)+h*.5f-5.f,static_cast<float>(w)-22.f,10.f);
        const auto id=slider.getComponentID();
        const auto c0=id=="effort"?Theme::ice:id=="distance"?Theme::bone:Theme::greenLight;
        const auto c1=id=="effort"?Theme::fire:id=="distance"?Theme::violet:Theme::green;
        g.setColour(juce::Colour(0xff030607));
        g.fillRoundedRectangle(track.expanded(2.f),6.f);
        juce::ColourGradient color(c0,track.getX(),track.getY(),c1,track.getRight(),track.getY(),false);
        g.setGradientFill(color);
        g.fillRoundedRectangle(track,5.f);
        g.setColour(juce::Colours::white.withAlpha(.25f));
        g.drawHorizontalLine(juce::roundToInt(track.getY()+1),track.getX()+4,track.getRight()-4);
        for(int i=1;i<5;++i){
            const float tick=track.getX()+i*track.getWidth()/5.f;
            g.setColour(juce::Colours::black.withAlpha(.43f));
            g.drawVerticalLine(juce::roundToInt(tick),track.getY(),track.getBottom());
        }
        const float px=juce::jlimit(track.getX(),track.getRight(),pos);
        const float py=track.getCentreY();
        g.setColour(juce::Colours::black.withAlpha(.52f));g.fillEllipse(px-13.f,py-12.f,27.f,27.f);
        juce::ColourGradient metal(Theme::bone.brighter(.08f),px-8.f,py-9.f,Theme::panelRaised,px+8.f,py+10.f,false);
        g.setGradientFill(metal);g.fillEllipse(px-9.f,py-9.f,18.f,18.f);
        g.setColour(Theme::greenLight);g.drawEllipse(px-10.f,py-10.f,20.f,20.f,1.3f);
        g.setColour(Theme::background);g.fillEllipse(px-2.4f,py-2.4f,4.8f,4.8f);
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float value,
                          float start,float end,juce::Slider& slider) override {
        if(w<12 || h<12) return;
        const auto b=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),
            static_cast<float>(w),static_cast<float>(h)).reduced(4.f);
        const float radius=std::min(b.getWidth(),b.getHeight())*.43f;
        const float cx=b.getCentreX(),cy=b.getCentreY();
        const float v=juce::jlimit(0.f,1.f,std::isfinite(value)?value:0.f);
        const float angle=start+v*(end-start);
        const bool texture=slider.getComponentID()=="throatTexture";
        const auto accent=texture?(v<.33f?Theme::ice:v<.67f?Theme::greenLight:Theme::fire):Theme::green;
        // Procedural 3D metal: no dynamic blur, textures or allocations.
        g.setColour(juce::Colours::black.withAlpha(.72f));
        g.fillEllipse(cx-radius-2.f,cy-radius+2.f,2.f*radius+4.f,2.f*radius+4.f);
        juce::ColourGradient outer(juce::Colour(0xff3e5145),cx-radius,cy-radius,
                                   juce::Colour(0xff080e0c),cx+radius,cy+radius,false);
        g.setGradientFill(outer);g.fillEllipse(cx-radius,cy-radius,2*radius,2*radius);
        g.setColour(Theme::muted.withAlpha(.65f));
        g.drawEllipse(cx-radius,cy-radius,2*radius,2*radius,1.f);
        const float innerR=radius*.78f;
        juce::ColourGradient core(juce::Colour(0xff38413a),cx-innerR,cy-innerR,
                                  juce::Colour(0xff101512),cx+innerR,cy+innerR,false);
        g.setGradientFill(core);g.fillEllipse(cx-innerR,cy-innerR,innerR*2,innerR*2);
        juce::Path track,progress;
        track.addCentredArc(cx,cy,radius*.92f,radius*.92f,0.f,start,end,true);
        progress.addCentredArc(cx,cy,radius*.92f,radius*.92f,0.f,start,angle,true);
        g.setColour(Theme::muted.withAlpha(.26f));g.strokePath(track,juce::PathStrokeType(2.8f));
        g.setColour(accent.withAlpha(.9f));g.strokePath(progress,juce::PathStrokeType(2.8f));
        const float endpointX=cx+std::sin(angle)*innerR*.72f;
        const float endpointY=cy-std::cos(angle)*innerR*.72f;
        g.setColour(juce::Colours::black.withAlpha(.7f));g.drawLine(cx+1.f,cy+1.f,endpointX+1.f,endpointY+1.f,3.4f);
        g.setColour(Theme::bone);g.drawLine(cx,cy,endpointX,endpointY,2.f);
        g.setColour(accent.withAlpha(.92f));g.fillEllipse(cx-2.f,cy-2.f,4.f,4.f);
    }
};

class ScenePad final : public juce::Component {
public:
    explicit ScenePad(SkellAfarAudioProcessor& p):processor(p){setMouseCursor(juce::MouseCursor::DraggingHandCursor);}
    void paint(juce::Graphics& g) override {
        panel(g,getLocalBounds().toFloat());
        sectionTitle(g,"SCENE PAD",12.f,9.f,static_cast<float>(getWidth())-24.f,14.f);
        const auto r=juce::Rectangle<float>(15,39,static_cast<float>(getWidth())-30,static_cast<float>(getHeight())-57);
        juce::ColourGradient padGradient(Theme::panelRaised,r.getX(),r.getY(),Theme::background,r.getRight(),r.getBottom(),false);
        g.setGradientFill(padGradient);g.fillRoundedRectangle(r,6.f);
        g.setColour(Theme::greenDim.brighter(.3f));g.drawRoundedRectangle(r,6.f,1.2f);
        for(int i=1;i<5;++i){
            const float fx=r.getX()+i*r.getWidth()/5.f;
            const float fy=r.getY()+i*r.getHeight()/5.f;
            g.setColour(Theme::muted.withAlpha(.16f));
            g.drawLine(fx,r.getY(),fx,r.getBottom());g.drawLine(r.getX(),fy,r.getRight(),fy);
        }
        const float effort=processor.parameters.getRawParameterValue("effort")->load();
        const float distance=processor.parameters.getRawParameterValue("distance")->load();
        const float xx=r.getX()+effort*r.getWidth();
        const float yy=r.getY()+distanceNormalized(distance)*r.getHeight();
        g.setColour(Theme::green.withAlpha(.14f));g.fillEllipse(xx-16,yy-16,32,32);
        g.setColour(Theme::greenLight.withAlpha(.26f));g.fillEllipse(xx-11,yy-11,22,22);
        g.setColour(Theme::bone);g.fillEllipse(xx-4.2f,yy-4.2f,8.4f,8.4f);
        g.setColour(Theme::muted.withAlpha(.85f));g.setFont(juce::Font(juce::FontOptions(9.5f)));
        g.drawText("NEAR",19,43,53,13,juce::Justification::centredLeft);
        g.drawText("FAR",19,getHeight()-34,53,13,juce::Justification::centredLeft);
        g.drawText("WHISPER",r.getX(),getHeight()-17.f,70.f,13.f,juce::Justification::centredLeft);
        g.drawText("EFFORT",r.getCentreX()-35.f,getHeight()-17.f,70.f,13.f,juce::Justification::centred);
        g.drawText("SHOUT",r.getRight()-70.f,getHeight()-17.f,70.f,13.f,juce::Justification::centredRight);
        g.setColour(Theme::bone.withAlpha(.85f));g.setFont(titleFont(10.f));
        g.drawText(juce::String(distance,1)+" m",r.getRight()-80.f,r.getY()+5.f,72.f,17.f,juce::Justification::centredRight);
    }
    void mouseDown(const juce::MouseEvent& e) override {gesture(true);drag(e);}
    void mouseDrag(const juce::MouseEvent& e) override {drag(e);}
    void mouseUp(const juce::MouseEvent&) override {gesture(false);}
private:
    void gesture(bool start){for(auto* id:{"effort","distance"})if(auto* p=processor.parameters.getParameter(id)){
        if(start)p->beginChangeGesture();else p->endChangeGesture();}}
    void drag(const juce::MouseEvent& e) {
        const auto r=juce::Rectangle<float>(15,39,static_cast<float>(getWidth())-30,static_cast<float>(getHeight())-57);
        const float x=std::clamp((e.position.x-r.getX())/r.getWidth(),0.f,1.f);
        const float y=std::clamp((e.position.y-r.getY())/r.getHeight(),0.f,1.f);
        if(auto* p=processor.parameters.getParameter("effort"))p->setValueNotifyingHost(x);
        if(auto* p=processor.parameters.getParameter("distance"))p->setValueNotifyingHost(y);
    }
    SkellAfarAudioProcessor& processor;
};

class MeterView final : public juce::Component, private juce::Timer {
public:
    explicit MeterView(SkellAfarAudioProcessor& p):processor(p){startTimerHz(25);}
    void paint(juce::Graphics& g) override {
        const float w=static_cast<float>(getWidth());
        const float peak=std::clamp(processor.meterPeak.load(std::memory_order_relaxed),0.f,1.f);
        const float clarity=std::clamp(processor.meterClarity.load(std::memory_order_relaxed),0.f,1.f);
        // dB-scaled segmented LED bar (-48..0 dBFS) with peak-hold marker
        constexpr int segments=26;
        const float gap=2.f,segW=(w-gap*(segments-1))/segments;
        const float db=juce::Decibels::gainToDecibels(peak,-60.f);
        g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.setColour(Theme::muted);g.drawText("OUTPUT",0.f,0.f,80.f,12.f,juce::Justification::centredLeft);
        g.setColour(db>-1.f?Theme::fire:Theme::bone);
        g.drawText(peak<.001f?juce::String("-inf"):juce::String(db,1)+" dB",w-80.f,0.f,80.f,12.f,juce::Justification::centredRight);
        const int lit=juce::roundToInt(fraction*segments),holdIndex=juce::jlimit(0,segments-1,juce::roundToInt(hold*segments)-1);
        for(int i=0;i<segments;++i) {
            const float pos=static_cast<float>(i)/segments;
            const auto on=pos<.70f?Theme::green:(pos<.88f?juce::Colour(0xffffc24a):Theme::fire);
            const bool active=i<lit,isHold=(i==holdIndex && hold>.02f);
            g.setColour(isHold?Theme::bone:(active?on:Theme::panelRaised));
            g.fillRoundedRectangle(static_cast<float>(i)*(segW+gap),15.f,segW,8.f,1.5f);
        }
        g.setColour(Theme::muted);g.drawText("CLARITY",0.f,31.f,80.f,12.f,juce::Justification::centredLeft);
        g.setColour(Theme::bone);g.drawText(juce::String(juce::roundToInt(clarity*100.f))+"%",w-80.f,31.f,80.f,12.f,juce::Justification::centredRight);
        g.setColour(Theme::panelRaised);g.fillRoundedRectangle(0.f,46.f,w,8.f,4.f);
        juce::ColourGradient fill(Theme::ice.darker(.4f),0.f,0.f,Theme::ice,w,0.f,false);
        g.setGradientFill(fill);g.fillRoundedRectangle(0.f,46.f,std::max(8.f,w*clarity),8.f,4.f);
    }
private:
    void timerCallback() override {
        const float peak=std::clamp(processor.meterPeak.load(std::memory_order_relaxed),0.f,1.f);
        fraction=std::clamp((juce::Decibels::gainToDecibels(peak,-60.f)+48.f)/48.f,0.f,1.f);
        if(fraction>=hold){hold=fraction;holdFrames=30;}
        else if(holdFrames>0)--holdFrames;
        else hold=std::max(fraction,hold-.025f);
        repaint();
    }
    SkellAfarAudioProcessor& processor;
    float fraction=0.f,hold=0.f;
    int holdFrames=0;
};
}
