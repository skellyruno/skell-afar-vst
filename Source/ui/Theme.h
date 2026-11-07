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
inline constexpr int sceneX=320,sceneY=72,sceneW=480,sceneH=332;
inline constexpr int padX=820,padY=72,padW=260,padH=260;
inline constexpr int walkX=820,walkY=346,walkW=260,walkH=215;
inline constexpr int lowerX=314,lowerY=575,lowerW=774,lowerH=115;
}
inline void panel(juce::Graphics& g,juce::Rectangle<float> r,float corner=10.f) {
    g.setColour(Theme::panel);g.fillRoundedRectangle(r,corner);
    g.setColour(Theme::greenDim.withAlpha(.72f));g.drawRoundedRectangle(r,corner,1.1f);
}
inline juce::Font titleFont(float size) {return juce::Font(juce::FontOptions(size).withStyle("Bold"));}
}
