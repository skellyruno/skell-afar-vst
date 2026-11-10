#pragma once
#include "Theme.h"
#include "../PluginProcessor.h"
#include <algorithm>
#include <cmath>

namespace afar::ui {
// Vector scene display. The per-scene backdrop is rendered once into a cached image
// (at physical pixel density) and only the source, ripples and path are animated.
class SceneView final : public juce::Component, public juce::SettableTooltipClient, private juce::Timer {
public:
    explicit SceneView(SkellAfarAudioProcessor& p):processor(p) {startTimerHz(30);setMouseCursor(juce::MouseCursor::DraggingHandCursor);setTooltip("Drag the voice: left/right changes distance, up/down changes its facing angle.");}
    void resized() override {backdropScene=-1;}
    void paint(juce::Graphics& g) override {
        const auto b=getLocalBounds().toFloat();
        panel(g,b);
        const int scene=std::clamp(static_cast<int>(processor.parameters.getRawParameterValue("space")->load()),0,14);
        const auto& s=getSpace(scene);
        const auto art=artArea();
        // ---- header
        sectionTitle(g,juce::String(s.name).toUpperCase(),14.f,10.f,b.getWidth()-120.f,16.f);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(11.f)));
        g.drawText(s.description,24.f,31.f,b.getWidth()-120.f,14.f,juce::Justification::centredLeft);
        const char* cat=s.category==Category::Underground?"UNDERGROUND":s.category==Category::Outdoor?"OUTDOOR":"INDOOR";
        const juce::Rectangle<float> pill(b.getRight()-104.f,12.f,90.f,17.f);
        g.setColour(Theme::background);g.fillRoundedRectangle(pill,8.5f);
        g.setColour(Theme::greenDim.brighter(.3f));g.drawRoundedRectangle(pill,8.5f,1.f);
        g.setColour(Theme::greenLight);g.setFont(titleFont(9.f));g.drawText(cat,pill,juce::Justification::centred);
        // ---- backdrop
        juce::Path clip;clip.addRoundedRectangle(art,7.f);
        g.saveState();
        g.reduceClipRegion(clip);
        juce::ColourGradient sky(juce::Colour(0xff0b1411),art.getX(),art.getY(),juce::Colour(0xff040706),art.getX(),art.getBottom(),false);
        g.setGradientFill(sky);g.fillRect(art);
        ensureBackdrop(scene,g.getInternalContext().getPhysicalPixelScaleFactor());
        if(backdrop.isValid()) g.drawImage(backdrop,art);
        // drifting dust motes (cheap, deterministic) for a little life in every space
        for(int i=0;i<16;++i) {
            const float sp=.25f+.75f*hash01(i,3);
            const float px=art.getX()+art.getWidth()*std::fmod(hash01(i,1)+drift*sp,1.f);
            const float py=art.getY()+art.getHeight()*(.10f+.80f*hash01(i,2))+3.f*std::sin(drift*9.f*sp+static_cast<float>(i));
            g.setColour(Theme::greenLight.withAlpha(.07f+.10f*sp));
            g.fillEllipse(px-1.f,py-1.f,2.f*sp+1.f,2.f*sp+1.f);
        }
        // ---- source / mic (distance runs right = near to left = far)
        const float d=std::max(.05f,processor.meterDistance.load(std::memory_order_relaxed));
        const float facing=std::clamp(processor.parameters.getRawParameterValue("angle")->load()/180.f,0.f,1.f);
        const float level=std::clamp(processor.meterPeak.load(std::memory_order_relaxed),0.f,1.f);
        const float x=srcX(distanceNormalized(d)),y=art.getY()+art.getHeight()*(.20f+.60f*facing);
        const float micX=micPosition(),floorY=art.getBottom()-9.f;
        g.setColour(Theme::green.withAlpha(.30f));g.drawLine(x,y,micX,y,1.4f);
        // first-floor-bounce path (illustrative only, not a ray trace)
        juce::Path bounce;bounce.startNewSubPath(x,y);bounce.lineTo((x+micX)*.5f,floorY);bounce.lineTo(micX,y);
        g.setColour(Theme::ice.withAlpha(.16f));g.strokePath(bounce,juce::PathStrokeType(1.f));
        for(int i=0;i<3;++i) {
            const float progress=std::fmod(pulsePhase+static_cast<float>(i)/3.f,1.f);
            const float rr=9.f+progress*74.f;
            g.setColour(Theme::green.withAlpha((.20f+.16f*level)*(1.f-progress)));
            g.drawEllipse(x-rr,y-rr,rr*2.f,rr*2.f,1.1f);
        }
        g.setColour(Theme::green.withAlpha(.18f));g.fillEllipse(x-11.f,y-11.f,22.f,22.f);
        g.setColour(Theme::greenLight);g.fillEllipse(x-5.5f,y-5.5f,11.f,11.f);
        g.setColour(Theme::bone);g.fillRoundedRectangle(micX-5.f,y-11.f,10.f,22.f,3.f);
        g.drawLine(micX,y+12.f,micX,y+19.f,1.5f);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(9.f)));
        g.drawText("VOICE",x-24.f,y-26.f,48.f,11.f,juce::Justification::centred);
        g.drawText("MIC",micX-16.f,y-26.f,32.f,11.f,juce::Justification::centred);
        g.restoreState();
        g.setColour(Theme::greenDim.withAlpha(.9f));g.drawRoundedRectangle(art,7.f,1.f);
        // ---- log distance ruler (0.1 .. 100 m), marker follows the source
        const float ry=art.getBottom()+13.f;
        g.setColour(Theme::greenDim.brighter(.2f));g.drawLine(srcX(1.f),ry,srcX(0.f),ry,1.f);
        g.setFont(juce::Font(juce::FontOptions(9.f)));
        for(const float m:{.1f,1.f,10.f,100.f}) {
            const float tx=srcX(distanceNormalized(m));
            g.setColour(Theme::muted.withAlpha(.8f));g.drawLine(tx,ry-3.f,tx,ry+3.f,1.f);
            g.drawText(juce::String(m<1.f?.1:static_cast<double>(m),m<1.f?1:0)+" m",tx-20.f,ry+4.f,40.f,11.f,juce::Justification::centred);
        }
        juce::Path marker;marker.addTriangle(x-5.f,ry+4.f,x+5.f,ry+4.f,x,ry-3.f);
        g.setColour(Theme::green);g.fillPath(marker);
        // ---- readout chips
        const float dr=processor.meterDrr.load(std::memory_order_relaxed);
        const float cy=b.getBottom()-31.f;
        chip(g,14.f,cy,"DIST",meters(d));
        chip(g,126.f,cy,"DRR",juce::String(dr,1)+" dB");
        chip(g,238.f,cy,"RT60",juce::String(s.rt60Seconds,1)+" s");
        const float sizeNow=processor.parameters.getRawParameterValue("size")->load();
        chip(g,350.f,cy,"ECHO",juce::String(echoDelayMs(s.firstEchoMeters*sizeNow),0)+" ms");
    }
    void mouseDown(const juce::MouseEvent& e) override {
        for(auto* id:{"distance","angle"})
            if(auto* p=processor.parameters.getParameter(id))p->beginChangeGesture();
        drag(e);
    }
    void mouseDrag(const juce::MouseEvent& e) override {drag(e);}
    void mouseUp(const juce::MouseEvent&) override {
        for(auto* id:{"distance","angle"})
            if(auto* p=processor.parameters.getParameter(id))p->endChangeGesture();
    }
private:
    juce::Rectangle<float> artArea() const {
        return {13.f,52.f,static_cast<float>(getWidth())-26.f,static_cast<float>(getHeight())-52.f-76.f};
    }
    float micPosition() const {const auto a=artArea();return a.getRight()-22.f;}
    // normalised distance 0 (near) .. 1 (far) -> x; far is left, near is next to the mic
    float srcX(float norm) const {
        const auto a=artArea();const float left=a.getX()+18.f,right=micPosition()-20.f;
        return left+(right-left)*(1.f-std::clamp(norm,0.f,1.f));
    }
    static juce::String meters(float m) {return juce::String(m,m<10.f?2:(m<100.f?1:0))+" m";}
    static void chip(juce::Graphics& g,float x,float y,const juce::String& label,const juce::String& value) {
        const juce::Rectangle<float> r(x,y,104.f,22.f);
        g.setColour(Theme::background);g.fillRoundedRectangle(r,6.f);
        g.setColour(Theme::greenDim.withAlpha(.9f));g.drawRoundedRectangle(r,6.f,1.f);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(9.f)));
        g.drawText(label,r.withTrimmedLeft(8.f),juce::Justification::centredLeft);
        g.setColour(Theme::bone);g.setFont(titleFont(11.f));
        g.drawText(value,r.withTrimmedRight(8.f),juce::Justification::centredRight);
    }
    void drag(const juce::MouseEvent& e) {
        const auto a=artArea();
        const float left=a.getX()+18.f,right=micPosition()-20.f;
        const float nx=std::clamp((static_cast<float>(e.x)-left)/std::max(1.f,right-left),0.f,1.f);
        if(auto* p=processor.parameters.getParameter("distance"))
            p->setValueNotifyingHost(p->convertTo0to1(distanceMeters(1.f-nx)));
        const float ny=std::clamp((static_cast<float>(e.y)-a.getY())/std::max(1.f,a.getHeight()),0.f,1.f);
        if(auto* p=processor.parameters.getParameter("angle"))
            p->setValueNotifyingHost(p->convertTo0to1(std::clamp((ny-.20f)/.60f,0.f,1.f)*180.f));
    }
    void ensureBackdrop(int scene,float pixelScale) {
        const auto a=artArea();
        const float ps=std::max(1.f,std::round(pixelScale*4.f)/4.f);
        const int w=juce::roundToInt(a.getWidth()*ps),h=juce::roundToInt(a.getHeight()*ps);
        if(w<8||h<8)return;
        if(scene==backdropScene && backdrop.getWidth()==w && backdrop.getHeight()==h)return;
        backdrop=juce::Image(juce::Image::ARGB,w,h,true);
        juce::Graphics ig(backdrop);
        ig.addTransform(juce::AffineTransform::scale(ps));
        drawBackdrop(ig,scene,a.getWidth(),a.getHeight());
        backdropScene=scene;
    }
    // Original line art, one motif per factory space. Deterministic, no randomness at runtime.
    static void drawBackdrop(juce::Graphics& g,int scene,float W,float H) {
        const auto lerp=[](float a,float b,float t){return a+(b-a)*t;};
        const auto rnd=[](int i,int k){const float v=std::sin(static_cast<float>(i)*12.9898f+static_cast<float>(k)*78.233f)*43758.5453f;return v-std::floor(v);};
        const auto stroke=[&](float a){g.setColour(Theme::green.withAlpha(a));};
        const auto fillc=[&](float a){g.setColour(Theme::greenDim.withAlpha(a));};
        const float floorY=H*.80f;
        const auto ground=[&](float y,float a){stroke(a);g.drawLine(0.f,y,W,y,1.2f);};
        switch(scene) {
        case 0: case 1: case 2: { // tunnel, sewer pipe, subway: receding frames toward the far (left) end
            const float vx=W*.12f;
            for(int i=0;i<9;++i) {
                const float t=static_cast<float>(i)/9.f,sc=std::pow(1.f-t,1.5f);
                const float w=W*sc*1.02f,h=H*sc*.96f,cx=lerp(W*.5f,vx,t),cy=lerp(H*.5f,H*.52f,t);
                fillc(.05f);
                if(scene==1){g.fillEllipse(cx-w*.5f,cy-h*.5f,w,h);stroke(.09f+.22f*t);g.drawEllipse(cx-w*.5f,cy-h*.5f,w,h,1.1f);}
                else{g.fillRoundedRectangle(cx-w*.5f,cy-h*.5f,w,h,12.f*sc);stroke(.09f+.22f*t);g.drawRoundedRectangle(cx-w*.5f,cy-h*.5f,w,h,12.f*sc,1.1f);}
            }
            if(scene==2){stroke(.32f);g.drawLine(W*.98f,H*.97f,vx+4.f,H*.56f,1.4f);g.drawLine(W*.66f,H*.97f,vx+2.f,H*.56f,1.4f);}
            if(scene==1){for(int i=0;i<5;++i){stroke(.18f);const float rx=W*(.25f+.14f*static_cast<float>(i));g.drawEllipse(rx,H*.86f+static_cast<float>(i%2)*5.f,26.f,5.f,1.f);}}
            if(scene==0){for(int i=0;i<6;++i){const float t=std::pow(static_cast<float>(i)/6.f,.8f);fillc(.9f);g.fillEllipse(lerp(W*.9f,vx,t),lerp(H*.10f,H*.46f,t),3.f,3.f);}}
            break; }
        case 3: { // cave: stalactites, stalagmites, irregular walls
            for(int i=0;i<12;++i){
                const float x=W*(static_cast<float>(i)+.5f)/12.f,hh=H*(.12f+.28f*rnd(i,1)),w=10.f+16.f*rnd(i,2);
                juce::Path p;p.addTriangle(x-w*.5f,0.f,x+w*.5f,0.f,x,hh);fillc(.35f);g.fillPath(p);stroke(.25f);g.strokePath(p,juce::PathStrokeType(1.f));
                const float hu=H*(.08f+.22f*rnd(i,3)),xu=x+(rnd(i,4)-.5f)*16.f;
                juce::Path q;q.addTriangle(xu-w*.5f,H,xu+w*.5f,H,xu,H-hu);fillc(.30f);g.fillPath(q);stroke(.2f);g.strokePath(q,juce::PathStrokeType(1.f));
            }
            break; }
        case 4: { // cellar / bunker: brick wall and low arch
            for(int r=0;r<9;++r){const float y=H*(.1f+.09f*static_cast<float>(r));stroke(.12f);g.drawLine(0.f,y,W,y,1.f);
                for(int c=0;c<14;++c){const float x=W*(static_cast<float>(c)+(r%2?.5f:0.f))/14.f;g.drawLine(x,y,x,y+H*.09f,1.f);}}
            juce::Path arch;arch.startNewSubPath(W*.62f,floorY);arch.lineTo(W*.62f,H*.45f);arch.quadraticTo(W*.74f,H*.12f,W*.86f,H*.45f);arch.lineTo(W*.86f,floorY);
            fillc(.25f);g.fillPath(arch);stroke(.4f);g.strokePath(arch,juce::PathStrokeType(1.5f));
            ground(floorY,.3f);break; }
        case 5: { // graveyard: moon, ground, headstones scattered in depth
            fillc(.9f);g.fillEllipse(W*.16f-26.f,H*.20f-26.f,52.f,52.f);stroke(.35f);g.fillEllipse(W*.16f-11.f,H*.20f-11.f,22.f,22.f);
            ground(H*.55f,.18f);ground(floorY,.28f);
            for(int i=0;i<13;++i){
                const float depth=rnd(i,1),by=lerp(H*.60f,H*.92f,depth),sz=lerp(11.f,26.f,depth),x=W*(.04f+.92f*rnd(i,2));
                fillc(.5f);g.fillRoundedRectangle(x,by-sz*1.3f,sz,sz*1.3f,sz*.45f);stroke(.3f);g.drawRoundedRectangle(x,by-sz*1.3f,sz,sz*1.3f,sz*.45f,1.1f);
                if(i%3==0){g.drawLine(x+sz*.5f,by-sz*1.05f,x+sz*.5f,by-sz*.45f,1.f);g.drawLine(x+sz*.3f,by-sz*.8f,x+sz*.7f,by-sz*.8f,1.f);}
            }
            break; }
        case 6: { // forest: trunks with canopy blobs
            ground(floorY,.22f);
            for(int i=0;i<16;++i){
                const float depth=rnd(i,1),x=W*rnd(i,2),w=lerp(3.f,10.f,depth),top=lerp(H*.30f,-H*.1f,depth),base=lerp(H*.66f,H*.96f,depth);
                fillc(.55f);g.fillRect(x,top,w,base-top);stroke(.14f+.14f*depth);g.drawRect(x,top,w,base-top,1.f);
                fillc(.22f);g.fillEllipse(x-w*2.5f,top-w*1.5f,w*6.f,w*5.f);
            }
            break; }
        case 7: { // canyon: strata walls
            juce::Path l,r;
            l.startNewSubPath(0.f,0.f);l.lineTo(W*.30f,0.f);l.lineTo(W*.22f,H*.28f);l.lineTo(W*.30f,H*.5f);l.lineTo(W*.20f,H*.72f);l.lineTo(W*.26f,floorY);l.lineTo(0.f,floorY);l.closeSubPath();
            r.startNewSubPath(W,0.f);r.lineTo(W*.72f,0.f);r.lineTo(W*.80f,H*.3f);r.lineTo(W*.74f,H*.55f);r.lineTo(W*.83f,H*.75f);r.lineTo(W*.78f,floorY);r.lineTo(W,floorY);r.closeSubPath();
            fillc(.30f);g.fillPath(l);g.fillPath(r);stroke(.4f);g.strokePath(l,juce::PathStrokeType(1.5f));g.strokePath(r,juce::PathStrokeType(1.5f));
            for(int i=1;i<6;++i){stroke(.10f);g.drawLine(0.f,H*.15f*static_cast<float>(i),W*.26f,H*.15f*static_cast<float>(i)+4.f,1.f);g.drawLine(W*.78f,H*.15f*static_cast<float>(i),W,H*.15f*static_cast<float>(i)+4.f,1.f);}
            ground(floorY,.3f);break; }
        case 8: { // open field: horizon, hills, grass
            const float hy=H*.52f;
            juce::Path hills;hills.startNewSubPath(0.f,hy);hills.quadraticTo(W*.25f,hy-H*.16f,W*.5f,hy);hills.quadraticTo(W*.75f,hy+H*.10f,W,hy-H*.06f);
            stroke(.3f);g.strokePath(hills,juce::PathStrokeType(1.4f));ground(hy+H*.12f,.12f);ground(floorY,.22f);
            for(int i=0;i<46;++i){const float x=W*rnd(i,1),y=lerp(hy+H*.16f,H*.97f,rnd(i,2)),h=lerp(3.f,10.f,(y-hy)/H);stroke(.20f);g.drawLine(x,y,x+1.5f,y-h,1.f);}
            break; }
        case 9: { // city alley: facades both sides, narrow sky
            for(int i=0;i<6;++i){
                const float bw=W*.12f,hh=H*(.45f+.4f*rnd(i,1));
                fillc(.35f);g.fillRect(W*.0f+bw*static_cast<float>(i)*.9f,H-hh,bw*.9f,hh);
                g.fillRect(W-bw*.9f*static_cast<float>(i+1),H-hh*(.7f+.3f*rnd(i,2)),bw*.9f,hh);
                for(int k=0;k<6;++k){stroke(.16f);g.fillRect(W*.0f+bw*static_cast<float>(i)*.9f+6.f+static_cast<float>(k%2)*12.f,H-hh+10.f+static_cast<float>(k/2)*22.f,5.f,8.f);}
            }
            ground(floorY,.3f);break; }
        case 10: { // parking garage: low slab and columns
            stroke(.30f);g.drawLine(0.f,H*.16f,W,H*.16f,1.5f);ground(floorY,.3f);
            for(int i=0;i<8;++i){
                const float t=static_cast<float>(i)/8.f,x=W*(.92f-.84f*std::pow(t,.85f)),w=lerp(18.f,6.f,t),top=lerp(H*.16f,H*.30f,t),bot=lerp(floorY,H*.62f,t);
                fillc(.4f);g.fillRect(x,top,w,bot-top);stroke(.3f);g.drawRect(x,top,w,bot-top,1.f);
            }
            break; }
        case 11: { // stairwell: flights of steps, handrail
            juce::Path steps;steps.startNewSubPath(W*.96f,H*.92f);
            for(int i=0;i<10;++i){const float x=W*(.96f-.085f*static_cast<float>(i+1)),y=H*(.92f-.075f*static_cast<float>(i));steps.lineTo(x+W*.085f,y-H*.075f);steps.lineTo(x,y-H*.075f);}
            stroke(.4f);g.strokePath(steps,juce::PathStrokeType(1.5f));
            stroke(.22f);g.drawLine(W*.96f,H*.72f,W*.10f,H*.00f,1.2f);
            break; }
        case 12: { // cathedral: nested pointed arches
            for(int i=0;i<5;++i){
                const float t=static_cast<float>(i)/5.f,w=W*(.70f-.11f*static_cast<float>(i)),cx=lerp(W*.5f,W*.30f,t),top=H*(.05f+.12f*t),spring=H*(.50f+.02f*t),fl=lerp(floorY,H*.62f,t);
                juce::Path a;a.startNewSubPath(cx-w*.5f,fl);a.lineTo(cx-w*.5f,spring);
                a.cubicTo(cx-w*.5f,spring-H*.2f,cx-w*.12f,top+H*.12f,cx,top);a.cubicTo(cx+w*.12f,top+H*.12f,cx+w*.5f,spring-H*.2f,cx+w*.5f,spring);a.lineTo(cx+w*.5f,fl);
                stroke(.12f+.25f*t);g.strokePath(a,juce::PathStrokeType(1.3f));
            }
            ground(floorY,.25f);break; }
        case 13: { // empty pool: tiled basin and ladder
            for(int i=0;i<10;++i){const float x=W*(static_cast<float>(i)/9.f);stroke(.10f);g.drawLine(x,floorY,lerp(x,W*.5f,.5f),H*.35f,1.f);}
            for(int i=0;i<6;++i){const float y=lerp(floorY,H*.35f,static_cast<float>(i)/5.f);stroke(.10f);g.drawLine(lerp(0.f,W*.25f,(floorY-y)/(floorY-H*.35f)),y,lerp(W,W*.75f,(floorY-y)/(floorY-H*.35f)),y,1.f);}
            ground(floorY,.3f);stroke(.4f);g.drawLine(W*.9f,H*.15f,W*.9f,H*.55f,1.5f);g.drawLine(W*.94f,H*.15f,W*.94f,H*.55f,1.5f);
            for(int i=0;i<4;++i)g.drawLine(W*.9f,H*(.22f+.09f*static_cast<float>(i)),W*.94f,H*(.22f+.09f*static_cast<float>(i)),1.f);
            break; }
        default: { // warehouse: racking and crates
            for(int c=0;c<6;++c){
                const float x=W*(.08f+.15f*static_cast<float>(c));stroke(.3f);g.drawLine(x,H*.2f,x,floorY,1.4f);
                for(int r=0;r<3;++r){const float y=H*(.35f+.2f*static_cast<float>(r));g.drawLine(x,y,x+W*.12f,y,1.2f);
                    if(rnd(c,r)>.35f){fillc(.5f);g.fillRect(x+4.f+rnd(c,r+5)*10.f,y-14.f,16.f,14.f);}}
            }
            ground(floorY,.3f);break; }
        }
    }
    static float hash01(int i,int k) {
        const float v=std::sin(static_cast<float>(i)*12.9898f+static_cast<float>(k)*78.233f)*43758.5453f;
        return v-std::floor(v);
    }
    void timerCallback() override {
        drift=std::fmod(drift+.004f,1000.f);
        pulsePhase=std::fmod(pulsePhase+.016f+.025f*processor.meterEffort.load(std::memory_order_relaxed),1.f);
        repaint();
    }
    SkellAfarAudioProcessor& processor;
    float pulsePhase=0.f,drift=0.f;
    juce::Image backdrop;
    int backdropScene=-1;
};
}
