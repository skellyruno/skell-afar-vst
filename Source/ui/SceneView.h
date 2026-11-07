#pragma once
#include "Theme.h"
#include "../PluginProcessor.h"
#include <cmath>

namespace afar::ui {
class SceneView final : public juce::Component, private juce::Timer {
public:
    explicit SceneView(SkellAfarAudioProcessor& p):processor(p) {startTimerHz(30);setMouseCursor(juce::MouseCursor::DraggingHandCursor);}
    void paint(juce::Graphics& g) override {
        auto b=getLocalBounds().toFloat();panel(g,b);
        auto drawArea=b.reduced(13.f,39.f);
        const int scene=static_cast<int>(processor.parameters.getRawParameterValue("space")->load());
        const auto& s=getSpace(scene);
        g.setColour(Theme::green.withAlpha(.1f));
        for(int i=1;i<=6;++i){
            const float y=drawArea.getY()+drawArea.getHeight()*i/7.f;
            g.drawHorizontalLine(static_cast<int>(y),drawArea.getX(),drawArea.getRight());
        }
        if(scene==0 || scene==1 || scene==2 || scene==9 || scene==10 || scene==11 || scene==12 || scene==13 || scene==14) {
            for(int i=0;i<7;++i) {
                const float t=i/6.f;
                auto inner=drawArea.reduced(drawArea.getWidth()*.44f*t,drawArea.getHeight()*.40f*t);
                g.setColour(Theme::green.withAlpha(.10f+.15f*(1.f-t)));
                g.drawRoundedRectangle(inner,15.f,1.1f);
            }
        } else if(scene==5) {
            for(int i=0;i<11;++i) {
                const float x=drawArea.getX()+12+i*42.f;
                const float y=drawArea.getY()+30+static_cast<float>((i*23)%105);
                g.setColour(Theme::muted.withAlpha(.25f));
                g.drawRoundedRectangle(x,y,17,22,7,1.4f);
                g.drawLine(x+8,y+2,x+8,y+17,1.f);
            }
        } else if(scene==6) {
            for(int i=0;i<24;++i){
                float x=drawArea.getX()+float((i*97)%444),y=drawArea.getY()+float((i*61)%205);
                g.setColour(Theme::greenDim.brighter(.5f).withAlpha(.32f));g.fillEllipse(x,y,13+(i%5)*4,13+(i%5)*4);
            }
        } else if(scene==7) {
            juce::Path canyon;
            canyon.startNewSubPath(drawArea.getX(),drawArea.getBottom());
            canyon.lineTo(drawArea.getX()+64,drawArea.getY()+35);
            canyon.lineTo(drawArea.getCentreX()-45,drawArea.getBottom()-32);
            canyon.lineTo(drawArea.getCentreX()+40,drawArea.getBottom()-32);
            canyon.lineTo(drawArea.getRight()-62,drawArea.getY()+36);
            canyon.lineTo(drawArea.getRight(),drawArea.getBottom());
            g.setColour(Theme::greenDim.brighter(.7f).withAlpha(.5f));g.strokePath(canyon,juce::PathStrokeType(3.f));
        } else {
            g.setColour(Theme::muted.withAlpha(.3f));g.drawLine(drawArea.getX(),drawArea.getBottom()*.73f,drawArea.getRight(),drawArea.getBottom()*.73f,2.f);
        }
        const float d=processor.meterDistance.load(std::memory_order_relaxed);
        const float x=drawArea.getX()+drawArea.getWidth()*(1.f-distanceNormalized(d));
        const float y=drawArea.getCentreY();
        const auto level=processor.meterPeak.load(std::memory_order_relaxed);
        g.setColour(Theme::green.withAlpha(.07f+.10f*level));
        for(int i=1;i<=3;++i)g.drawEllipse(x-i*18.f,y-i*18.f,i*36.f,i*36.f,1.f);
        g.setColour(Theme::green.withAlpha(.14f));g.drawLine(x,y,drawArea.getRight()-20,y,1.5f);
        g.setColour(Theme::greenLight);g.fillEllipse(x-6,y-6,12,12);
        const float micX=drawArea.getRight()-20;
        g.setColour(Theme::bone);g.fillRoundedRectangle(micX-6,y-12,12,24,3);
        g.drawLine(micX,y+13,micX,y+20,1.5f);
        g.setColour(Theme::bone);g.setFont(titleFont(16.f));
        g.drawText(juce::String(s.name).toUpperCase(),15,10,getWidth()-30,21,juce::Justification::centredLeft);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(11.f)));
        g.drawText("VOICE  /  REFLECTION MODEL  /  MIC",15,31,getWidth()-30,15,juce::Justification::centredLeft);
        const float dr=processor.meterDrr.load(std::memory_order_relaxed);
        auto status=juce::String(d,2)+" m     DRR "+juce::String(dr,1)+" dB     RT60 "+juce::String(s.rt60Seconds,1)+" s";
        g.setColour(Theme::bone);g.drawFittedText(status,16,getHeight()-34,getWidth()-32,23,juce::Justification::centredLeft,1);
    }
    void mouseDown(const juce::MouseEvent& e) override {
        if(auto* p=processor.parameters.getParameter("distance"))p->beginChangeGesture();
        drag(e);
    }
    void mouseDrag(const juce::MouseEvent& e) override {drag(e);}
    void mouseUp(const juce::MouseEvent&) override {
        if(auto* p=processor.parameters.getParameter("distance"))p->endChangeGesture();
    }
private:
    void drag(const juce::MouseEvent& e) {
        const float nx=std::clamp((static_cast<float>(e.x)-13.f)/std::max(1.f,static_cast<float>(getWidth())-26.f),0.f,1.f);
        if(auto* p=processor.parameters.getParameter("distance"))
            p->setValueNotifyingHost(p->convertTo0to1(distanceMeters(1.f-nx)));
    }
    void timerCallback() override {repaint();}
    SkellAfarAudioProcessor& processor;
};
}
