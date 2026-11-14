#pragma once
#include "Theme.h"
#include "../dsp/SceneModel.h"
#include <array>
#include <vector>
#include <functional>
#include <BinaryData.h>

namespace afar::ui {
class SpaceBrowser final : public juce::Component, private juce::ListBoxModel {
public:
    SpaceBrowser() : list("SpaceCards",this) {
        const char* names[]={"ALL","UNDER","OUT","IN"};
        for(int i=0;i<4;++i){
            tabs[i].setButtonText(names[i]);
            tabs[i].onClick=[this,i]{category=i;refresh();};
            tabs[i].setColour(juce::TextButton::buttonColourId,Theme::panelRaised);
            tabs[i].setColour(juce::TextButton::textColourOffId,Theme::bone);
            addAndMakeVisible(tabs[i]);
        }
        search.setIndents(9,7);
        search.setFont(juce::Font(juce::FontOptions(13.f)));
        search.setTextToShowWhenEmpty("Search spaces...",Theme::muted);
        search.setColour(juce::TextEditor::backgroundColourId,Theme::panelRaised);
        search.setColour(juce::TextEditor::textColourId,Theme::bone);
        search.setColour(juce::TextEditor::outlineColourId,Theme::greenDim);
        search.onTextChange=[this]{refresh();};
        addAndMakeVisible(search);
        list.setRowHeight(67);
        list.setColour(juce::ListBox::backgroundColourId,Theme::panel);
        list.setColour(juce::ListBox::outlineColourId,Theme::greenDim);
        addAndMakeVisible(list);
        for(int i=0;i<20;++i) {
            int sz=0;
            const auto resource=juce::String::formatted("space_%02d_png",i);
            if (const auto* bytes=BinaryData::getNamedResource(resource.toRawUTF8(),sz))
                covers[static_cast<std::size_t>(i)]=juce::ImageFileFormat::loadFrom(bytes,static_cast<std::size_t>(sz));
        }
        refresh();
    }
    std::function<void(int)> onSelect;
    void setSelectedSpace(int s) {
        for(int i=0;i<static_cast<int>(filtered.size());++i) {
            if (filtered[static_cast<std::size_t>(i)]==s) {
                if(list.getSelectedRow()!=i) list.selectRow(i, true, false);
                return;
            }
        }
    }
    void paint(juce::Graphics& g) override {
        panel(g,getLocalBounds().toFloat());
        sectionTitle(g,"SPACE BROWSER",14.f,9.f,static_cast<float>(getWidth())-28.f,15.f);
    }
    void resized() override {
        const int w=getWidth()-22;
        for(int i=0;i<4;++i)tabs[i].setBounds(11+i*(w/4),39,(w/4)-2,27);
        search.setBounds(11,73,w,29);
        list.setBounds(11,112,w,getHeight()-124);
    }
private:
    int getNumRows() override {return static_cast<int>(filtered.size());}
    void paintListBoxItem(int row,juce::Graphics& g,int w,int h,bool selected) override {
        if(row<0 || row>=static_cast<int>(filtered.size()))return;
        const int id=filtered[static_cast<std::size_t>(row)];
        const auto& spec=getSpace(id);
        const juce::Rectangle<float> card(1.f,2.f,static_cast<float>(w)-2.f,static_cast<float>(h)-4.f);
        juce::Path clip;clip.addRoundedRectangle(card,7.f);
        g.saveState();
        g.reduceClipRegion(clip);
        const auto& img=covers[static_cast<std::size_t>(id)];
        if(img.isValid()) {
            // crop (not stretch) the cover to the card's aspect ratio
            const float srcW=static_cast<float>(img.getWidth()),srcFullH=static_cast<float>(img.getHeight());
            const float srcH=std::min(srcFullH,srcW*card.getHeight()/card.getWidth());
            g.drawImage(img,juce::roundToInt(card.getX()),juce::roundToInt(card.getY()),juce::roundToInt(card.getWidth()),juce::roundToInt(card.getHeight()),
                        0,juce::roundToInt((srcFullH-srcH)*.5f),img.getWidth(),juce::roundToInt(srcH));
        } else { g.fillAll(Theme::panelRaised); }
        juce::ColourGradient scrim(juce::Colours::black.withAlpha(.86f),card.getX(),0.f,juce::Colours::black.withAlpha(.30f),card.getRight(),0.f,false);
        g.setGradientFill(scrim);g.fillRect(card);
        if(selected){g.setColour(Theme::green.withAlpha(.13f));g.fillRect(card);g.setColour(Theme::green);g.fillRect(card.getX(),card.getY(),3.f,card.getHeight());}
        g.restoreState();
        g.setColour(selected?Theme::green:Theme::greenDim.withAlpha(.55f));
        g.drawRoundedRectangle(card.reduced(.5f),7.f,selected?1.5f:1.f);
        g.setColour(selected?Theme::greenLight:Theme::bone);g.setFont(titleFont(13.5f));
        g.drawFittedText(spec.name,14,9,w-86,22,juce::Justification::centredLeft,1);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(11.4f)));
        g.drawFittedText(spec.description,14,35,w-24,22,juce::Justification::centredLeft,1);
        const char* tag=spec.category==Category::Underground?"UNDER":spec.category==Category::Outdoor?"OUT":"IN";
        const juce::Rectangle<float> pill(static_cast<float>(w)-58.f,11.f,46.f,15.f);
        g.setColour(juce::Colours::black.withAlpha(.55f));g.fillRoundedRectangle(pill,7.5f);
        g.setColour(Theme::greenDim.brighter(.4f));g.drawRoundedRectangle(pill,7.5f,1.f);
        g.setColour(Theme::greenLight);g.setFont(titleFont(8.5f));g.drawText(tag,pill,juce::Justification::centred);
    }
    void selectedRowsChanged(int row) override {
        if(row>=0 && row<static_cast<int>(filtered.size()) && onSelect)
            onSelect(filtered[static_cast<std::size_t>(row)]);
    }
    void refresh() {
        filtered.clear();
        const auto term=search.getText().trim().toLowerCase();
        for (int i=0;i<static_cast<int>(spaces.size());++i) {
            const auto& s=getSpace(i);
            if (category==1 && s.category!=Category::Underground)continue;
            if (category==2 && s.category!=Category::Outdoor)continue;
            if (category==3 && s.category!=Category::Indoor)continue;
            if (!juce::String(s.name).toLowerCase().contains(term) && !juce::String(s.description).toLowerCase().contains(term))continue;
            filtered.push_back(i);
        }
        for(int i=0;i<4;++i){
            tabs[i].setColour(juce::TextButton::buttonColourId,i==category?Theme::greenDim.brighter(.35f):Theme::panelRaised);
            tabs[i].setColour(juce::TextButton::textColourOffId,i==category?Theme::greenLight:Theme::bone);
        }
        list.updateContent();list.repaint();
    }
    int category=0;
    std::array<juce::TextButton,4> tabs;
    juce::TextEditor search;
    juce::ListBox list;
    std::vector<int> filtered;
    std::array<juce::Image,20> covers;
};
}
