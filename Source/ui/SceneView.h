#pragma once
#include "Theme.h"
#include "Backdrops.h"
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
        const auto tint=s.category==Category::Underground?Theme::ice:s.category==Category::Outdoor?Theme::green:Theme::amber;
        g.setColour(juce::Colours::black.withAlpha(.35f));g.fillRoundedRectangle(pill,8.5f);
        g.setColour(tint.withAlpha(.45f));g.drawRoundedRectangle(pill.reduced(.5f),8.5f,1.f);
        g.setColour(tint);g.setFont(titleFont(9.f));g.drawText(cat,pill,juce::Justification::centred);
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
            g.setColour(Theme::green.withAlpha(.05f+.09f*sc));
            g.drawEllipse(cx-rw,y-4.f*sc-1.f,rw*2.f,8.f*sc+2.f,1.f);
            g.setColour(Theme::muted.withAlpha(.25f+.30f*sc));
            g.drawText(m<1.f?juce::String("0.3 m"):juce::String(static_cast<int>(m))+" m",cx+rw-34.f,y-6.f,34.f,11.f,juce::Justification::centredRight);
        }
        // ---- skeleton
        const float side=lateralSign*(angle/180.f)*art.getWidth()*.40f;
        const float unit=std::clamp(std::pow(art.getHeight()/228.f,1.4f),.8f,2.2f);   // the skeleton grows with the stage (SIMPLE mode)
        const float sx=cx+side,foot=footYFor(dn),sc=scaleFor(dn)*unit;
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
        {   // studio condenser: steel grille with a lit edge, shock-mount ring, stand and soft floor shadow
            const float mu=unit;
            glow(g,micX,micY+8.f*mu,18.f*mu,juce::Colours::black,2.f);
            g.setColour(Theme::muted);g.drawLine(micX,micY-6.f*mu,micX,micY+8.f*mu,1.6f);g.drawLine(micX-10.f*mu,micY+8.f*mu,micX+10.f*mu,micY+8.f*mu,1.6f);
            const juce::Rectangle<float> body(micX-5.5f*mu,micY-28.f*mu,11.f*mu,22.f*mu);
            juce::ColourGradient steel(juce::Colour(0xfff4f1e6),body.getX(),body.getY(),juce::Colour(0xff7d8480),body.getRight(),body.getBottom(),false);
            g.setGradientFill(steel);g.fillRoundedRectangle(body,5.f*mu);
            g.setColour(juce::Colours::black.withAlpha(.28f));
            for(int i=1;i<6;++i) g.drawLine(body.getX()+1.f,body.getY()+body.getHeight()*static_cast<float>(i)/6.f,body.getRight()-1.f,body.getY()+body.getHeight()*static_cast<float>(i)/6.f,.8f);
            g.setColour(juce::Colours::white.withAlpha(.35f));g.drawLine(body.getX()+1.8f*mu,body.getY()+4.f*mu,body.getX()+1.8f*mu,body.getBottom()-4.f*mu,1.1f);
            g.setColour(Theme::muted);g.drawRoundedRectangle(micX-7.f*mu,micY-9.f*mu,14.f*mu,5.f*mu,2.f,1.2f);
            g.setColour(Theme::faint);g.setFont(titleFont(8.5f));g.drawText("MIC",micX+10.f*mu,micY-24.f*mu,30.f,11.f,juce::Justification::centredLeft);
        }
        g.restoreState();
        {   // glass edge: shadow falling from the top, hairline frame, faint lower lip
            juce::ColourGradient top(juce::Colours::black.withAlpha(.45f),art.getX(),art.getY(),juce::Colours::black.withAlpha(0.f),art.getX(),art.getY()+16.f,false);
            g.setGradientFill(top);g.fillRect(art.withHeight(16.f));
            g.setColour(Theme::line);g.drawRoundedRectangle(art.reduced(.5f),7.f,1.f);
        }
        const float dr=processor.meterDrr.load(std::memory_order_relaxed);
        const float cy=b.getBottom()-31.f;
        const float sizeNow=pv.getRawParameterValue("size")->load();
        const float rowW=4.f*104.f+3.f*8.f,x0=std::max(14.f,(static_cast<float>(getWidth())-rowW)*.5f);   // centred on wide stages
        chip(g,x0,cy,"DIST",meters(d));
        chip(g,x0+112.f,cy,"DRR",juce::String(dr,1)+" dB");
        chip(g,x0+224.f,cy,"RT60",juce::String(s.rt60Seconds,1)+" s");
        chip(g,x0+336.f,cy,"ECHO",juce::String(echoDelayMs(s.firstEchoMeters*sizeNow),0)+" ms");
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
        inset(g,r,6.f);
        g.setColour(Theme::faint);g.setFont(titleFont(8.5f));
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
    // Walking skeleton, feet at footY, ~100 units tall at s = 1. Bones are tapered shafts with joint heads, lit from the
    // upper left; the skull has modelled sockets that glow with the output level and a mandible that drops with the voice.
    static void drawSkeleton(juce::Graphics& g,float cx,float footY,float s,float turn,float phase,float walk,
                             float effort,float level,float breath) {
        const float ts=std::max(.34f,std::abs(std::cos(turn)));
        const bool front=std::cos(turn)>=0.f;
        const float thick=std::max(1.1f,2.4f*s);
        const juce::Colour hi(0xfff2efe2),mid(0xffd2cdbb),lo(0xff86826f),shade(0xff3a382f);
        const auto W=[&](float v){return std::max(.7f,v*s);};
        // one tapered bone with a lit edge and rounded joint heads
        const auto bone=[&](float x0,float y0,float x1,float y1,float w0,float w1) {
            const float dx=x1-x0,dy=y1-y0,len=std::max(.001f,std::sqrt(dx*dx+dy*dy)),nx=-dy/len,ny=dx/len;
            const float a=W(w0),b=W(w1);
            g.setColour(Theme::green.withAlpha(.06f));g.drawLine(x0,y0,x1,y1,(a+b)*3.4f);
            juce::Path p;p.startNewSubPath(x0+nx*a,y0+ny*a);p.lineTo(x1+nx*b,y1+ny*b);p.lineTo(x1-nx*b,y1-ny*b);p.lineTo(x0-nx*a,y0-ny*a);p.closeSubPath();
            g.setColour(mid);g.fillPath(p);
            g.setColour(hi.withAlpha(.75f));g.drawLine(x0-nx*a*.45f,y0-ny*a*.45f,x1-nx*b*.45f,y1-ny*b*.45f,std::max(.6f,(a+b)*.35f));
            g.setColour(shade.withAlpha(.55f));g.drawLine(x0+nx*a*.85f,y0+ny*a*.85f,x1+nx*b*.85f,y1+ny*b*.85f,std::max(.5f,(a+b)*.22f));
            g.setColour(hi);g.fillEllipse(x0-a*1.45f,y0-a*1.45f,a*2.9f,a*2.9f);g.fillEllipse(x1-b*1.45f,y1-b*1.45f,b*2.9f,b*2.9f);
            g.setColour(shade.withAlpha(.35f));g.drawEllipse(x0-a*1.45f,y0-a*1.45f,a*2.9f,a*2.9f,.6f);g.drawEllipse(x1-b*1.45f,y1-b*1.45f,b*2.9f,b*2.9f,.6f);
        };
        // contact shadow and a soft aura that breathes with the voice
        glow(g,cx,footY,30.f*s,juce::Colours::black,2.6f);
        const float bob=std::abs(std::sin(phase))*2.5f*s*walk;
        const float hipY=footY-46.f*s-bob,shY=footY-76.f*s-bob+breath*.6f*s,headY=footY-92.f*s-bob;
        glow(g,cx,(hipY+shY)*.5f,46.f*s,Theme::green,.55f+1.4f*level);
        for(int sd=-1;sd<=1;sd+=2) {   // legs: opposite swing, knee lift while stepping
            const float ph=phase+(sd>0?0.f:pi);
            const float th=std::sin(ph)*.32f*walk,lift=std::max(0.f,std::sin(ph))*7.f*s*walk;
            const float hipX=cx+static_cast<float>(sd)*4.5f*s*ts;
            const float kx=hipX+std::sin(th)*22.f*s,ky=hipY+std::cos(th)*22.f*s-lift*.5f;
            const float sh=th-std::max(0.f,std::sin(ph+1.2f))*.7f*walk;
            const float fx=kx+std::sin(sh)*22.f*s,fy=ky+std::cos(sh)*22.f*s-lift*.5f;
            bone(hipX,hipY,kx,ky,2.1f,1.6f);bone(kx,ky,fx,fy,1.6f,1.1f);
            juce::Path foot;foot.startNewSubPath(fx-1.6f*s,fy);foot.lineTo(fx+7.f*s*static_cast<float>(sd)*(front?1.f:1.2f),fy+1.5f*s);foot.lineTo(fx+6.f*s*static_cast<float>(sd),fy+3.2f*s);foot.lineTo(fx-2.f*s,fy+2.4f*s);foot.closeSubPath();
            g.setColour(mid);g.fillPath(foot);g.setColour(shade.withAlpha(.5f));g.strokePath(foot,juce::PathStrokeType(.6f));
        }
        // pelvis: two iliac wings and the sacrum
        for(int sd=-1;sd<=1;sd+=2) {
            const float o=static_cast<float>(sd)*ts;juce::Path w;
            w.startNewSubPath(cx,hipY-3.f*s);w.quadraticTo(cx+o*9.f*s,hipY-6.f*s,cx+o*9.5f*s,hipY-1.f*s);w.quadraticTo(cx+o*8.f*s,hipY+4.f*s,cx+o*3.5f*s,hipY+4.5f*s);w.lineTo(cx,hipY+3.f*s);w.closeSubPath();
            g.setColour(mid);g.fillPath(w);g.setColour(hi.withAlpha(.55f));g.strokePath(w,juce::PathStrokeType(.7f));
        }
        g.setColour(lo);g.fillEllipse(cx-2.f*s,hipY-2.f*s,4.f*s,6.f*s);
        for(int i=0;i<10;++i) {   // vertebrae: lumbar, thoracic, cervical, slight S-curve
            const float t=static_cast<float>(i)/9.f,y=hipY-3.f*s-(hipY-shY-3.f*s)*t,off=std::sin(t*pi)*1.3f*s*(front?0.f:1.f);
            const float w=(3.8f-1.2f*t)*s*(front?1.f:.8f);
            g.setColour(hi);g.fillRoundedRectangle(cx+off-w*.5f,y-1.2f*s,w,2.4f*s,1.f*s);
            if(s>.55f){g.setColour(shade.withAlpha(.4f));g.drawRoundedRectangle(cx+off-w*.5f,y-1.2f*s,w,2.4f*s,1.f*s,.5f);}
        }
        for(int i=0;i<6;++i) {   // ribcage: paired ribs sweeping out and down, breathing slightly
            const float ry=shY+(3.f+static_cast<float>(i)*4.1f)*s,w=(12.5f-static_cast<float>(i)*1.6f)*s*ts*(1.f+breath*.035f);
            for(int sd=-1;sd<=1;sd+=2) {
                juce::Path r;r.startNewSubPath(cx,ry);r.quadraticTo(cx+static_cast<float>(sd)*w*.95f,ry-2.2f*s,cx+static_cast<float>(sd)*w,ry+(3.8f+static_cast<float>(i)*.5f)*s);
                g.setColour(shade.withAlpha(.5f));g.strokePath(r,juce::PathStrokeType(thick*1.15f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
                g.setColour(hi.withAlpha(.92f));g.strokePath(r,juce::PathStrokeType(thick*.75f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
            }
        }
        if(front) {g.setColour(mid.withAlpha(.9f));g.fillRoundedRectangle(cx-1.3f*s,shY+2.f*s,2.6f*s,20.f*s,1.2f*s);}   // sternum
        bone(cx-9.5f*s*ts,shY,cx+9.5f*s*ts,shY,.9f,.9f);   // clavicles
        const float spread=std::max(0.f,effort-.55f)*1.2f,tuck=std::max(0.f,.35f-effort);   // shout opens, whisper tucks in
        for(int sd=-1;sd<=1;sd+=2) {   // arms
            const float swing=std::sin(phase+(sd>0?pi:0.f))*.5f*walk;
            const float a1=static_cast<float>(sd)*(.10f+spread-tuck*.25f)+swing*.7f;
            const float shx=cx+static_cast<float>(sd)*10.f*s*ts;
            const float ex=shx+std::sin(a1)*15.f*s,ey=shY+std::cos(a1)*15.f*s;
            const float a2=a1+static_cast<float>(-sd)*(.30f+tuck*1.2f)+swing*.3f;
            const float hx2=ex+std::sin(a2)*14.f*s,hy2=ey+std::cos(a2)*14.f*s;
            bone(shx,shY,ex,ey,1.5f,1.1f);bone(ex,ey,hx2,hy2,1.1f,.8f);
            for(int f=-1;f<=1;++f) {g.setColour(hi.withAlpha(.85f));g.drawLine(hx2,hy2,hx2+std::sin(a2+static_cast<float>(f)*.35f)*4.5f*s,hy2+std::cos(a2+static_cast<float>(f)*.35f)*4.5f*s,std::max(.6f,.9f*s));}
        }
        for(int i=0;i<3;++i) {   // cervical vertebrae
            const float y=shY-(1.5f+static_cast<float>(i)*2.6f)*s;g.setColour(hi);g.fillRoundedRectangle(cx-1.5f*s,y-1.1f*s,3.f*s,2.2f*s,.9f*s);
        }
        // ---- skull, drawn larger than the body scale so it stays readable when far away
        const float k=s*1.2f;
        const float jaw=std::clamp(effort*.45f+level*.9f,0.f,1.f);
        juce::Path skull;
        skull.startNewSubPath(cx-9.4f*k*ts,headY+1.5f*k);
        skull.cubicTo(cx-10.f*k*ts,headY-8.f*k,cx-5.f*k*ts,headY-11.6f*k,cx,headY-11.6f*k);
        skull.cubicTo(cx+5.f*k*ts,headY-11.6f*k,cx+10.f*k*ts,headY-8.f*k,cx+9.4f*k*ts,headY+1.5f*k);
        skull.cubicTo(cx+9.f*k*ts,headY+4.f*k,cx+7.f*k*ts,headY+5.f*k,cx+6.4f*k*ts,headY+7.f*k);
        skull.lineTo(cx+4.2f*k*ts,headY+8.6f*k);skull.lineTo(cx-4.2f*k*ts,headY+8.6f*k);skull.lineTo(cx-6.4f*k*ts,headY+7.f*k);
        skull.cubicTo(cx-7.f*k*ts,headY+5.f*k,cx-9.f*k*ts,headY+4.f*k,cx-9.4f*k*ts,headY+1.5f*k);skull.closeSubPath();
        glow(g,cx,headY-1.f*k,17.f*k,Theme::green,.8f+1.6f*level);
        juce::ColourGradient cranium(juce::Colour(0xfffaf8ee),cx-6.f*k,headY-10.f*k,juce::Colour(0xffb4af9b),cx+5.f*k,headY+9.f*k,false);
        g.setGradientFill(cranium);g.fillPath(skull);
        g.setColour(shade.withAlpha(.65f));g.strokePath(skull,juce::PathStrokeType(std::max(.7f,.9f*k)));
        if(front) {
            for(int sd=-1;sd<=1;sd+=2) {   // eye sockets with glowing pupils
                const float ex=cx+static_cast<float>(sd)*4.1f*k*ts,ey=headY+.8f*k;
                g.setColour(juce::Colour(0xff0a0c0b));g.fillEllipse(ex-2.9f*k*ts,ey-3.f*k,5.8f*k*ts,6.2f*k);
                glow(g,ex,ey+.2f*k,5.2f*k,Theme::green,.6f+3.f*level);
                g.setColour(Theme::green.withAlpha(.35f+.65f*level));g.fillEllipse(ex-1.2f*k*ts,ey-1.1f*k,2.4f*k*ts,2.4f*k);
            }
            juce::Path nose;nose.addTriangle(cx-1.5f*k*ts,headY+5.2f*k,cx+1.5f*k*ts,headY+5.2f*k,cx,headY+2.6f*k);
            g.setColour(juce::Colour(0xff161814));g.fillPath(nose);
            g.setColour(shade.withAlpha(.4f));g.drawLine(cx-7.6f*k*ts,headY+3.6f*k,cx-5.2f*k*ts,headY+5.6f*k,std::max(.6f,.7f*k));g.drawLine(cx+7.6f*k*ts,headY+3.6f*k,cx+5.2f*k*ts,headY+5.6f*k,std::max(.6f,.7f*k));   // cheekbones
            const float drop=jaw*5.f*k;
            if(jaw>.04f) {   // mouth interior behind the dropped mandible
                g.setColour(juce::Colour(0xff050606).withAlpha(.9f));
                juce::Path m;m.startNewSubPath(cx-4.2f*k*ts,headY+8.6f*k);m.lineTo(cx+4.2f*k*ts,headY+8.6f*k);m.lineTo(cx+3.8f*k*ts,headY+9.4f*k+drop);m.lineTo(cx-3.8f*k*ts,headY+9.4f*k+drop);m.closeSubPath();g.fillPath(m);
            }
            for(int t2=-3;t2<=3;++t2) {   // upper teeth
                const float tx=cx+static_cast<float>(t2)*1.15f*k*ts;g.setColour(hi);g.fillRoundedRectangle(tx-.5f*k*ts,headY+7.4f*k,1.0f*k*ts,1.6f*k,.3f*k);
            }
            juce::Path man;man.startNewSubPath(cx-5.4f*k*ts,headY+6.6f*k+drop*.4f);man.lineTo(cx-4.6f*k*ts,headY+10.6f*k+drop);man.quadraticTo(cx,headY+12.2f*k+drop,cx+4.6f*k*ts,headY+10.6f*k+drop);
            man.lineTo(cx+5.4f*k*ts,headY+6.6f*k+drop*.4f);man.lineTo(cx+4.2f*k*ts,headY+9.2f*k+drop);man.lineTo(cx-4.2f*k*ts,headY+9.2f*k+drop);man.closeSubPath();
            g.setColour(mid);g.fillPath(man);g.setColour(shade.withAlpha(.55f));g.strokePath(man,juce::PathStrokeType(std::max(.6f,.7f*k)));
            for(int t2=-2;t2<=2;++t2) {g.setColour(hi);const float tx=cx+static_cast<float>(t2)*1.2f*k*ts;g.fillRoundedRectangle(tx-.5f*k*ts,headY+9.2f*k+drop-1.5f*k,1.f*k*ts,1.5f*k,.3f*k);}
        } else {
            const float ex=cx+(std::sin(turn)>0.f?1.f:-1.f)*3.6f*k*ts;
            g.setColour(juce::Colour(0xff0a0c0b));g.fillEllipse(ex-2.4f*k*ts,headY-2.f*k,4.8f*k*ts,5.6f*k);
            glow(g,ex,headY+.6f*k,4.6f*k,Theme::green,.6f+3.f*level);
            g.setColour(shade.withAlpha(.5f));g.drawLine(cx,headY-10.f*k,cx,headY-2.f*k,std::max(.6f,.7f*k));   // cranial seam
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
