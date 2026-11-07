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
    }
    void drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float, float,
                          juce::Slider::SliderStyle style,juce::Slider& slider) override {
        if(style!=juce::Slider::LinearHorizontal) {
            juce::LookAndFeel_V4::drawLinearSlider(g,x,y,w,h,pos,pos,pos,style,slider);return;
        }
        const auto track=juce::Rectangle<float>(static_cast<float>(x)+10,y+h*.48f,static_cast<float>(w)-20,8.f);
        const auto id=slider.getComponentID();
        const auto left=id=="effort"?Theme::ice:Theme::bone;
        const auto right=id=="effort"?Theme::fire:Theme::violet;
        juce::ColourGradient grad(left,track.getX(),track.getY(),right,track.getRight(),track.getY(),false);
        g.setGradientFill(grad);g.fillRoundedRectangle(track,4.f);
        const float px=std::clamp(pos,track.getX(),track.getRight());
        g.setColour(juce::Colours::black.withAlpha(.48f));g.fillEllipse(px-13.f,track.getCentreY()-13.f,26.f,26.f);
        g.setColour(Theme::bone);g.fillEllipse(px-7.f,track.getCentreY()-7.f,14.f,14.f);
        g.setColour(Theme::greenLight.withAlpha(.8f));g.drawEllipse(px-11.f,track.getCentreY()-11.f,22.f,22.f,1.2f);
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float value,float start,float end,juce::Slider&) override {
        const auto b=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),static_cast<float>(w),static_cast<float>(h)).reduced(6.f);
        const float r=std::min(b.getWidth(),b.getHeight())*.43f;
        const float cx=b.getCentreX(),cy=b.getCentreY();
        g.setColour(Theme::panelRaised);g.fillEllipse(cx-r,cy-r,2*r,2*r);
        g.setColour(Theme::greenDim.brighter(.3f));g.drawEllipse(cx-r,cy-r,2*r,2*r,1.5f);
        const float angle=start+std::clamp(value,0.f,1.f)*(end-start);
        juce::Path arc;arc.addCentredArc(cx,cy,r*.86f,r*.86f,0.f,start,angle,true);
        g.setColour(Theme::green);g.strokePath(arc,juce::PathStrokeType(2.4f));
        g.setColour(Theme::bone);g.drawLine(cx,cy,cx+std::sin(angle)*r*.66f,cy-std::cos(angle)*r*.66f,2.f);
    }
};

class ScenePad final : public juce::Component {
public:
    explicit ScenePad(SkellAfarAudioProcessor& p):processor(p){setMouseCursor(juce::MouseCursor::DraggingHandCursor);}
    void paint(juce::Graphics& g) override {
        panel(g,getLocalBounds().toFloat());
        g.setColour(Theme::bone);g.setFont(titleFont(14.f));g.drawText("SCENE PAD",12,10,getWidth()-24,18,juce::Justification::centredLeft);
        const auto r=juce::Rectangle<float>(15,39,static_cast<float>(getWidth())-30,static_cast<float>(getHeight())-57);
        g.setColour(Theme::greenDim);g.drawRoundedRectangle(r,4.f,1.2f);
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
        g.setColour(Theme::greenLight);g.fillEllipse(xx-5,yy-5,10,10);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText("WHISPER       EFFORT       SHOUT",16,getHeight()-17,getWidth()-32,13,juce::Justification::centred);
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
        const float pk=std::clamp(processor.meterPeak.load(std::memory_order_relaxed),0.f,1.f);
        const float clarity=std::clamp(processor.meterClarity.load(std::memory_order_relaxed),0.f,1.f);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText("PEAK",0,0,getWidth(),13,juce::Justification::centred);
        g.setColour(Theme::panelRaised);g.fillRoundedRectangle(5.f,17.f,getWidth()-10.f,7.f,3.f);
        g.setColour(pk>.9f?Theme::fire:Theme::green);g.fillRoundedRectangle(5.f,17.f,(getWidth()-10.f)*pk,7.f,3.f);
        g.setColour(Theme::muted);g.drawText("CLEAR",0,30,getWidth(),13,juce::Justification::centred);
        g.setColour(Theme::panelRaised);g.fillRoundedRectangle(5.f,48.f,getWidth()-10.f,7.f,3.f);
        g.setColour(Theme::ice);g.fillRoundedRectangle(5.f,48.f,(getWidth()-10.f)*clarity,7.f,3.f);
    }
private:
    void timerCallback() override {repaint();}
    SkellAfarAudioProcessor& processor;
};
}
