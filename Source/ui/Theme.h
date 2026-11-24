#pragma once
#include <JuceHeader.h>
namespace afar::ui {
// Design language: deep neutral charcoal surfaces, hairline neutral borders, ONE accent (phosphor green) used only
// for state and signal (active, value arcs, meters, the logo). Depth comes from light, not from outlines:
// a faint top highlight, a darker lower edge and recessed "wells" for readouts.
namespace Theme {
inline const juce::Colour background {0xff07090b};
inline const juce::Colour panel {0xff12171a};
inline const juce::Colour panelRaised {0xff1c2327};
inline const juce::Colour panelDeep {0xff0a0e10};
inline const juce::Colour green {0xff4cf26e};
inline const juce::Colour greenLight {0xffa4ffb4};
inline const juce::Colour greenDim {0xff1b5a2c};
inline const juce::Colour bone {0xffeae7dc};
inline const juce::Colour muted {0xff8f9a95};
inline const juce::Colour faint {0xff5d6863};
inline const juce::Colour ice {0xff7fd6ff};
inline const juce::Colour fire {0xffff6a3a};
inline const juce::Colour violet {0xff7a5cff};
inline const juce::Colour amber {0xffe8bd7a};
inline const juce::Colour line {0x1fffffff};       // hairline borders: white at 12%
inline const juce::Colour lineSoft {0x12ffffff};   // separators: white at 7%
inline constexpr int designW=1100,designH=700;
inline constexpr int topBar=56;
inline constexpr int browserX=20,browserY=72,browserW=280,browserH=568;
}
inline juce::Font titleFont(float size) {return juce::Font(juce::FontOptions(size).withStyle("Bold"));}
inline juce::Font captionFont(float size=9.5f) {return juce::Font(juce::FontOptions(size));}

// Raised surface: vertical gradient, light catching the top edge, darker lower edge, neutral hairline.
inline void panel(juce::Graphics& g,juce::Rectangle<float> r,float corner=10.f) {
    juce::ColourGradient fill(juce::Colour(0xff1a2125),r.getX(),r.getY(),juce::Colour(0xff0f1417),r.getX(),r.getBottom(),false);
    g.setGradientFill(fill);g.fillRoundedRectangle(r,corner);
    g.setColour(Theme::line);g.drawRoundedRectangle(r.reduced(.5f),corner,1.f);
    g.setColour(juce::Colour(0x14ffffff));
    g.drawHorizontalLine(juce::roundToInt(r.getY())+1,r.getX()+corner,r.getRight()-corner);
    g.setColour(juce::Colour(0x59000000));
    g.drawHorizontalLine(juce::roundToInt(r.getBottom())-2,r.getX()+corner,r.getRight()-corner);
}
// Recessed well for readouts and displays: dark fill, shadow falling from the top edge, light catching the bottom lip.
inline void inset(juce::Graphics& g,juce::Rectangle<float> r,float corner=5.f) {
    g.setColour(Theme::panelDeep);g.fillRoundedRectangle(r,corner);
    juce::ColourGradient shade(juce::Colour(0x8c000000),r.getX(),r.getY(),juce::Colour(0x00000000),r.getX(),r.getY()+std::min(9.f,r.getHeight()*.5f),false);
    g.setGradientFill(shade);g.fillRoundedRectangle(r.withHeight(std::min(9.f,r.getHeight()*.5f)),corner);
    g.setColour(Theme::lineSoft);g.drawRoundedRectangle(r.reduced(.5f),corner,1.f);
    g.setColour(juce::Colour(0x0fffffff));g.drawHorizontalLine(juce::roundToInt(r.getBottom())-1,r.getX()+corner,r.getRight()-corner);
}
// Soft light: one radial gradient (smooth falloff, no banding, no blur pass; identical in every renderer).
inline void glow(juce::Graphics& g,float cx,float cy,float radius,juce::Colour c,float strength=1.f) {
    juce::ColourGradient light(c.withAlpha(std::min(1.f,.30f*strength)),cx,cy,c.withAlpha(0.f),cx+radius,cy,true);
    g.setGradientFill(light);g.fillEllipse(cx-radius,cy-radius,radius*2.f,radius*2.f);
}
inline void hairline(juce::Graphics& g,float x1,float y,float x2,float alpha=1.f) {
    g.setColour(Theme::lineSoft.withMultipliedAlpha(alpha));g.drawHorizontalLine(juce::roundToInt(y),x1,x2);
}
// Consistent section heading: small accent tick + bold bone-white caps.
inline void sectionTitle(juce::Graphics& g,const juce::String& text,float x,float y,float w,float size=13.f) {
    juce::ColourGradient bar(Theme::green,x,y+3.f,Theme::greenDim,x,y+3.f+size,false);
    g.setGradientFill(bar);g.fillRoundedRectangle(x,y+3.f,2.5f,size,1.2f);
    g.setColour(Theme::bone);g.setFont(titleFont(size));
    g.drawText(text,x+10.f,y,w-10.f,size+6.f,juce::Justification::centredLeft);
}
// Small muted caption (field labels, axis names).
inline void caption(juce::Graphics& g,const juce::String& text,float x,float y,float w,float h,juce::Justification j=juce::Justification::centredLeft,float size=9.5f) {
    g.setColour(Theme::muted);g.setFont(captionFont(size));g.drawText(text,x,y,w,h,j);
}
}
