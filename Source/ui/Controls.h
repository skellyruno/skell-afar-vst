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
        g.setColour(Theme::bone);g.setFont(titleFont(14.f));g.drawText("SCENE PAD",12,10,getWidth()-24,18,juce::Justification::centredLeft);
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
        g.drawText("WHISPER        EFFORT        SHOUT",16,getHeight()-17,getWidth()-32,13,juce::Justification::centred);
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
