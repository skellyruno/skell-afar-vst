#pragma once
#include <JuceHeader.h>
namespace afar::ui {
namespace Theme {
inline const juce::Colour background {0xff06090a};
inline const juce::Colour panel {0xff12171a};
inline const juce::Colour panelRaised {0xff1b2124};
inline const juce::Colour green {0xff39ff3a};
inline const juce::Colour greenLight {0xff8dff8a};
inline const juce::Colour greenDim {0xff135c1a};
inline const juce::Colour bone {0xffe8e6da};
inline const juce::Colour muted {0xff8a9a90};
inline const juce::Colour ice {0xff7fd6ff};
inline const juce::Colour fire {0xffff5a2a};
inline const juce::Colour violet {0xff6a4dff};
inline constexpr int designW=1100,designH=700;
inline constexpr int topBar=56;
inline constexpr int browserX=20,browserY=72,browserW=280,browserH=568;
}
inline void panel(juce::Graphics& g,juce::Rectangle<float> r,float corner=10.f) {
    juce::ColourGradient fill(juce::Colour(0xff182024),r.getX(),r.getY(),Theme::panel.darker(.3f),r.getX(),r.getBottom(),false);
    g.setGradientFill(fill);g.fillRoundedRectangle(r,corner);
    g.setColour(juce::Colours::white.withAlpha(.04f));
    g.drawHorizontalLine(juce::roundToInt(r.getY())+1,r.getX()+corner,r.getRight()-corner);
    g.setColour(Theme::greenDim.withAlpha(.72f));g.drawRoundedRectangle(r.reduced(.5f),corner,1.1f);
}
inline juce::Font titleFont(float size) {return juce::Font(juce::FontOptions(size).withStyle("Bold"));}
// Consistent section heading: neon accent bar + bold bone-white caps.
inline void sectionTitle(juce::Graphics& g,const juce::String& text,float x,float y,float w,float size=13.f) {
    g.setColour(Theme::green);g.fillRoundedRectangle(x,y+3.f,3.f,size,1.5f);
    g.setColour(Theme::bone);g.setFont(titleFont(size));
    g.drawText(text,x+10.f,y,w-10.f,size+6.f,juce::Justification::centredLeft);
}
}
