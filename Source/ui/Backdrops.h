#pragma once
#include "Theme.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace afar::ui {
// Environment art for the stage. Every scene is built from the same layers so the 20 spaces feel like one family:
//   1. sky / ceiling gradient   2. light source(s)   3. depth layers (far -> near, each darker and crisper)
//   4. ground plane with perspective   5. atmospheric fog at the horizon   6. vignette.
// Deterministic (no runtime randomness); rendered once per scene into a cached image, so cost is irrelevant.
namespace backdrop {
using P=std::pair<float,float>;
struct Canvas {
    juce::Graphics& g; float W,H,hz;   // hz: horizon / far-floor line (the skeleton's farthest foot position)
    static juce::Colour C(unsigned v){return juce::Colour(static_cast<std::uint32_t>(v));}
    static float rnd(int i,int k){const float v=std::sin(static_cast<float>(i)*12.9898f+static_cast<float>(k)*78.233f)*43758.5453f;return v-std::floor(v);}
    static float lerp(float a,float b,float t){return a+(b-a)*t;}
    void vgrad(float y0,float y1,juce::Colour a,juce::Colour b,float x=0.f,float w=-1.f) const {
        juce::ColourGradient gr(a,0.f,y0,b,0.f,y1,false);g.setGradientFill(gr);g.fillRect(x,y0,w<0.f?W-x:w,y1-y0);
    }
    void glowAt(float x,float y,float r,juce::Colour c,float a) const {
        juce::ColourGradient gr(c.withAlpha(a),x,y,c.withAlpha(0.f),x+r,y,true);g.setGradientFill(gr);g.fillEllipse(x-r,y-r,r*2.f,r*2.f);
    }
    static juce::Path pathOf(const std::vector<P>& pts) {
        juce::Path p;
        p.startNewSubPath(pts[0].first,pts[0].second);
        for(size_t i=1;i<pts.size();++i) p.lineTo(pts[i].first,pts[i].second);
        p.closeSubPath();
        return p;
    }
    void poly(const std::vector<P>& pts,juce::Colour c) const {
        if(pts.size()<3) return;
        g.setColour(c);
        g.fillPath(pathOf(pts));
    }
    void polyGrad(const std::vector<P>& pts,const juce::ColourGradient& gr) const {
        if(pts.size()<3) return;
        g.setGradientFill(gr);
        g.fillPath(pathOf(pts));
    }
    void line(float x0,float y0,float x1,float y1,juce::Colour c,float t=1.f) const {g.setColour(c);g.drawLine(x0,y0,x1,y1,t);}
    // horizontal band of fog centred on y
    void fog(float y,float h,juce::Colour c,float a) const {
        vgrad(y-h,y,c.withAlpha(0.f),c.withAlpha(a));vgrad(y,y+h,c.withAlpha(a),c.withAlpha(0.f));
    }
    // layered ridge line (hills, trees, mesas): sum of sines, filled down to the bottom of the canvas
    void ridge(float y,float amp,int seed,juce::Colour c,float freq=1.f,float jag=0.f) const {
        std::vector<P> pts;pts.push_back({0.f,H});
        for(float x=0.f;x<=W+8.f;x+=6.f){
            const float n=std::sin(x*.011f*freq+static_cast<float>(seed)*1.7f)*.55f+std::sin(x*.027f*freq+static_cast<float>(seed)*3.1f)*.30f
                         +std::sin(x*.071f*freq+static_cast<float>(seed))*.15f*(1.f+jag);
            pts.push_back({x,y-amp*n});
        }
        pts.push_back({W,H});poly(pts,c);
    }
    void vignette(float a=.60f) const {
        juce::ColourGradient v(juce::Colours::black.withAlpha(0.f),W*.5f,H*.52f,juce::Colours::black.withAlpha(a),W*.5f+W*.66f,H*.52f,true);
        g.setGradientFill(v);g.fillRect(0.f,0.f,W,H);
    }
    void rays(float x0,float y0,float spread,float len,int n,juce::Colour c,float a,float angle) const {   // light shafts from a point
        for(int i=0;i<n;++i){
            const float t=n>1?static_cast<float>(i)/static_cast<float>(n-1):.5f,ang=angle+(t-.5f)*spread,wd=len*.06f*(.6f+rnd(i,3));
            const float x1=x0+std::sin(ang)*len,y1=y0+std::cos(ang)*len;
            juce::ColourGradient gr(c.withAlpha(a*(.5f+rnd(i,5))),x0,y0,c.withAlpha(0.f),x1,y1,false);
            polyGrad({{x0-wd*.2f,y0},{x0+wd*.2f,y0},{x1+wd,y1},{x1-wd,y1}},gr);
        }
    }
};

// ---- perspective rooms: tunnels, halls, alleys, shafts -------------------------------------------------------------
enum class Rib{Rect,Round,Arch,Truss,Beam};
struct Room {
    unsigned ceil,wall,floorC,end,ribC,lightC;
    Rib rib=Rib::Rect; int ribs=8; float fw=.11f,ftop=.30f,fbot=.47f; float lightA=.55f; bool lights=true,floorLines=true;
    float ribA=.55f,endGlow=.55f;
};
inline void room(const Canvas& k,const Room& r) {
    using C=Canvas;const float W=k.W,H=k.H;
    const float L0=-W*.14f,R0=W*1.14f,T0=-H*.30f,B0=H*1.14f;
    const float L1=W*.5f-W*r.fw,R1=W*.5f+W*r.fw,T1=H*r.ftop,B1=H*r.fbot;
    const auto at=[&](float p,float& l,float& rr,float& t,float& b){l=C::lerp(L0,L1,p);rr=C::lerp(R0,R1,p);t=C::lerp(T0,T1,p);b=C::lerp(B0,B1,p);};
    k.g.setColour(C::C(r.end).darker(.5f));k.g.fillRect(0.f,0.f,W,H);
    k.polyGrad({{L0,T0},{R0,T0},{R1,T1},{L1,T1}},juce::ColourGradient(C::C(r.ceil).brighter(.12f),0.f,T0,C::C(r.ceil).darker(.45f),0.f,T1,false));
    k.polyGrad({{L0,T0},{L1,T1},{L1,B1},{L0,B0}},juce::ColourGradient(C::C(r.wall).brighter(.10f),L0,0.f,C::C(r.wall).darker(.55f),L1,0.f,false));
    k.polyGrad({{R0,T0},{R1,T1},{R1,B1},{R0,B0}},juce::ColourGradient(C::C(r.wall).brighter(.04f),R0,0.f,C::C(r.wall).darker(.60f),R1,0.f,false));
    k.polyGrad({{L0,B0},{R0,B0},{R1,B1},{L1,B1}},juce::ColourGradient(C::C(r.floorC).brighter(.10f),0.f,B0,C::C(r.floorC).darker(.50f),0.f,B1,false));
    k.polyGrad({{L1,T1},{R1,T1},{R1,B1},{L1,B1}},juce::ColourGradient(C::C(r.end).darker(.30f),W*.5f,(T1+B1)*.5f,C::C(r.end).darker(.80f),W*.5f,B1+4.f,true));
    k.glowAt(W*.5f,(T1+B1)*.5f,W*.30f,C::C(r.lightC),r.endGlow);
    k.glowAt(W*.5f,(T1+B1)*.5f,W*.46f,C::C(r.lightC),r.endGlow*.35f);   // bloom softens the edge of the far opening
    if(r.floorLines) for(int i=-6;i<=6;++i){   // floor seams converge on the vanishing point
        const float x0=W*.5f+static_cast<float>(i)*W*.16f,x1=W*.5f+static_cast<float>(i)*(R1-L1)/12.f;
        k.line(x0,B0,x1,B1,juce::Colours::white.withAlpha(.045f),1.f);
    }
    for(int i=r.ribs;i>=0;--i){   // ribs / frames, far to near
        const float p=1.f-1.f/(1.f+static_cast<float>(i)*.55f);float l,rr,t,b;at(p,l,rr,t,b);
        const float a=r.ribA*std::pow(1.f-p,.9f)+.04f;const auto col=C::C(r.ribC).withAlpha(a);
        k.g.setColour(col);
        if(r.rib==Rib::Round){k.g.drawEllipse(l,t,rr-l,b-t,1.2f);}
        else if(r.rib==Rib::Arch){
            juce::Path pa;const float spring=t+(b-t)*.38f;pa.startNewSubPath(l,b);pa.lineTo(l,spring);pa.quadraticTo(l+(rr-l)*.10f,t+(b-t)*.04f,(l+rr)*.5f,t);
            pa.quadraticTo(rr-(rr-l)*.10f,t+(b-t)*.04f,rr,spring);pa.lineTo(rr,b);k.g.strokePath(pa,juce::PathStrokeType(1.3f+2.f*(1.f-p)));
        } else if(r.rib==Rib::Truss){
            k.g.drawLine(l,t,rr,t,1.4f+(1.f-p));k.g.drawLine(l,t,(l+rr)*.5f,t-(b-t)*.10f,1.f);k.g.drawLine(rr,t,(l+rr)*.5f,t-(b-t)*.10f,1.f);
            k.g.drawLine(l,t,l+(rr-l)*.5f,t+(b-t)*.12f,.8f);k.g.drawLine(rr,t,l+(rr-l)*.5f,t+(b-t)*.12f,.8f);
        } else if(r.rib==Rib::Beam){k.g.drawLine(l,t,rr,t,2.f+4.f*(1.f-p));k.g.drawLine(l,t,l,b,1.f);k.g.drawLine(rr,t,rr,b,1.f);}
        else {k.g.drawLine(l,t,rr,t,1.1f+(1.f-p));k.g.drawLine(l,t,l,b,1.1f);k.g.drawLine(rr,t,rr,b,1.1f);}
        if(r.lights && i<r.ribs){   // ceiling light halfway to the next rib, with its pool on the floor
            const float pm=1.f-1.f/(1.f+(static_cast<float>(i)+.5f)*.55f);float l2,r2,t2,b2;at(pm,l2,r2,t2,b2);
            const float cx=(l2+r2)*.5f,ly=t2+(b2-t2)*.07f,s=1.f-pm;
            k.glowAt(cx,ly,(r2-l2)*.10f+10.f,C::C(r.lightC),r.lightA*(.35f+.65f*s));
            k.g.setColour(C::C(r.lightC).brighter(.4f).withAlpha(.85f*(.3f+.7f*s)));k.g.fillEllipse(cx-(r2-l2)*.012f-1.5f,ly-1.5f,(r2-l2)*.024f+3.f,3.f);
            k.glowAt(cx,b2-(b2-t2)*.02f,(r2-l2)*.16f,C::C(r.lightC),r.lightA*.35f*s);
        }
    }
}

// ---- outdoor skies ---------------------------------------------------------------------------------------------------
inline void sky(const Canvas& k,unsigned top,unsigned mid,unsigned low) {
    k.vgrad(0.f,k.hz*.72f,Canvas::C(top),Canvas::C(mid));k.vgrad(k.hz*.72f,k.hz+2.f,Canvas::C(mid),Canvas::C(low));
}
inline void stars(const Canvas& k,int n,float maxY,float a) {
    for(int i=0;i<n;++i){const float x=k.W*Canvas::rnd(i,11),y=maxY*Canvas::rnd(i,12),s=.7f+1.3f*Canvas::rnd(i,13);
        k.g.setColour(juce::Colours::white.withAlpha(a*(.3f+.7f*Canvas::rnd(i,14))));k.g.fillEllipse(x,y,s,s);}
}
inline void ground(const Canvas& k,unsigned near_,unsigned far_) {
    k.vgrad(k.hz-1.f,k.H,Canvas::C(far_),Canvas::C(near_));
    for(int i=-8;i<=8;++i) k.line(k.W*.5f+static_cast<float>(i)*k.W*.03f,k.hz,k.W*.5f+static_cast<float>(i)*k.W*.20f,k.H,juce::Colours::white.withAlpha(.025f),1.f);
}
inline void pine(const Canvas& k,float x,float base,float h,juce::Colour c,float rimA=0.f) {
    for(int t=0;t<4;++t){const float y0=base-h*(static_cast<float>(t)/4.f),w=h*.30f*(1.f-static_cast<float>(t)*.18f);
        k.poly({{x-w,y0},{x+w,y0},{x,y0-h*.42f}},c);}
    k.poly({{x-h*.025f,base},{x+h*.025f,base},{x+h*.025f,base+h*.1f},{x-h*.025f,base+h*.1f}},c);
    if(rimA>0.f) k.line(x,base-h*1.1f,x+h*.20f,base-h*.15f,juce::Colours::white.withAlpha(rimA),1.f);
}
}

inline void drawBackdrop(juce::Graphics& g,int scene,float W,float H) {
    using namespace backdrop;using C=Canvas;
    Canvas k{g,W,H,H*.42f};const float hz=k.hz;
    switch(scene) {
    case 0: { // TUNNEL: long concrete run, sodium lamps, cold daylight at the far end
        room(k,{0xff1b2a2e,0xff22373b,0xff141e21,0xff9fd8ee,0xff6fa7b0,0xffffb050,Rib::Rect,9,.09f,.30f,.47f,.60f,true,true,.50f,.70f});
        k.fog(hz,H*.10f,C::C(0xff9fd8ee),.10f);break; }
    case 1: { // SEWER: round brick pipe, green light, water channel with ripples
        room(k,{0xff131f1a,0xff1a2b22,0xff0f1a16,0xff6fa66a,0xff5f8d63,0xff9fe08c,Rib::Round,9,.10f,.26f,.50f,.40f,true,false,.50f,.45f});
        const float cx=W*.5f;
        k.polyGrad({{cx-W*.012f,hz},{cx+W*.012f,hz},{cx+W*.20f,H},{cx-W*.20f,H}},juce::ColourGradient(C::C(0xff4f8f8c).withAlpha(.30f),0.f,hz,C::C(0xff0b1714).withAlpha(.85f),0.f,H,false));
        for(int i=0;i<7;++i){const float t=static_cast<float>(i)/7.f,y=C::lerp(hz+4.f,H-8.f,t*t),w=C::lerp(6.f,60.f,t*t);
            g.setColour(juce::Colours::white.withAlpha(.10f*(1.f-t*.5f)));g.drawEllipse(cx-w*.5f+(C::rnd(i,1)-.5f)*40.f*t,y,w,w*.14f,1.f);}
        k.fog(hz,H*.12f,C::C(0xff6fa66a),.10f);break; }
    case 2: { // SUBWAY PLATFORM: tiled hall, tube lights, yellow edge strip, pillars
        room(k,{0xff1c2a31,0xff2a3d44,0xff161f24,0xffc9ecf5,0xff86b5c1,0xffd6f4ff,Rib::Rect,8,.12f,.28f,.47f,.55f,true,true,.50f,.45f});
        k.poly({{W*.64f,H},{W*.70f,H},{W*.545f,hz+4.f},{W*.54f,hz+4.f}},C::C(0xffe8b84a).withAlpha(.70f));   // platform edge
        k.poly({{W*.70f,H},{W*1.14f,H},{W*.55f,hz+4.f},{W*.545f,hz+4.f}},C::C(0xff05090b).withAlpha(.85f));   // track pit
        for(int i=1;i<5;++i){const float p=1.f-1.f/(1.f+static_cast<float>(i)*.9f),x=C::lerp(W*.10f,W*.45f,p),w=C::lerp(34.f,5.f,p);   // pillars
            k.vgrad(C::lerp(-4.f,hz*.74f,p),C::lerp(H,hz+6.f,p),C::C(0xff3d5560).withAlpha(.85f),C::C(0xff0f171b).withAlpha(.95f),x,w);}
        k.fog(hz,H*.09f,C::C(0xffc9ecf5),.08f);break; }
    case 3: { // CAVE: dripstone, damp teal glow, still pool
        k.vgrad(0.f,H,C::C(0xff06100f),C::C(0xff0d1c1b));k.glowAt(W*.5f,hz-H*.02f,W*.45f,C::C(0xff4cc2b8),.40f);k.glowAt(W*.78f,H*.18f,W*.22f,C::C(0xff6fe0d0),.12f);
        for(int i=0;i<4;++i){const float d=static_cast<float>(i)/3.f;   // far -> near stalactites, darker as they approach
            for(int j=0;j<10;++j){const float x=W*(static_cast<float>(j)+.5f)/10.f+(C::rnd(j+i*10,2)-.5f)*W*.08f,hh=H*(.10f+.30f*C::rnd(j+i*10,1))*(.6f+.6f*d),w=W*(.012f+.026f*C::rnd(j+i*10,4))*(.7f+.6f*d);
                k.polyGrad({{x-w,-4.f},{x+w,-4.f},{x+w*.15f,hh*.9f},{x,hh}},juce::ColourGradient(C::C(0xff122523).brighter(.12f*(1.f-d)),x,0.f,C::C(0xff050c0c),x,hh,false));}}
        k.vgrad(hz,H,C::C(0xff0b1a1a),C::C(0xff04090a));
        k.polyGrad({{W*.18f,H*.62f},{W*.82f,H*.62f},{W*.96f,H*.93f},{W*.04f,H*.93f}},juce::ColourGradient(C::C(0xff5fd0c4).withAlpha(.12f),0.f,H*.62f,C::C(0xff0a2020).withAlpha(.22f),0.f,H*.93f,false));   // pool
        for(int j=0;j<9;++j){const float x=W*(static_cast<float>(j)+.4f)/9.f,hh=H*(.06f+.16f*C::rnd(j,7)),w=W*(.014f+.02f*C::rnd(j,8));
            k.poly({{x-w,H+2.f},{x+w,H+2.f},{x,H-hh}},C::C(0xff050b0b));}
        k.fog(hz+H*.04f,H*.14f,C::C(0xff4cc2b8),.10f);break; }
    case 4: { // CELLAR / BUNKER: shallow brick box, single hanging bulb, crates
        room(k,{0xff201a16,0xff2c241d,0xff17120f,0xff3a2d22,0xff6d5a45,0xffffc27a,Rib::Rect,3,.34f,.20f,.55f,.85f,true,true,.55f,.45f});
        for(int r2=0;r2<9;++r2){const float y=H*(.20f+.04f*static_cast<float>(r2));k.line(W*.16f,y,W*.84f,y,C::C(0xff000000).withAlpha(.18f),1.f);   // brick courses on the end wall
            for(int c=0;c<14;++c){const float x=W*.16f+W*.68f*(static_cast<float>(c)+(r2%2?.5f:0.f))/14.f;k.line(x,y,x,y+H*.04f,C::C(0xff000000).withAlpha(.14f),1.f);}}
        k.line(W*.5f,0.f,W*.5f,H*.20f,C::C(0xff0a0807),1.2f);k.glowAt(W*.5f,H*.23f,W*.34f,C::C(0xffffb860),.55f);
        g.setColour(C::C(0xffffe2a8));g.fillEllipse(W*.5f-4.f,H*.20f,8.f,9.f);
        k.poly({{W*.08f,hz+H*.10f},{W*.20f,hz+H*.10f},{W*.20f,hz+H*.22f},{W*.08f,hz+H*.22f}},C::C(0xff0c0907).withAlpha(.9f));
        k.poly({{W*.78f,hz+H*.05f},{W*.92f,hz+H*.05f},{W*.92f,hz+H*.22f},{W*.78f,hz+H*.22f}},C::C(0xff0c0907).withAlpha(.9f));
        break; }
    case 5: { // GRAVEYARD: moonlit mist, bare trees, stones in depth
        sky(k,0xff050b11,0xff0e2420,0xff2a4a3e);stars(k,70,hz*.7f,.7f);
        k.glowAt(W*.16f,H*.20f,W*.34f,C::C(0xffa8e8c8),.55f);g.setColour(C::C(0xffe4f6dc).withAlpha(.96f));g.fillEllipse(W*.16f-17.f,H*.20f-17.f,34.f,34.f);
        g.setColour(C::C(0xffb7d6b0).withAlpha(.5f));g.fillEllipse(W*.16f-8.f,H*.20f-6.f,9.f,9.f);g.fillEllipse(W*.16f+3.f,H*.20f+3.f,6.f,6.f);
        k.ridge(hz-2.f,H*.06f,2,C::C(0xff0a1a17),.8f);
        for(int t=0;t<3;++t){   // bare trees
            const float x=W*(.30f+.30f*static_cast<float>(t))+C::rnd(t,1)*W*.08f,b=hz+2.f,h=H*(.32f-.04f*static_cast<float>(t));
            g.setColour(C::C(0xff050c0b));g.drawLine(x,b,x,b-h,2.4f);
            for(int br=0;br<6;++br){const float y=b-h*(.35f+.12f*static_cast<float>(br)),dx=(br%2?1.f:-1.f)*h*(.20f+.06f*C::rnd(br,t));g.drawLine(x,y,x+dx,y-h*.14f,1.4f);g.drawLine(x+dx,y-h*.14f,x+dx*1.4f,y-h*.28f,1.f);}}
        ground(k,0xff060d0c,0xff112420);
        for(int i=0;i<15;++i){   // headstones: dark faces, moon-side rim light
            const float d=C::rnd(i,1),by=C::lerp(hz+H*.05f,H*.94f,d),sz=C::lerp(9.f,30.f,d),x=W*(.03f+.94f*C::rnd(i,2));
            k.poly({{x,by},{x+sz,by},{x+sz,by-sz*1.1f},{x+sz*.5f,by-sz*1.4f},{x,by-sz*1.1f}},C::C(0xff0b1514));
            k.line(x,by,x,by-sz*1.1f,juce::Colours::white.withAlpha(.18f),1.f);k.line(x,by-sz*1.1f,x+sz*.5f,by-sz*1.4f,juce::Colours::white.withAlpha(.22f),1.f);
            if(i%3==0){k.line(x+sz*.5f,by-sz*1.05f,x+sz*.5f,by-sz*.45f,juce::Colours::white.withAlpha(.12f),1.f);k.line(x+sz*.3f,by-sz*.8f,x+sz*.7f,by-sz*.8f,juce::Colours::white.withAlpha(.12f),1.f);}
            k.glowAt(x+sz*.5f,by,sz*1.4f,C::C(0xff000000),.30f);}
        k.fog(hz+H*.05f,H*.13f,C::C(0xff8fd0b8),.16f);break; }
    case 6: { // FOREST: dusk, god rays, trunks in fog
        sky(k,0xff0a1a16,0xff1d3b2e,0xff6a9a72);k.glowAt(W*.28f,-H*.05f,W*.55f,C::C(0xffd8f0a8),.30f);k.rays(W*.28f,-4.f,.7f,H*1.1f,6,C::C(0xffe6f4b4),.10f,.35f);
        for(int layer=0;layer<3;++layer){   // far (pale) -> near (black)
            const float d=static_cast<float>(layer)/2.f;
            for(int i=0;i<(layer==0?16:layer==1?11:6);++i){
                const float x=W*C::rnd(i+layer*17,2),w=C::lerp(W*.012f,W*.045f,d)*(.7f+.6f*C::rnd(i,3)),base=C::lerp(hz+4.f,H*1.02f,d*d*.9f+.1f*d),top=-4.f;
                const auto col=C::C(0xff0a1511).interpolatedWith(C::C(0xff7fa58a),(1.f-d)*.55f);
                k.polyGrad({{x-w,top},{x+w,top},{x+w*1.4f,base},{x-w*1.4f,base}},juce::ColourGradient(col.withAlpha(.95f),x,H*.15f,col.darker(.5f).withAlpha(.98f),x,base,false));}
            if(layer<2) k.fog(hz+H*.02f,H*.30f,C::C(0xffa8d8b0),.14f-.05f*static_cast<float>(layer));}
        k.vgrad(hz-2.f,H,C::C(0xff12261c),C::C(0xff050c09));
        for(int i=0;i<40;++i){const float x=W*C::rnd(i,5),y=C::lerp(hz+4.f,H,C::rnd(i,6)*C::rnd(i,6)),h2=C::lerp(4.f,16.f,(y-hz)/(H-hz));
            k.line(x,y,x+(C::rnd(i,7)-.5f)*6.f,y-h2,C::C(0xff2a5a3a).withAlpha(.5f),1.f);}
        for(int i=0;i<14;++i){const float x=W*C::rnd(i,8),y=H*(.30f+.5f*C::rnd(i,9));k.glowAt(x,y,7.f,C::C(0xffd8ff9a),.55f);}   // fireflies
        k.fog(hz+H*.06f,H*.12f,C::C(0xffa8d8b0),.10f);break; }
    case 7: { // CANYON / VALLEY: warm dusk mesas
        sky(k,0xff1c1730,0xff6a3a46,0xffe39a62);k.glowAt(W*.70f,hz-H*.01f,W*.40f,C::C(0xffffc27a),.55f);g.setColour(C::C(0xffffe2b0).withAlpha(.95f));g.fillEllipse(W*.70f-13.f,hz-H*.05f,26.f,26.f);
        const unsigned cols[4]={0xff6c4152,0xff4a2a3a,0xff2d1a25,0xff150d14};
        for(int l=0;l<4;++l){const float y=hz+H*(.02f+.07f*static_cast<float>(l));
            std::vector<P> pts{{0.f,H}};float x=0.f;int i=0;
            while(x<W+30.f){const float stepW=W*(.05f+.07f*C::rnd(i+l*9,1)),top=y-H*(.20f-.04f*static_cast<float>(l))*(.4f+C::rnd(i+l*9,2));pts.push_back({x,top});pts.push_back({x+stepW*.9f,top});x+=stepW;++i;}
            pts.push_back({W+30.f,H});k.polyGrad(pts,juce::ColourGradient(C::C(cols[l]).brighter(.10f),0.f,y-H*.2f,C::C(cols[l]).darker(.35f),0.f,H,false));
            for(int s=0;s<3;++s) k.line(0.f,y-H*(.02f+.03f*static_cast<float>(s)),W,y-H*(.02f+.03f*static_cast<float>(s)),juce::Colours::white.withAlpha(.03f),1.f);}
        ground(k,0xff0c0709,0xff2a1a1a);k.fog(hz+H*.02f,H*.10f,C::C(0xffffb27a),.12f);break; }
    case 8: { // OPEN FIELD: wide dawn, soft hills, grass
        sky(k,0xff0b1f2e,0xff2f5f70,0xffe8b78a);k.glowAt(W*.30f,hz,W*.55f,C::C(0xffffd29a),.50f);g.setColour(C::C(0xffffe8c4).withAlpha(.92f));g.fillEllipse(W*.30f-11.f,hz-16.f,22.f,22.f);
        k.ridge(hz-H*.005f,H*.05f,4,C::C(0xff3a5a52),.6f);k.ridge(hz+H*.02f,H*.045f,7,C::C(0xff1f3a34),.8f);
        const float tx=W*.74f;g.setColour(C::C(0xff0d1a16));g.drawLine(tx,hz+H*.02f,tx,hz-H*.11f,2.f);k.glowAt(tx,hz-H*.14f,H*.10f,C::C(0xff0d1a16),.9f);
        ground(k,0xff05100d,0xff1b352c);
        for(int i=0;i<110;++i){const float x=W*C::rnd(i,5),d=C::rnd(i,6)*C::rnd(i,6),y=C::lerp(hz+3.f,H,d),h2=C::lerp(3.f,22.f,d);
            k.line(x,y,x+(C::rnd(i,7)-.5f)*10.f*d,y-h2,C::C(0xff4a7a58).withAlpha(.35f+.3f*d),1.f);}
        k.fog(hz+H*.02f,H*.12f,C::C(0xffe8b78a),.18f);break; }
    case 9: { // CITY ALLEY: night, lit windows, wet street, one cold sign
        room(k,{0xff0b1119,0xff18222e,0xff0d1218,0xff1f3a4a,0xff3a5568,0xff7fd6ff,Rib::Rect,8,.075f,.12f,.47f,.45f,false,false,.40f,.55f});
        for(int sd=0;sd<2;++sd) for(int i=0;i<8;++i){   // lit windows follow the facade perspective
            const float p=1.f-1.f/(1.f+static_cast<float>(i)*.55f),pn=1.f-1.f/(1.f+(static_cast<float>(i)+.8f)*.55f);
            const float L0=-W*.14f,L1=W*.5f-W*.075f,R0=W*1.14f,R1=W*.5f+W*.075f,T0=-H*.30f,T1=H*.12f,B0=H*1.14f,B1=H*.47f;
            const float x0=sd?C::lerp(R0,R1,p):C::lerp(L0,L1,p),x1=sd?C::lerp(R0,R1,pn):C::lerp(L0,L1,pn);
            for(int row=0;row<3;++row){if(C::rnd(i*3+row,sd+20)<.45f)continue;
                const float ya=C::lerp(C::lerp(T0,T1,p),C::lerp(B0,B1,p),.18f+.20f*static_cast<float>(row)),yb=C::lerp(C::lerp(T0,T1,pn),C::lerp(B0,B1,pn),.18f+.20f*static_cast<float>(row)+.10f);
                const float xa=C::lerp(x0,x1,.25f),xb=C::lerp(x0,x1,.75f);
                k.polyGrad({{xa,ya},{xb,C::lerp(ya,yb,.1f)},{xb,yb},{xa,C::lerp(ya,yb,.9f)}},juce::ColourGradient(C::C(0xffffd28a).withAlpha(.75f),xa,ya,C::C(0xffffa850).withAlpha(.55f),xb,yb,false));}}
        k.glowAt(W*.20f,H*.36f,W*.15f,C::C(0xff7fd6ff),.40f);k.poly({{W*.14f,H*.31f},{W*.26f,H*.31f},{W*.26f,H*.37f},{W*.14f,H*.37f}},C::C(0xffbdeaff).withAlpha(.85f));
        k.vgrad(hz+H*.02f,H,C::C(0xff7fd6ff).withAlpha(.04f),C::C(0xff7fd6ff).withAlpha(.0f));   // wet-street sheen
        k.fog(hz,H*.10f,C::C(0xff9fb8c8),.12f);break; }
    case 10: { // PARKING GARAGE: low slab, beams, pillars, painted bays
        room(k,{0xff1e2422,0xff252d2b,0xff15191a,0xff8fb8a8,0xff5f7a70,0xffd6ffe8,Rib::Beam,9,.15f,.34f,.50f,.55f,true,false,.45f,.38f});
        for(int i=-5;i<=5;++i) k.poly({{W*.5f+static_cast<float>(i)*W*.020f-1.f,hz+3.f},{W*.5f+static_cast<float>(i)*W*.020f+1.f,hz+3.f},{W*.5f+static_cast<float>(i)*W*.30f+3.f,H},{W*.5f+static_cast<float>(i)*W*.30f-3.f,H}},juce::Colours::white.withAlpha(.05f));
        k.poly({{W*.47f,hz+3.f},{W*.485f,hz+3.f},{W*.40f,H},{W*.36f,H}},C::C(0xffe8c84a).withAlpha(.40f));
        for(int sd=0;sd<2;++sd) for(int i=0;i<3;++i){const float p=1.f-1.f/(1.f+static_cast<float>(i)*1.1f),x=sd?C::lerp(W*.90f,W*.60f,p):C::lerp(W*.10f,W*.40f,p),w=C::lerp(W*.05f,W*.010f,p);
            k.vgrad(C::lerp(-4.f,H*.32f,p),C::lerp(H,hz+8.f,p),C::C(0xff39433f),C::C(0xff0c100f),x-w*.5f,w);}
        k.fog(hz,H*.09f,C::C(0xff8fb8a8),.10f);break; }
    case 11: { // STAIRWELL: tall shaft, flights and rail, window light from above
        room(k,{0xff1b1f27,0xff262c36,0xff131519,0xff6a7fa0,0xff5a6a82,0xffbcd4ff,Rib::Rect,6,.13f,.14f,.48f,.35f,true,false,.45f,.42f});
        for(int s=0;s<12;++s){const float p=static_cast<float>(s)/12.f,xa=C::lerp(-W*.04f,W*.36f,p),ya=C::lerp(H*.95f,H*.40f,p);   // flight on the left wall
            k.poly({{xa,ya},{xa+W*.045f,ya-H*.012f},{xa+W*.045f,ya+H*.05f},{xa,ya+H*.06f}},C::C(0xff2f3846).withAlpha(.9f));k.line(xa,ya,xa+W*.045f,ya-H*.012f,juce::Colours::white.withAlpha(.10f),1.f);}
        k.line(-W*.04f,H*.82f,W*.40f,H*.30f,C::C(0xffa8bad8).withAlpha(.5f),1.6f);k.line(-W*.04f,H*.95f,W*.40f,H*.43f,C::C(0xffa8bad8).withAlpha(.25f),1.f);
        k.rays(W*.62f,-H*.05f,.5f,H*.9f,4,C::C(0xffbcd4ff),.10f,-.25f);break; }
    case 12: { // CATHEDRAL: pointed nave, stained glass, shafts, polished floor
        room(k,{0xff191726,0xff262238,0xff141320,0xff2b2440,0xff8e82c0,0xffffe0a8,Rib::Arch,8,.15f,.10f,.48f,.38f,true,false,.50f,.40f});
        const float cx=W*.5f;
        k.glowAt(cx,H*.26f,W*.10f,C::C(0xffff7a5a),.55f);k.glowAt(cx-W*.04f,H*.24f,W*.07f,C::C(0xff7aa8ff),.45f);k.glowAt(cx+W*.04f,H*.22f,W*.07f,C::C(0xffffd070),.50f);
        k.rays(cx-W*.20f,-H*.10f,.5f,H*1.1f,4,C::C(0xffffe0a8),.10f,.10f);k.rays(cx+W*.20f,-H*.10f,.5f,H*1.1f,3,C::C(0xffa8c4ff),.08f,-.12f);
        k.vgrad(hz+2.f,H,C::C(0xffffe0a8).withAlpha(.10f),C::C(0xffffe0a8).withAlpha(0.f),W*.34f,W*.32f);   // reflected light on the floor
        k.fog(hz,H*.11f,C::C(0xffd6c8ff),.10f);break; }
    case 13: { // EMPTY POOL: tiled basin under open sky
        k.vgrad(0.f,hz,C::C(0xff12203a),C::C(0xff7a8fa8));
        room(k,{0xff12203a,0xff2d6a7a,0xff1f5668,0xff43879a,0xff8fd0d8,0xffdff8ff,Rib::Rect,0,.20f,.36f,.50f,.0f,false,false,.3f,.40f});
        const float L0=-W*.14f,R0=W*1.14f,B0=H*1.14f,L1=W*.5f-W*.20f,R1=W*.5f+W*.20f,B1=H*.50f;
        for(int i=1;i<9;++i){const float p=1.f-1.f/(1.f+static_cast<float>(i)*.55f),y=C::lerp(B0,B1,p);k.line(C::lerp(L0,L1,p),y,C::lerp(R0,R1,p),y,juce::Colours::white.withAlpha(.10f),1.f);}
        for(int i=-8;i<=8;++i) k.line(W*.5f+static_cast<float>(i)*W*.15f,B0,W*.5f+static_cast<float>(i)*(R1-L1)/16.f,B1,juce::Colours::white.withAlpha(.08f),1.f);
        for(int i=0;i<2;++i){const float x=W*.10f+static_cast<float>(i)*W*.035f;g.setColour(C::C(0xffd8e8ee).withAlpha(.85f));g.drawLine(x,H*.55f,x,H*.30f,2.2f);}   // ladder
        for(int r2=0;r2<5;++r2) k.line(W*.10f,H*.50f-static_cast<float>(r2)*H*.045f,W*.135f,H*.50f-static_cast<float>(r2)*H*.045f,C::C(0xffd8e8ee).withAlpha(.7f),1.5f);
        k.glowAt(W*.5f,H*.1f,W*.5f,C::C(0xffdff8ff),.18f);k.fog(hz,H*.10f,C::C(0xffdff8ff),.10f);break; }
    case 14: { // WAREHOUSE: trusses, skylight shafts, racking, dust
        room(k,{0xff1c1a18,0xff2a2622,0xff141210,0xff5a5448,0xff7a705c,0xffffe2a8,Rib::Truss,9,.14f,.26f,.50f,.45f,true,true,.40f,.35f});
        k.rays(W*.30f,H*.02f,.30f,H*1.0f,4,C::C(0xffffe8b8),.13f,.18f);k.rays(W*.70f,H*.02f,.30f,H*1.0f,3,C::C(0xffffe8b8),.10f,-.18f);
        for(int sd=0;sd<2;++sd) for(int i=0;i<3;++i){const float p=1.f-1.f/(1.f+static_cast<float>(i)*.9f),x=sd?C::lerp(W*.90f,W*.60f,p):C::lerp(W*.04f,W*.38f,p),w=C::lerp(W*.10f,W*.02f,p),y0=C::lerp(H*.10f,H*.32f,p),y1=C::lerp(H*1.02f,hz+6.f,p);
            k.poly({{x,y0},{x+w,y0},{x+w,y1},{x,y1}},C::C(0xff0b0a09).withAlpha(.78f));
            for(int sh=1;sh<5;++sh){const float y=C::lerp(y0,y1,static_cast<float>(sh)/5.f);k.line(x,y,x+w,y,C::C(0xff8a7a5a).withAlpha(.32f),1.4f);}}
        for(int i=0;i<30;++i){const float x=W*C::rnd(i,3),y=H*C::rnd(i,4);g.setColour(juce::Colours::white.withAlpha(.12f+.2f*C::rnd(i,5)));g.fillEllipse(x,y,1.6f,1.6f);}
        break; }
    case 15: { // BATHROOM TILE: bright small room, grid, mirror sheen, tub
        room(k,{0xffcfd8dc,0xffa9b8be,0xff8f9ea5,0xffe8f4f6,0xff52666d,0xffffffff,Rib::Rect,6,.30f,.18f,.56f,.75f,true,true,.50f,.55f});
        k.vgrad(0.f,H,C::C(0xff000000).withAlpha(.55f),C::C(0xff000000).withAlpha(.35f));   // pull the bright room into the dark UI
        const float mx0=W*.34f,mx1=W*.66f,my0=H*.12f,my1=H*.38f;
        k.polyGrad({{mx0,my0},{mx1,my0},{mx1,my1},{mx0,my1}},juce::ColourGradient(C::C(0xffeaf8ff).withAlpha(.85f),mx0,my0,C::C(0xff7fa8b8).withAlpha(.70f),mx1,my1,false));
        k.poly({{mx0+W*.04f,my0},{mx0+W*.09f,my0},{mx0+W*.03f,my1},{mx0-W*.02f,my1}},juce::Colours::white.withAlpha(.35f));
        k.poly({{W*.04f,H*.62f},{W*.30f,H*.58f},{W*.32f,H*.90f},{W*.02f,H*.94f}},C::C(0xffe8f4f6).withAlpha(.55f));
        break; }
    case 16: { // CRYPT: vaulted stone, candlelight, sarcophagi
        room(k,{0xff17162a,0xff23223a,0xff111020,0xff2a2748,0xff6f6aa8,0xffffa85a,Rib::Arch,8,.13f,.16f,.48f,.35f,false,false,.40f,.20f});
        for(int sd=0;sd<2;++sd) for(int i=0;i<5;++i){   // wall candles along both sides
            const float p=1.f-1.f/(1.f+(static_cast<float>(i)+.5f)*.55f),L0=-W*.14f,L1=W*.5f-W*.13f,R0=W*1.14f,R1=W*.5f+W*.13f;
            const float x=sd?C::lerp(R0,R1,p):C::lerp(L0,L1,p),y=C::lerp(H*.50f,H*.30f,p);
            k.glowAt(x,y,C::lerp(36.f,9.f,p),C::C(0xffffa85a),.50f);g.setColour(C::C(0xffffe0a8));g.fillEllipse(x-1.6f,y-3.f,3.2f,5.f);}
        for(int i=0;i<2;++i){const float x0=i?W*.64f:W*.08f,w=W*.26f;
            k.poly({{x0,H*.78f},{x0+w,H*.78f},{x0+w*.92f,H*.98f},{x0+w*.08f,H*.98f}},C::C(0xff0b0a16).withAlpha(.9f));k.line(x0,H*.78f,x0+w,H*.78f,juce::Colours::white.withAlpha(.10f),1.f);}
        k.fog(hz+H*.02f,H*.14f,C::C(0xff8a86d0),.12f);break; }
    case 17: { // ELEVATOR SHAFT: steel tube, rails, floor lights, far door
        room(k,{0xff161c20,0xff1f272c,0xff101417,0xff2e4a3f,0xff6a8a8f,0xff7aff9a,Rib::Rect,10,.075f,.04f,.50f,.40f,true,false,.45f,.55f});
        for(int sd=0;sd<2;++sd) for(int j=0;j<2;++j){const float x=W*.5f+(sd?1.f:-1.f)*W*(.045f+.025f*static_cast<float>(j));
            k.line(x,-4.f,x,hz,C::C(0xffa9c4c8).withAlpha(.28f),1.4f);k.line(x+(sd?1.f:-1.f)*W*.34f,-4.f,x+(sd?1.f:-1.f)*W*.34f,H,C::C(0xffa9c4c8).withAlpha(.18f),2.f);}
        for(int i=0;i<6;++i){const float y=H*(.06f+.14f*static_cast<float>(i));k.line(W*.30f,y,W*.70f,y,C::C(0xff7aff9a).withAlpha(.05f),1.f);}
        k.fog(hz,H*.10f,C::C(0xff7aff9a),.08f);break; }
    case 18: { // SNOWFIELD: polar night, aurora, pines, flurries
        sky(k,0xff060e1c,0xff17304a,0xff4a6e8a);stars(k,60,hz*.6f,.8f);
        for(int r2=0;r2<3;++r2){std::vector<P> top,bot;for(float x=0.f;x<=W+8.f;x+=8.f){const float y=H*(.10f+.05f*static_cast<float>(r2))+std::sin(x*.012f+static_cast<float>(r2)*2.f)*H*.06f;top.push_back({x,y});bot.push_back({x,y+H*(.14f+.04f*static_cast<float>(r2))});}
            std::vector<P> pts=top;for(auto it=bot.rbegin();it!=bot.rend();++it)pts.push_back(*it);
            k.polyGrad(pts,juce::ColourGradient(C::C(r2==1?0xff7affc8:0xff4cf2a0).withAlpha(.22f),0.f,H*.1f,C::C(0xff4cf2a0).withAlpha(0.f),0.f,H*.40f,false));}
        k.glowAt(W*.78f,H*.18f,W*.22f,C::C(0xffd8ecff),.40f);g.setColour(C::C(0xffeef6ff).withAlpha(.95f));g.fillEllipse(W*.78f-10.f,H*.18f-10.f,20.f,20.f);
        k.ridge(hz-2.f,H*.05f,3,C::C(0xff9fb8cc),.7f);
        for(int i=0;i<16;++i){const float x=W*C::rnd(i,1),h=H*(.10f+.12f*C::rnd(i,2));pine(k,x,hz+2.f,h,C::C(0xff0b1622),.0f);}
        k.vgrad(hz-1.f,H,C::C(0xff5a7a96),C::C(0xff0c1824));
        for(int i=0;i<70;++i){const float x=W*C::rnd(i,5),d=C::rnd(i,6)*C::rnd(i,6),y=C::lerp(hz+4.f,H,d);g.setColour(juce::Colours::white.withAlpha(.25f+.4f*d));g.fillEllipse(x,y,1.f+2.f*d,1.f+1.f*d);}
        for(int i=0;i<60;++i){const float x=W*C::rnd(i,8),y=H*C::rnd(i,9);g.setColour(juce::Colours::white.withAlpha(.20f+.30f*C::rnd(i,10)));g.fillEllipse(x,y,1.4f,1.4f);}
        k.fog(hz+H*.03f,H*.12f,C::C(0xffbcd8ee),.18f);break; }
    case 19: { // ARENA: night stadium, tiered stands, floodlights
        k.vgrad(0.f,hz,C::C(0xff080b14),C::C(0xff1b2236));
        for(int t=0;t<5;++t){const float y=hz-H*(.04f+.075f*static_cast<float>(t)),a=.9f-.12f*static_cast<float>(t);   // tiers
            k.polyGrad({{-W*.05f,y},{W*1.05f,y},{W*1.05f,y+H*.07f},{-W*.05f,y+H*.07f}},juce::ColourGradient(C::C(0xff2a3350).withAlpha(a),0.f,y,C::C(0xff0e1220).withAlpha(a),0.f,y+H*.07f,false));
            for(int i=0;i<46;++i){const float x=W*C::rnd(i+t*50,1);g.setColour(C::C(i%3?0xff8aa0d8:0xffd8c88a).withAlpha(.22f));g.fillEllipse(x,y+H*.012f+H*.04f*C::rnd(i+t*50,2),1.8f,1.8f);}}
        for(int sd=0;sd<2;++sd){const float tx=sd?W*.92f:W*.08f;k.line(tx,H*.04f,tx,hz,C::C(0xff0a0d16),2.f);
            k.rays(tx,H*.06f,.6f,H*.9f,3,C::C(0xffe8f0ff),.09f,sd?-.5f:.5f);k.glowAt(tx,H*.06f,W*.12f,C::C(0xffe8f0ff),.65f);}
        ground(k,0xff06100c,0xff17331f);
        for(int i=-5;i<=5;++i) k.line(W*.5f+static_cast<float>(i)*W*.02f,hz+2.f,W*.5f+static_cast<float>(i)*W*.22f,H,juce::Colours::white.withAlpha(.06f),1.f);
        g.setColour(juce::Colours::white.withAlpha(.07f));g.drawEllipse(W*.30f,hz+H*.10f,W*.40f,H*.12f,1.2f);
        k.fog(hz+H*.02f,H*.12f,C::C(0xffa8b8e8),.14f);break; }
    default: k.vgrad(0.f,H,C::C(0xff06100d),C::C(0xff0d1c16));break;
    }
    k.vignette(.62f);
}
}
