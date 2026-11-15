#pragma once
#include "Theme.h"
#include "../dsp/SceneModel.h"
#include <BinaryData.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>

namespace afar::ui {
// SIMPLE | PRO segmented switch for the top bar.
class ModeSwitch final : public juce::Component, public juce::SettableTooltipClient {
public:
    ModeSwitch() { setTooltip("SIMPLE: just the stage and the environments. PRO: every control."); setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    std::function<void(bool)> onChange;
    void setSimple(bool s) { if(simple!=s){simple=s;repaint();} }
    bool isSimple() const {return simple;}
    void paint(juce::Graphics& g) override {
        const auto r=getLocalBounds().toFloat();
        inset(g,r,r.getHeight()*.5f);
        const float half=r.getWidth()*.5f;
        const auto active=juce::Rectangle<float>(simple?r.getX():r.getX()+half,r.getY(),half,r.getHeight()).reduced(2.f);
        glow(g,active.getCentreX(),active.getCentreY(),active.getWidth()*.8f,Theme::green,.8f);
        juce::ColourGradient fill(Theme::greenDim.brighter(.45f),0.f,active.getY(),Theme::greenDim.darker(.1f),0.f,active.getBottom(),false);
        g.setGradientFill(fill);g.fillRoundedRectangle(active,active.getHeight()*.5f);
        g.setColour(Theme::green.withAlpha(.65f));g.drawRoundedRectangle(active.reduced(.5f),active.getHeight()*.5f,1.f);
        g.setFont(titleFont(9.5f));
        g.setColour(simple?Theme::bone:Theme::muted);g.drawText("SIMPLE",r.withWidth(half),juce::Justification::centred);
        g.setColour(simple?Theme::muted:Theme::bone);g.drawText("PRO",r.withTrimmedLeft(half),juce::Justification::centred);
    }
    void mouseUp(const juce::MouseEvent& e) override {
        const bool wantSimple=static_cast<float>(e.x)<static_cast<float>(getWidth())*.5f;
        if(wantSimple!=simple){simple=wantSimple;repaint();if(onChange)onChange(simple);}
    }
private:
    bool simple=false;
};

// Horizontally scrolling row of environment cards for SIMPLE mode: click to choose, drag or wheel to scroll,
// arrows at the edges, and the chosen card is eased to the centre.
class EnvironmentStrip final : public juce::Component, public juce::SettableTooltipClient, private juce::Timer {
public:
    EnvironmentStrip() {
        for(int i=0;i<20;++i) {
            int sz=0;
            const auto resource=juce::String::formatted("space_%02d_png",i);
            if(const auto* bytes=BinaryData::getNamedResource(resource.toRawUTF8(),sz))
                covers[static_cast<std::size_t>(i)]=juce::ImageFileFormat::loadFrom(bytes,static_cast<std::size_t>(sz));
        }
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        setTooltip("Click an environment. Drag or scroll to browse.");
        startTimerHz(60);
    }
    std::function<void(int)> onSelect;
    void setSelectedSpace(int s) {
        s=std::clamp(s,0,19);
        if(s!=selected) {selected=s;centreOn(s,false);repaint();}
    }
    void resized() override {centreOn(selected,true);}
    void paint(juce::Graphics& g) override {
        const auto bounds=getLocalBounds().toFloat();
        panel(g,bounds);
        sectionTitle(g,"ENVIRONMENTS",14.f,8.f,200.f,13.f);
        g.setColour(Theme::muted);g.setFont(captionFont(10.f));
        g.drawText(juce::String(selected+1)+" / 20  \xc2\xb7  drag or scroll to browse",bounds.getWidth()-260.f,9.f,246.f,14.f,juce::Justification::centredRight);
        const float cy=cardTop(),ch=cardHeight();
        g.saveState();
        g.reduceClipRegion(getLocalBounds().withTrimmedTop(static_cast<int>(cy)-6).withTrimmedBottom(4));
        for(int i=0;i<20;++i) {
            const float x=cardX(i)-scroll;
            if(x>bounds.getWidth()||x+cardW<0.f) continue;
            drawCard(g,i,{x,cy-(i==selected?3.f:0.f),cardW,ch+(i==selected?3.f:0.f)});
        }
        g.restoreState();
        const float fadeW=44.f;   // edge fades and arrows
        const juce::Colour edge(0xff0f1417);
        juce::ColourGradient left(edge,0.f,0.f,edge.withAlpha(0.f),fadeW,0.f,false);
        g.setGradientFill(left);g.fillRect(1.f,cy-6.f,fadeW,ch+12.f);
        juce::ColourGradient right(edge.withAlpha(0.f),bounds.getWidth()-fadeW,0.f,edge,bounds.getWidth(),0.f,false);
        g.setGradientFill(right);g.fillRect(bounds.getWidth()-fadeW-1.f,cy-6.f,fadeW,ch+12.f);
        if(scroll>2.f) arrow(g,18.f,cy+ch*.5f,-1);
        if(scroll<maxScroll()-2.f) arrow(g,bounds.getWidth()-18.f,cy+ch*.5f,1);
    }
    void mouseDown(const juce::MouseEvent& e) override {downX=e.x;downScroll=target;dragged=false;}
    void mouseDrag(const juce::MouseEvent& e) override {
        const float dx=static_cast<float>(e.x-downX);
        if(std::abs(dx)>4.f) dragged=true;
        if(dragged) {target=std::clamp(downScroll-dx,0.f,maxScroll());scroll=target;repaint();}
    }
    void mouseUp(const juce::MouseEvent& e) override {
        if(dragged) return;
        const float x=static_cast<float>(e.x);
        if(x<34.f && scroll>2.f) {target=std::clamp(target-(cardW+gap)*2.f,0.f,maxScroll());return;}
        if(x>static_cast<float>(getWidth())-34.f && scroll<maxScroll()-2.f) {target=std::clamp(target+(cardW+gap)*2.f,0.f,maxScroll());return;}
        const float y=static_cast<float>(e.y);
        if(y<cardTop()-4.f || y>cardTop()+cardHeight()+4.f) return;
        const int i=static_cast<int>(std::floor((x+scroll-pad)/(cardW+gap)));
        const float within=x+scroll-pad-static_cast<float>(i)*(cardW+gap);
        if(i>=0 && i<20 && within<=cardW) {
            selected=i;centreOn(i,false);repaint();
            if(onSelect) onSelect(i);
        }
    }
    void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails& w) override {
        const float d=std::abs(w.deltaX)>std::abs(w.deltaY)?w.deltaX:-w.deltaY;
        target=std::clamp(target+d*(cardW+gap)*3.f,0.f,maxScroll());
    }
private:
    static constexpr float cardW=188.f,gap=12.f,pad=18.f;
    float cardTop() const {return 34.f;}
    float cardHeight() const {return std::max(40.f,static_cast<float>(getHeight())-cardTop()-12.f);}
    static float cardX(int i) {return pad+static_cast<float>(i)*(cardW+gap);}
    float maxScroll() const {return std::max(0.f,pad*2.f+20.f*(cardW+gap)-gap-static_cast<float>(getWidth()));}
    void centreOn(int i,bool instant) {
        target=std::clamp(cardX(i)+cardW*.5f-static_cast<float>(getWidth())*.5f,0.f,maxScroll());
        if(instant) scroll=target;
    }
    void timerCallback() override {
        if(std::abs(target-scroll)>.4f) {scroll+=(target-scroll)*.22f;repaint();}
        else if(scroll!=target) {scroll=target;repaint();}
    }
    static void arrow(juce::Graphics& g,float cx,float cy,int dir) {
        glow(g,cx,cy,22.f,Theme::green,.7f);
        g.setColour(juce::Colour(0xcc0a0e10));g.fillEllipse(cx-13.f,cy-13.f,26.f,26.f);
        g.setColour(Theme::line.brighter(.3f));g.drawEllipse(cx-13.f,cy-13.f,26.f,26.f,1.f);
        juce::Path c;const float d=static_cast<float>(dir);
        c.startNewSubPath(cx-d*2.5f,cy-5.f);c.lineTo(cx+d*2.5f,cy);c.lineTo(cx-d*2.5f,cy+5.f);
        g.setColour(Theme::bone);g.strokePath(c,juce::PathStrokeType(1.8f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    }
    void drawCard(juce::Graphics& g,int id,juce::Rectangle<float> card) const {
        const bool isSel=id==selected;
        const auto& spec=getSpace(id);
        const auto tint=spec.category==Category::Underground?Theme::ice:spec.category==Category::Outdoor?Theme::green:Theme::amber;
        if(isSel) glow(g,card.getCentreX(),card.getCentreY(),card.getWidth()*.75f,Theme::green,1.1f);
        juce::Path clip;clip.addRoundedRectangle(card,9.f);
        g.saveState();g.reduceClipRegion(clip);
        const auto& img=covers[static_cast<std::size_t>(id)];
        if(img.isValid()) {   // crop the art side of the cover (the right ~60%) to the card's aspect
            const float fullH=static_cast<float>(img.getHeight()),srcW=std::min(static_cast<float>(img.getWidth())*.62f,fullH*card.getWidth()/card.getHeight());
            const float srcX=static_cast<float>(img.getWidth())-srcW;
            g.drawImage(img,juce::roundToInt(card.getX()),juce::roundToInt(card.getY()),juce::roundToInt(card.getWidth()),juce::roundToInt(card.getHeight()),
                        juce::roundToInt(srcX),0,juce::roundToInt(srcW),img.getHeight());
        } else {g.fillAll(Theme::panelRaised);}
        juce::ColourGradient scrim(juce::Colour(0x00080b0d),0.f,card.getY()+card.getHeight()*.30f,juce::Colour(0xf2080b0d),0.f,card.getBottom(),false);
        g.setGradientFill(scrim);g.fillRect(card);
        if(!isSel) {g.setColour(juce::Colour(0x4a0a0e10));g.fillRect(card);}   // unselected cards sit back
        g.restoreState();
        g.setColour(isSel?Theme::green.withAlpha(.85f):Theme::line);
        g.drawRoundedRectangle(card.reduced(.5f),9.f,isSel?1.6f:1.f);
        const juce::Rectangle<float> pill(card.getX()+10.f,card.getY()+9.f,50.f,15.f);
        g.setColour(juce::Colours::black.withAlpha(.45f));g.fillRoundedRectangle(pill,7.5f);
        g.setColour(tint.withAlpha(.5f));g.drawRoundedRectangle(pill.reduced(.5f),7.5f,1.f);
        g.setColour(tint);g.setFont(titleFont(8.5f));
        g.drawText(spec.category==Category::Underground?"UNDER":spec.category==Category::Outdoor?"OUTDOOR":"INDOOR",pill,juce::Justification::centred);
        g.setColour(isSel?Theme::greenLight:Theme::bone);g.setFont(titleFont(14.f));
        g.drawFittedText(spec.name,juce::roundToInt(card.getX())+11,juce::roundToInt(card.getBottom())-40,juce::roundToInt(card.getWidth())-22,20,juce::Justification::centredLeft,1);
        g.setColour(Theme::muted.brighter(.15f));g.setFont(captionFont(10.5f));
        g.drawFittedText(spec.description,juce::roundToInt(card.getX())+11,juce::roundToInt(card.getBottom())-22,juce::roundToInt(card.getWidth())-22,16,juce::Justification::centredLeft,1);
    }
    std::array<juce::Image,20> covers;
    int selected=5,downX=0;
    float scroll=0.f,target=0.f,downScroll=0.f;
    bool dragged=false;
};
}
