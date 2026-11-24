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
        setColour(juce::Slider::textBoxBackgroundColourId,Theme::panelDeep);
        setColour(juce::Slider::textBoxOutlineColourId,Theme::lineSoft);
        setColour(juce::ComboBox::backgroundColourId,Theme::panelRaised);
        setColour(juce::ComboBox::textColourId,Theme::bone);
        setColour(juce::ComboBox::outlineColourId,Theme::line);
        setColour(juce::PopupMenu::backgroundColourId,Theme::panel);
        setColour(juce::PopupMenu::textColourId,Theme::bone);
        setColour(juce::TextButton::buttonColourId,Theme::panelRaised);
        setColour(juce::TextButton::textColourOffId,Theme::bone);
        setColour(juce::ToggleButton::textColourId,Theme::bone);
        setColour(juce::Label::textColourId,Theme::bone);
        setColour(juce::ScrollBar::thumbColourId,juce::Colour(0x40ffffff));
        setColour(juce::ScrollBar::trackColourId,juce::Colour(0x00000000));
        setColour(juce::TextEditor::backgroundColourId,Theme::panelDeep);
        setColour(juce::TextEditor::outlineColourId,Theme::line);
        setColour(juce::TextEditor::highlightColourId,Theme::greenDim);
        setColour(juce::TextEditor::highlightedTextColourId,Theme::bone);
        setColour(juce::TextEditor::focusedOutlineColourId,Theme::green.withAlpha(.7f));
        setColour(juce::CaretComponent::caretColourId,Theme::green);
        setColour(juce::ComboBox::arrowColourId,Theme::muted);
        setColour(juce::ComboBox::focusedOutlineColourId,Theme::green.withAlpha(.7f));
        setColour(juce::PopupMenu::highlightedBackgroundColourId,Theme::greenDim);
        setColour(juce::PopupMenu::highlightedTextColourId,Theme::bone);
        setColour(juce::TextButton::buttonOnColourId,Theme::greenDim);
        setColour(juce::TooltipWindow::backgroundColourId,Theme::panelRaised);
        setColour(juce::TooltipWindow::textColourId,Theme::bone);
        setColour(juce::TooltipWindow::outlineColourId,Theme::line);
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
    // Labels with an opaque background (the slider value boxes) are drawn as recessed wells; others are plain text.
    void drawLabel(juce::Graphics& g,juce::Label& label) override {
        const auto bounds=label.getLocalBounds().toFloat();
        if(label.findColour(juce::Label::backgroundColourId).getFloatAlpha()>.5f) inset(g,bounds,4.f);
        if(label.isBeingEdited()) return;
        const float alpha=label.isEnabled()?1.f:.5f;
        const auto font=label.getFont();
        const auto textArea=label.getBorderSize().subtractedFrom(label.getLocalBounds());
        g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));g.setFont(font);
        g.drawFittedText(label.getText(),textArea,label.getJustificationType(),
                         std::max(1,static_cast<int>(static_cast<float>(textArea.getHeight())/font.getHeight())),label.getMinimumHorizontalScale());
    }

    void drawToggleButton(juce::Graphics& g,juce::ToggleButton& b,bool highlighted,bool) override {
        const auto area=b.getLocalBounds().toFloat();
        const float s=std::min(16.f,area.getHeight()-4.f);
        const juce::Rectangle<float> box(area.getX()+2.f,area.getCentreY()-s*.5f,s,s);
        const bool on=b.getToggleState();
        if(on) glow(g,box.getCentreX(),box.getCentreY(),s*1.05f,Theme::green,.8f);
        inset(g,box,4.f);
        if(on){
            juce::ColourGradient lit(Theme::green.withAlpha(.30f),box.getX(),box.getY(),Theme::green.withAlpha(.08f),box.getX(),box.getBottom(),false);
            g.setGradientFill(lit);g.fillRoundedRectangle(box.reduced(1.f),3.f);
            g.setColour(Theme::green.withAlpha(.85f));g.drawRoundedRectangle(box.reduced(.5f),4.f,1.1f);
            juce::Path tick;
            tick.startNewSubPath(box.getX()+s*.24f,box.getCentreY());
            tick.lineTo(box.getX()+s*.44f,box.getBottom()-s*.27f);
            tick.lineTo(box.getRight()-s*.22f,box.getY()+s*.27f);
            g.setColour(Theme::greenLight);
            g.strokePath(tick,juce::PathStrokeType(1.9f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        } else if(highlighted) {
            g.setColour(Theme::line.brighter(.3f));g.drawRoundedRectangle(box.reduced(.5f),4.f,1.f);
        }
        g.setColour(b.isEnabled()?(on||highlighted?Theme::bone:Theme::bone.withAlpha(.82f)):Theme::muted);g.setFont(titleFont(11.f));
        g.drawText(b.getButtonText(),juce::Rectangle<float>(box.getRight()+8.f,area.getY(),area.getRight()-box.getRight()-8.f,area.getHeight()),
                   juce::Justification::centredLeft,true);
    }
    juce::Font getTextButtonFont(juce::TextButton&,int buttonHeight) override { return titleFont(std::min(12.f,static_cast<float>(buttonHeight)*.44f)); }
    void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour& background,bool highlighted,bool down) override {
        const auto r=b.getLocalBounds().toFloat().reduced(.5f);
        const bool on=b.getToggleState();
        auto top=background.brighter(.10f),bottom=background.darker(.30f);
        if(on){top=Theme::greenDim.brighter(.30f);bottom=Theme::greenDim.darker(.25f);}
        if(down) std::swap(top,bottom); else if(highlighted) top=top.brighter(.10f);
        juce::ColourGradient fill(top,r.getX(),r.getY(),bottom,r.getX(),r.getBottom(),false);
        g.setGradientFill(fill);g.fillRoundedRectangle(r,5.f);
        g.setColour(juce::Colour(down?0x00ffffff:0x1affffff));g.drawHorizontalLine(juce::roundToInt(r.getY())+1,r.getX()+5.f,r.getRight()-5.f);
        g.setColour(on?Theme::green.withAlpha(.6f):(highlighted?Theme::line.brighter(.4f):Theme::line));
        g.drawRoundedRectangle(r,5.f,1.f);
    }
    void drawComboBox(juce::Graphics& g,int width,int height,bool down,int,int,int,int,juce::ComboBox&) override {
        const juce::Rectangle<float> r(0.f,0.f,static_cast<float>(width),static_cast<float>(height));
        juce::ColourGradient fill(Theme::panelRaised.brighter(.06f),0.f,0.f,Theme::panelRaised.darker(.35f),0.f,r.getBottom(),false);
        g.setGradientFill(fill);g.fillRoundedRectangle(r.reduced(.5f),5.f);
        g.setColour(juce::Colour(0x14ffffff));g.drawHorizontalLine(1,5.f,r.getRight()-5.f);
        g.setColour(down?Theme::green.withAlpha(.6f):Theme::line);g.drawRoundedRectangle(r.reduced(.5f),5.f,1.f);
        const float ax=r.getRight()-19.f,ay=r.getCentreY();
        g.setColour(Theme::lineSoft);g.drawVerticalLine(juce::roundToInt(r.getRight()-30.f),6.f,r.getBottom()-6.f);
        juce::Path chevron;
        chevron.startNewSubPath(ax-4.f,ay-2.f);chevron.lineTo(ax,ay+2.5f);chevron.lineTo(ax+4.f,ay-2.f);
        g.setColour(down?Theme::greenLight:Theme::muted);
        g.strokePath(chevron,juce::PathStrokeType(1.6f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    }
    juce::Font getComboBoxFont(juce::ComboBox&) override { return titleFont(12.f); }
    void drawCornerResizer(juce::Graphics& g,int w,int h,bool over,bool dragging) override {
        g.setColour((over||dragging)?Theme::green:Theme::faint);
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
            static_cast<float>(y)+h*.5f-4.f,static_cast<float>(w)-22.f,8.f);
        const auto id=slider.getComponentID();
        const auto c0=id=="effort"?Theme::ice:id=="distance"?Theme::bone:Theme::greenLight;
        const auto c1=id=="effort"?Theme::fire:id=="distance"?Theme::violet:Theme::green;
        const float px=juce::jlimit(track.getX(),track.getRight(),pos);
        const float py=track.getCentreY();
        inset(g,track.expanded(2.f,2.5f),6.f);                                   // machined groove
        juce::ColourGradient color(c0.withAlpha(.20f),track.getX(),track.getY(),c1.withAlpha(.20f),track.getRight(),track.getY(),false);
        g.setGradientFill(color);g.fillRoundedRectangle(track,4.f);                      // full scale, dimmed
        const auto lit=juce::Rectangle<float>(track.getX(),track.getY(),px-track.getX(),track.getHeight());
        if(lit.getWidth()>0.f) {
            juce::ColourGradient active(c0,track.getX(),track.getY(),c1,track.getRight(),track.getY(),false);
            g.setGradientFill(active);g.fillRoundedRectangle(lit,std::min(4.f,lit.getWidth()*.5f));
            if(lit.getWidth()>6.f) {
                g.setColour(juce::Colours::white.withAlpha(.30f));
                g.drawHorizontalLine(juce::roundToInt(track.getY()+1),lit.getX()+3.f,lit.getRight()-3.f);
            }
        }
        for(int i=1;i<5;++i){                                                       // scale divisions
            const float tick=track.getX()+i*track.getWidth()/5.f;
            g.setColour(juce::Colours::black.withAlpha(.40f));
            g.drawVerticalLine(juce::roundToInt(tick),track.getY()+1.f,track.getBottom()-1.f);
        }
        // thumb: soft shadow, machined puck with a lit top edge, accent core
        g.setColour(juce::Colours::black.withAlpha(.16f));g.fillEllipse(px-14.f,py-11.f,28.f,28.f);
        g.setColour(juce::Colours::black.withAlpha(.22f));g.fillEllipse(px-12.f,py-10.f,24.f,24.f);
        juce::ColourGradient puck(juce::Colour(0xff5a6560),px-7.f,py-10.f,juce::Colour(0xff111715),px+7.f,py+10.f,false);
        g.setGradientFill(puck);g.fillEllipse(px-9.5f,py-9.5f,19.f,19.f);
        g.setColour(juce::Colours::white.withAlpha(.22f));g.drawEllipse(px-9.5f,py-9.5f,19.f,19.f,1.f);
        juce::ColourGradient dish(juce::Colour(0xff1a211e),px-5.f,py-6.f,juce::Colour(0xff2f3934),px+5.f,py+6.f,false);
        g.setGradientFill(dish);g.fillEllipse(px-6.f,py-6.f,12.f,12.f);
        glow(g,px,py,6.f,c1,.9f);
        g.setColour(c1.brighter(.25f));g.fillEllipse(px-2.2f,py-2.2f,4.4f,4.4f);
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float value,
                          float start,float end,juce::Slider& slider) override {
        if(w<12 || h<12) return;
        const auto b=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),
            static_cast<float>(w),static_cast<float>(h)).reduced(3.f);
        const float outerR=std::min(b.getWidth(),b.getHeight())*.5f;
        const float cx=b.getCentreX(),cy=b.getCentreY();
        const float radius=outerR*.72f,arcR=outerR*.86f;
        const float v=juce::jlimit(0.f,1.f,std::isfinite(value)?value:0.f);
        const float angle=start+v*(end-start);
        const bool texture=slider.getComponentID()=="throatTexture";
        const auto accent=texture?(v<.33f?Theme::ice:v<.67f?Theme::greenLight:Theme::fire):Theme::green;
        // scale ticks (outside the value arc)
        for(int i=0;i<=10;++i){
            const float a=start+(end-start)*static_cast<float>(i)/10.f,sn=std::sin(a),cs=-std::cos(a);
            const bool major=(i%5)==0;
            const float r0=outerR*(major?.93f:.96f),r1=outerR*1.0f;
            g.setColour(juce::Colours::white.withAlpha((a<=angle+.001f?.38f:.16f)+(major?.08f:0.f)));
            g.drawLine(cx+sn*r0,cy+cs*r0,cx+sn*r1,cy+cs*r1,major?1.3f:1.f);
        }
        // value arc with a soft halo
        juce::Path track,progress;
        track.addCentredArc(cx,cy,arcR,arcR,0.f,start,end,true);
        progress.addCentredArc(cx,cy,arcR,arcR,0.f,start,std::max(start+.001f,angle),true);
        g.setColour(juce::Colours::black.withAlpha(.55f));g.strokePath(track,juce::PathStrokeType(3.6f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        g.setColour(juce::Colours::white.withAlpha(.07f));g.strokePath(track,juce::PathStrokeType(2.2f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        if(v>.004f){
            g.setColour(accent.withAlpha(.055f));g.strokePath(progress,juce::PathStrokeType(6.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
            g.setColour(accent.withAlpha(.12f));g.strokePath(progress,juce::PathStrokeType(4.1f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
            g.setColour(accent);g.strokePath(progress,juce::PathStrokeType(2.6f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        }
        // body: contact shadow, bevelled rim, brushed cap with a lit upper edge
        g.setColour(juce::Colours::black.withAlpha(.16f));g.fillEllipse(cx-radius-3.f,cy-radius+.5f,2.f*radius+6.f,2.f*radius+6.f);
        g.setColour(juce::Colours::black.withAlpha(.26f));g.fillEllipse(cx-radius-1.5f,cy-radius+1.f,2.f*radius+3.f,2.f*radius+3.f);
        juce::ColourGradient rim(juce::Colour(0xff65706a),cx-radius,cy-radius,juce::Colour(0xff080b0a),cx+radius,cy+radius,false);
        g.setGradientFill(rim);g.fillEllipse(cx-radius,cy-radius,2*radius,2*radius);
        const float capR=radius*.85f;
        juce::ColourGradient cap(juce::Colour(0xff323c37),cx-capR*.7f,cy-capR*.9f,juce::Colour(0xff121816),cx+capR*.7f,cy+capR*.9f,false);
        g.setGradientFill(cap);g.fillEllipse(cx-capR,cy-capR,capR*2,capR*2);
        g.setColour(juce::Colours::black.withAlpha(.45f));g.drawEllipse(cx-capR,cy-capR,capR*2,capR*2,.8f);
        juce::Path sheen;sheen.addCentredArc(cx,cy,capR*.93f,capR*.93f,0.f,-2.55f,-.55f,true);
        g.setColour(juce::Colours::white.withAlpha(.20f));g.strokePath(sheen,juce::PathStrokeType(1.2f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        // pointer: shadowed ivory blade with an accent dot at the tip
        const float sn=std::sin(angle),cs=-std::cos(angle);
        const float p0=capR*.18f,p1=capR*.80f;
        g.setColour(juce::Colours::black.withAlpha(.65f));g.drawLine(cx+sn*p0+1.f,cy+cs*p0+1.f,cx+sn*p1+1.f,cy+cs*p1+1.f,3.2f);
        g.setColour(Theme::bone);g.drawLine(cx+sn*p0,cy+cs*p0,cx+sn*p1,cy+cs*p1,2.f);
        g.setColour(accent);g.fillEllipse(cx+sn*p1-1.6f,cy+cs*p1-1.6f,3.2f,3.2f);
    }
};

class ScenePad final : public juce::Component {
public:
    explicit ScenePad(SkellAfarAudioProcessor& p):processor(p){setMouseCursor(juce::MouseCursor::DraggingHandCursor);}
    void paint(juce::Graphics& g) override {
        panel(g,getLocalBounds().toFloat());
        sectionTitle(g,"SCENE PAD",12.f,9.f,static_cast<float>(getWidth())-24.f,14.f);
        const auto r=juce::Rectangle<float>(15,39,static_cast<float>(getWidth())-30,static_cast<float>(getHeight())-57);
        inset(g,r,7.f);
        juce::ColourGradient wash(juce::Colour(0x0cffffff),r.getCentreX(),r.getY(),juce::Colour(0x00ffffff),r.getCentreX(),r.getBottom(),false);
        g.setGradientFill(wash);g.fillRoundedRectangle(r.reduced(1.f),6.f);
        for(int i=1;i<5;++i){
            const float fx=r.getX()+i*r.getWidth()/5.f;
            const float fy=r.getY()+i*r.getHeight()/5.f;
            g.setColour(Theme::lineSoft);
            g.drawLine(fx,r.getY()+4.f,fx,r.getBottom()-4.f,1.f);g.drawLine(r.getX()+4.f,fy,r.getRight()-4.f,fy,1.f);
        }
        const float effort=processor.parameters.getRawParameterValue("effort")->load();
        const float distance=processor.parameters.getRawParameterValue("distance")->load();
        const float xx=r.getX()+effort*r.getWidth();
        const float yy=r.getY()+distanceNormalized(distance)*r.getHeight();
        g.setColour(Theme::green.withAlpha(.22f));
        g.drawLine(xx,r.getY()+4.f,xx,r.getBottom()-4.f,1.f);g.drawLine(r.getX()+4.f,yy,r.getRight()-4.f,yy,1.f);
        glow(g,xx,yy,22.f,Theme::green,1.1f);
        g.setColour(juce::Colours::black.withAlpha(.5f));g.fillEllipse(xx-6.f,yy-4.f,12.f,12.f);
        g.setColour(Theme::bone);g.fillEllipse(xx-5.f,yy-5.f,10.f,10.f);
        g.setColour(Theme::green);g.fillEllipse(xx-2.2f,yy-2.2f,4.4f,4.4f);
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
            const float sx=static_cast<float>(i)*(segW+gap);
            g.setColour(isHold?Theme::bone:(active?on:Theme::panelDeep));
            g.fillRoundedRectangle(sx,15.f,segW,8.f,1.5f);
            if(active && !isHold){g.setColour(juce::Colours::white.withAlpha(.22f));g.fillRoundedRectangle(sx,15.f,segW,2.5f,1.2f);}
            else if(!active){g.setColour(Theme::lineSoft);g.drawRoundedRectangle(sx+.5f,15.5f,segW-1.f,7.f,1.5f,1.f);}
        }
        g.setColour(Theme::muted);g.drawText("CLARITY EST.",0.f,31.f,92.f,12.f,juce::Justification::centredLeft);
        g.setColour(Theme::bone);g.drawText(juce::String(juce::roundToInt(clarity*100.f))+"%",w-80.f,31.f,80.f,12.f,juce::Justification::centredRight);
        inset(g,juce::Rectangle<float>(0.f,46.f,w,8.f),4.f);
        juce::ColourGradient fill(Theme::ice.darker(.4f),0.f,0.f,Theme::ice,w,0.f,false);
        const float clarityWidth=std::max(0.f,(w-2.f)*clarity);
        if(clarityWidth>0.f) {
            g.setGradientFill(fill);
            g.fillRoundedRectangle(1.f,47.f,clarityWidth,6.f,std::min(3.f,clarityWidth*.5f));
        }
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
