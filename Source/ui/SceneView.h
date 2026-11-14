#pragma once
#include "Theme.h"
#include "../PluginProcessor.h"
#include <algorithm>
#include <cmath>

namespace afar::ui {
// Scene stage. The voice is a skeleton: distance is depth (far = small and up at the horizon, near =
// large in the foreground next to the microphone), it walks whenever the distance changes (drags,
// automation or Walk paths), its jaw follows the output level and its arms open with vocal effort.
// The per-scene backdrop is rendered once into a cached image at physical-pixel density.
class SceneView final : public juce::Component, public juce::SettableTooltipClient, private juce::Timer {
public:
    explicit SceneView(SkellAfarAudioProcessor& p):processor(p) {
        startTimerHz(30);setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        setTooltip("Drag the skeleton: up/down moves it back and forward (distance), left/right turns it away (angle).");
    }
    void resized() override {backdropScene=-1;}
    void paint(juce::Graphics& g) override {
        const auto b=getLocalBounds().toFloat();
        panel(g,b);
        const int scene=std::clamp(static_cast<int>(processor.parameters.getRawParameterValue("space")->load()),0,static_cast<int>(spaces.size())-1);
        const auto& s=getSpace(scene);
        const auto art=artArea();
        sectionTitle(g,juce::String(s.name).toUpperCase(),14.f,10.f,b.getWidth()-120.f,16.f);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(11.f)));
        g.drawText(s.description,24.f,31.f,b.getWidth()-120.f,14.f,juce::Justification::centredLeft);
        const char* cat=s.category==Category::Underground?"UNDERGROUND":s.category==Category::Outdoor?"OUTDOOR":"INDOOR";
        const juce::Rectangle<float> pill(b.getRight()-104.f,12.f,90.f,17.f);
        g.setColour(Theme::background);g.fillRoundedRectangle(pill,8.5f);
        g.setColour(Theme::greenDim.brighter(.3f));g.drawRoundedRectangle(pill,8.5f,1.f);
        g.setColour(Theme::greenLight);g.setFont(titleFont(9.f));g.drawText(cat,pill,juce::Justification::centred);
        juce::Path clip;clip.addRoundedRectangle(art,7.f);
        g.saveState();
        g.reduceClipRegion(clip);
        juce::ColourGradient sky(juce::Colour(0xff0b1411),art.getX(),art.getY(),juce::Colour(0xff040706),art.getX(),art.getBottom(),false);
        g.setGradientFill(sky);g.fillRect(art);
        ensureBackdrop(scene,g.getInternalContext().getPhysicalPixelScaleFactor());
        if(backdrop.isValid()) g.drawImage(backdrop,art);
        for(int i=0;i<16;++i) {   // drifting dust motes
            const float sp=.25f+.75f*hash01(i,3);
            const float px=art.getX()+art.getWidth()*std::fmod(hash01(i,1)+drift*sp,1.f);
            const float py=art.getY()+art.getHeight()*(.10f+.80f*hash01(i,2))+3.f*std::sin(drift*9.f*sp+static_cast<float>(i));
            g.setColour(Theme::greenLight.withAlpha(.07f+.10f*sp));
            g.fillEllipse(px-1.f,py-1.f,2.f*sp+1.f,2.f*sp+1.f);
        }
        const auto& pv=processor.parameters;
        const float d=std::max(.05f,processor.meterDistance.load(std::memory_order_relaxed));
        const float dn=distanceNormalized(d);
        const float angle=std::clamp(pv.getRawParameterValue("angle")->load(),0.f,180.f);
        const float effort=std::clamp(pv.getRawParameterValue("effort")->load(),0.f,1.f);
        const float level=std::clamp(processor.meterPeak.load(std::memory_order_relaxed),0.f,1.f);
        const float cx=art.getCentreX();
        // ---- ground rings: how far each distance is on the stage
        g.setFont(juce::Font(juce::FontOptions(9.f)));
        for(const float m:{.3f,1.f,3.f,10.f,30.f,100.f}) {
            const float dm=distanceNormalized(m),sc=scaleFor(dm),y=footYFor(dm);
            const float rw=art.getWidth()*(.18f+.30f*sc);
            g.setColour(Theme::green.withAlpha(.10f+.12f*sc));
            g.drawEllipse(cx-rw,y-4.f*sc-1.f,rw*2.f,8.f*sc+2.f,1.f);
            g.setColour(Theme::muted.withAlpha(.35f+.35f*sc));
            g.drawText(m<1.f?juce::String("0.3 m"):juce::String(static_cast<int>(m))+" m",cx+rw-34.f,y-6.f,34.f,11.f,juce::Justification::centredRight);
        }
        // ---- skeleton
        const float side=lateralSign*(angle/180.f)*art.getWidth()*.40f;
        const float sx=cx+side,foot=footYFor(dn),sc=scaleFor(dn);
        const float micX=cx,micY=art.getBottom()-9.f;
        const float hx=sx,hy=foot-92.f*sc;
        g.setColour(Theme::green.withAlpha(.22f));g.drawLine(hx,hy,micX,micY-14.f,1.3f);   // direct path
        for(int i=0;i<3;++i) {   // voice waves leaving the mouth
            const float progress=std::fmod(pulsePhase+static_cast<float>(i)/3.f,1.f);
            const float rr=(6.f+progress*60.f)*(.55f+.45f*sc);
            g.setColour(Theme::green.withAlpha((.18f+.20f*level)*(1.f-progress)));
            g.drawEllipse(hx-rr,hy+2.f*sc-rr,rr*2.f,rr*2.f,1.1f);
        }
        drawSkeleton(g,sx,foot,sc,angle*pi/180.f,walkPhase,moveAmt,effort,level,std::sin(breathPhase));
        // microphone in the foreground
        g.setColour(Theme::bone);g.fillRoundedRectangle(micX-5.f,micY-26.f,10.f,20.f,4.f);
        g.setColour(Theme::muted);g.drawLine(micX,micY-6.f,micX,micY+8.f,1.5f);g.drawLine(micX-9.f,micY+8.f,micX+9.f,micY+8.f,1.5f);
        g.setFont(juce::Font(juce::FontOptions(9.f)));g.drawText("MIC",micX+9.f,micY-24.f,30.f,11.f,juce::Justification::centredLeft);
        g.restoreState();
        g.setColour(Theme::greenDim.withAlpha(.9f));g.drawRoundedRectangle(art,7.f,1.f);
        const float dr=processor.meterDrr.load(std::memory_order_relaxed);
        const float cy=b.getBottom()-31.f;
        const float sizeNow=pv.getRawParameterValue("size")->load();
        chip(g,14.f,cy,"DIST",meters(d));
        chip(g,126.f,cy,"DRR",juce::String(dr,1)+" dB");
        chip(g,238.f,cy,"RT60",juce::String(s.rt60Seconds,1)+" s");
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
        return {13.f,52.f,static_cast<float>(getWidth())-26.f,static_cast<float>(getHeight())-52.f-52.f};
    }
    // depth mapping: dn 0 (near) .. 1 (far) -> stage position and size
    float footYFor(float dn) const {
        const auto a=artArea();const float nearY=a.getBottom()-20.f,farY=a.getY()+a.getHeight()*.42f;
        return nearY+(farY-nearY)*std::pow(std::clamp(dn,0.f,1.f),.75f);
    }
    static float scaleFor(float dn) {return 1.f-.76f*std::pow(std::clamp(dn,0.f,1.f),.6f);}
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
        const float nearY=a.getBottom()-20.f,farY=a.getY()+a.getHeight()*.42f;
        const float yNorm=std::clamp((nearY-(static_cast<float>(e.y)+46.f))/std::max(1.f,nearY-farY),0.f,1.f);
        if(auto* p=processor.parameters.getParameter("distance"))
            p->setValueNotifyingHost(p->convertTo0to1(distanceMeters(std::pow(yNorm,1.f/.75f))));
        const float rel=(static_cast<float>(e.x)-a.getCentreX())/(a.getWidth()*.40f);
        lateralSign=rel<0.f?-1.f:1.f;
        if(auto* p=processor.parameters.getParameter("angle"))
            p->setValueNotifyingHost(p->convertTo0to1(std::clamp(std::abs(rel),0.f,1.f)*180.f));
    }
    // Stylised walking skeleton, feet at footY, ~100 units tall at s = 1.
    static void drawSkeleton(juce::Graphics& g,float cx,float footY,float s,float turn,float phase,float walk,
                             float effort,float level,float breath) {
        const float ts=std::max(.32f,std::abs(std::cos(turn)));
        const bool front=std::cos(turn)>=0.f;
        const float thick=std::max(1.1f,2.4f*s);
        const auto limb=[&](float x0,float y0,float x1,float y1) {
            g.setColour(Theme::green.withAlpha(.10f));g.drawLine(x0,y0,x1,y1,thick*3.2f);
            g.setColour(Theme::bone.withAlpha(.92f));g.drawLine(x0,y0,x1,y1,thick);
        };
        g.setColour(juce::Colours::black.withAlpha(.35f));g.fillEllipse(cx-20.f*s,footY-3.f*s,40.f*s,6.f*s);
        const float bob=std::abs(std::sin(phase))*2.5f*s*walk;
        const float hipY=footY-46.f*s-bob,shY=footY-76.f*s-bob+breath*.6f*s,headY=footY-92.f*s-bob;
        for(int sd=-1;sd<=1;sd+=2) {   // legs: opposite swing, knee lift while stepping
            const float ph=phase+(sd>0?0.f:pi);
            const float th=std::sin(ph)*.32f*walk,lift=std::max(0.f,std::sin(ph))*7.f*s*walk;
            const float hipX=cx+static_cast<float>(sd)*4.5f*s*ts;
            const float kx=hipX+std::sin(th)*22.f*s,ky=hipY+std::cos(th)*22.f*s-lift*.5f;
            const float sh=th-std::max(0.f,std::sin(ph+1.2f))*.7f*walk;
            const float fx=kx+std::sin(sh)*22.f*s,fy=ky+std::cos(sh)*22.f*s-lift*.5f;
            limb(hipX,hipY,kx,ky);limb(kx,ky,fx,fy);limb(fx,fy,fx+3.f*s*static_cast<float>(sd),fy);
        }
        limb(cx-5.5f*s*ts,hipY,cx+5.5f*s*ts,hipY);
        limb(cx,hipY,cx,shY);
        for(int i=0;i<4;++i) {   // ribcage
            const float ry=shY+(4.f+static_cast<float>(i)*4.6f)*s,w=(11.f-static_cast<float>(i)*1.8f)*s*ts*(1.f+breath*.04f);
            juce::Path r;r.startNewSubPath(cx-w,ry);r.quadraticTo(cx,ry+3.5f*s,cx+w,ry);
            g.setColour(Theme::bone.withAlpha(.8f));g.strokePath(r,juce::PathStrokeType(thick*.8f));
        }
        limb(cx-9.f*s*ts,shY,cx+9.f*s*ts,shY);
        const float spread=std::max(0.f,effort-.55f)*1.2f,tuck=std::max(0.f,.35f-effort);   // shout opens, whisper tucks in
        for(int sd=-1;sd<=1;sd+=2) {   // arms
            const float swing=std::sin(phase+(sd>0?pi:0.f))*.5f*walk;
            const float a1=static_cast<float>(sd)*(.10f+spread-tuck*.25f)+swing*.7f;
            const float shx=cx+static_cast<float>(sd)*9.f*s*ts;
            const float ex=shx+std::sin(a1)*15.f*s,ey=shY+std::cos(a1)*15.f*s;
            const float a2=a1+static_cast<float>(-sd)*(.30f+tuck*1.2f)+swing*.3f;
            limb(shx,shY,ex,ey);limb(ex,ey,ex+std::sin(a2)*14.f*s,ey+std::cos(a2)*14.f*s);
        }
        limb(cx,headY+9.6f*s,cx,shY);   // neck
        const float k=s*1.2f;   // skull is drawn larger than the body scale so it stays readable when far
        const float jaw=std::clamp(effort*.45f+level*.9f,0.f,1.f);
        g.setColour(Theme::green.withAlpha(.12f));g.drawEllipse(cx-11.f*k,headY-11.f*k,22.f*k,20.f*k,thick*3.f);
        g.setColour(Theme::bone);g.drawEllipse(cx-9.f*k,headY-9.f*k,18.f*k,16.f*k,thick);
        if(front) {
            g.setColour(Theme::background.withAlpha(.95f));
            g.fillEllipse(cx-6.f*k,headY-3.f*k,5.f*k,5.6f*k);g.fillEllipse(cx+1.f*k,headY-3.f*k,5.f*k,5.6f*k);
            g.setColour(Theme::green.withAlpha(.3f+.7f*level));
            g.fillEllipse(cx-4.4f*k,headY-1.4f*k,2.4f*k,2.4f*k);g.fillEllipse(cx+2.6f*k,headY-1.4f*k,2.4f*k,2.4f*k);
            juce::Path nose;nose.addTriangle(cx-1.2f*k,headY+2.5f*k,cx+1.2f*k,headY+2.5f*k,cx,headY+.6f*k);
            g.setColour(Theme::bone.withAlpha(.9f));g.fillPath(nose);
            const float jy=headY+7.f*k+jaw*5.f*k;   // jaw drops with the voice
            g.drawLine(cx-5.f*k,headY+6.f*k,cx-4.f*k,jy,thick*.8f);g.drawLine(cx+5.f*k,headY+6.f*k,cx+4.f*k,jy,thick*.8f);
            g.drawLine(cx-4.f*k,jy,cx+4.f*k,jy,thick*.8f);
            for(int tooth=-2;tooth<=2;++tooth) {
                const float tx=cx+static_cast<float>(tooth)*1.7f*k;
                g.drawLine(tx,headY+6.f*k,tx,headY+8.f*k,std::max(1.f,thick*.5f));
            }
        } else {
            g.setColour(Theme::bone.withAlpha(.5f));g.drawLine(cx,headY-9.f*k,cx,headY+7.f*k,thick*.7f);   // cranial seam
        }
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
            const float vx=W*.5f;
            for(int i=0;i<9;++i) {
                const float t=static_cast<float>(i)/9.f,sc=std::pow(1.f-t,1.5f);
                const float w=W*sc*1.02f,h=H*sc*.96f,cx=vx,cy=lerp(H*.5f,H*.46f,t);
                fillc(.05f);
                if(scene==1){g.fillEllipse(cx-w*.5f,cy-h*.5f,w,h);stroke(.09f+.22f*t);g.drawEllipse(cx-w*.5f,cy-h*.5f,w,h,1.1f);}
                else{g.fillRoundedRectangle(cx-w*.5f,cy-h*.5f,w,h,12.f*sc);stroke(.09f+.22f*t);g.drawRoundedRectangle(cx-w*.5f,cy-h*.5f,w,h,12.f*sc,1.1f);}
            }
            if(scene==2){stroke(.32f);g.drawLine(W*.78f,H*.98f,vx+3.f,H*.52f,1.4f);g.drawLine(W*.22f,H*.98f,vx-3.f,H*.52f,1.4f);}
            if(scene==1){for(int i=0;i<5;++i){stroke(.18f);const float rx=W*(.25f+.14f*static_cast<float>(i));g.drawEllipse(rx,H*.86f+static_cast<float>(i%2)*5.f,26.f,5.f,1.f);}}
            if(scene==0){for(int i=0;i<6;++i){const float t=std::pow(static_cast<float>(i)/6.f,.8f);fillc(.9f);g.fillEllipse(vx-1.5f,lerp(H*.02f,H*.40f,t),3.f,3.f);}}
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
                const float t=static_cast<float>(i)/8.f,w=lerp(18.f,6.f,t),top=lerp(H*.16f,H*.30f,t),bot=lerp(floorY,H*.62f,t);
                for(int sd=-1;sd<=1;sd+=2){const float x=W*.5f+static_cast<float>(sd)*W*(.46f-.40f*std::pow(t,.8f))-w*.5f;
                fillc(.4f);g.fillRect(x,top,w,bot-top);stroke(.3f);g.drawRect(x,top,w,bot-top,1.f);}
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
                const float t=static_cast<float>(i)/5.f,w=W*(.70f-.11f*static_cast<float>(i)),cx=W*.5f,top=H*(.05f+.12f*t),spring=H*(.50f+.02f*t),fl=lerp(floorY,H*.62f,t);
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
        case 14: { // warehouse: racking and crates
            for(int c=0;c<6;++c){
                const float x=W*(.08f+.15f*static_cast<float>(c));stroke(.3f);g.drawLine(x,H*.2f,x,floorY,1.4f);
                for(int r=0;r<3;++r){const float y=H*(.35f+.2f*static_cast<float>(r));g.drawLine(x,y,x+W*.12f,y,1.2f);
                    if(rnd(c,r)>.35f){fillc(.5f);g.fillRect(x+4.f+rnd(c,r+5)*10.f,y-14.f,16.f,14.f);}}
            }
            ground(floorY,.3f);break; }
        case 15: { // bathroom: tiled wall grid, tub
            for(int i=1;i<16;++i){stroke(.10f);g.drawLine(W*static_cast<float>(i)/16.f,0.f,W*static_cast<float>(i)/16.f,floorY,1.f);}
            for(int i=1;i<8;++i){stroke(.10f);g.drawLine(0.f,floorY*static_cast<float>(i)/8.f,W,floorY*static_cast<float>(i)/8.f,1.f);}
            fillc(.45f);g.fillRoundedRectangle(W*.62f,H*.56f,W*.32f,H*.24f,10.f);stroke(.4f);g.drawRoundedRectangle(W*.62f,H*.56f,W*.32f,H*.24f,10.f,1.4f);
            g.drawEllipse(W*.12f,H*.50f,W*.10f,H*.07f,1.2f);g.drawLine(W*.17f,H*.57f,W*.17f,floorY,1.4f);
            ground(floorY,.3f);break; }
        case 16: { // crypt: dark nested arches and wall niches
            for(int i=0;i<5;++i){
                const float t=static_cast<float>(i)/5.f,w=W*(.62f-.10f*static_cast<float>(i)),cx=W*.5f,top=H*(.08f+.12f*t),spring=H*(.52f+.02f*t),fl=lerp(floorY,H*.64f,t);
                juce::Path a;a.startNewSubPath(cx-w*.5f,fl);a.lineTo(cx-w*.5f,spring);
                a.cubicTo(cx-w*.5f,spring-H*.2f,cx-w*.12f,top+H*.1f,cx,top);a.cubicTo(cx+w*.12f,top+H*.1f,cx+w*.5f,spring-H*.2f,cx+w*.5f,spring);a.lineTo(cx+w*.5f,fl);
                stroke(.10f+.22f*t);g.strokePath(a,juce::PathStrokeType(1.3f));
            }
            for(int i=0;i<4;++i){fillc(.5f);const float nx=W*(.06f+.06f*static_cast<float>(i%2)),ny=H*(.30f+.14f*static_cast<float>(i/2));
                g.fillRoundedRectangle(nx,ny,W*.05f,H*.10f,4.f);g.fillRoundedRectangle(W-nx-W*.05f,ny,W*.05f,H*.10f,4.f);}
            ground(floorY,.28f);break; }
        case 17: { // elevator shaft: looking up a metal tube, cables and landings
            for(int i=0;i<9;++i){
                const float t=static_cast<float>(i)/9.f,sc=std::pow(1.f-t,1.4f),w=W*.46f*sc,h=H*.92f*sc,cy=lerp(H*.55f,H*.18f,t);
                fillc(.05f);g.fillRect(W*.5f-w*.5f,cy-h*.5f,w,h);stroke(.08f+.25f*t);g.drawRect(W*.5f-w*.5f,cy-h*.5f,w,h,1.1f);
            }
            stroke(.35f);g.drawLine(W*.46f,0.f,W*.495f,H*.30f,1.2f);g.drawLine(W*.54f,0.f,W*.505f,H*.30f,1.2f);
            ground(floorY,.3f);break; }
        case 18: { // snowfield: horizon, pines, drifting flakes
            const float hy=H*.50f;
            ground(hy,.25f);ground(hy+H*.10f,.10f);
            for(int i=0;i<9;++i){const float x=W*rnd(i,1),h=lerp(8.f,26.f,rnd(i,2));juce::Path t;t.addTriangle(x-h*.35f,hy,x+h*.35f,hy,x,hy-h);fillc(.4f);g.fillPath(t);}
            for(int i=0;i<60;++i){stroke(.20f);g.fillEllipse(W*rnd(i,3),H*rnd(i,4),2.f,2.f);}
            break; }
        case 19: { // arena: curved stands, floor oval, light towers
            for(int i=0;i<7;++i){const float y=H*(.12f+.09f*static_cast<float>(i));stroke(.08f+.05f*static_cast<float>(i));
                juce::Path l;l.startNewSubPath(0.f,y+H*.12f);l.quadraticTo(W*.22f,y-H*.06f,W*.36f,y);g.strokePath(l,juce::PathStrokeType(1.1f));
                juce::Path r;r.startNewSubPath(W,y+H*.12f);r.quadraticTo(W*.78f,y-H*.06f,W*.64f,y);g.strokePath(r,juce::PathStrokeType(1.1f));}
            stroke(.3f);g.drawEllipse(W*.18f,H*.62f,W*.64f,H*.22f,1.3f);
            for(int sd=-1;sd<=1;sd+=2){const float tx=W*.5f+static_cast<float>(sd)*W*.43f;g.drawLine(tx,H*.05f,tx,H*.50f,1.4f);fillc(.9f);g.fillEllipse(tx-4.f,H*.03f,8.f,8.f);}
            break; }
        default: break;
        }
    }
    static float hash01(int i,int k) {
        const float v=std::sin(static_cast<float>(i)*12.9898f+static_cast<float>(k)*78.233f)*43758.5453f;
        return v-std::floor(v);
    }
    void timerCallback() override {
        drift=std::fmod(drift+.004f,1000.f);
        pulsePhase=std::fmod(pulsePhase+.016f+.025f*processor.meterEffort.load(std::memory_order_relaxed),1.f);
        // walking: animate while the (smoothed) distance is actually changing
        const float d=std::max(.05f,processor.meterDistance.load(std::memory_order_relaxed));
        const float relChange=std::abs(d-lastDistance)/std::max(.05f,lastDistance);
        lastDistance=d;
        moveAmt+=.15f*(std::clamp(relChange*60.f,0.f,1.f)-moveAmt);
        walkPhase=std::fmod(walkPhase+moveAmt*.30f,2.f*pi);
        breathPhase=std::fmod(breathPhase+.07f,2.f*pi);
        repaint();
    }
    SkellAfarAudioProcessor& processor;
    float pulsePhase=0.f,drift=0.f,lateralSign=1.f,lastDistance=3.f,moveAmt=0.f,walkPhase=0.f,breathPhase=0.f;
    juce::Image backdrop;
    int backdropScene=-1;
};
}
