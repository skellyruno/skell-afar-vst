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
        for(int i=0;i<16;++i) {
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
        g.setColour(Theme::bone);g.setFont(titleFont(15));
        g.drawText("SPACE BROWSER",14,9,getWidth()-28,20,juce::Justification::centredLeft);
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
        if (covers[static_cast<std::size_t>(id)].isValid()) {
            g.drawImage(covers[static_cast<std::size_t>(id)],0,0,w,h,0,0,480,200);
            g.setColour(juce::Colours::black.withAlpha(.50f));g.fillRect(0,0,w,h);
        } else { g.fillAll(Theme::panelRaised); }
        if(selected){g.setColour(Theme::green);g.drawRect(0,0,w,h,2);}
        g.setColour(Theme::bone);g.setFont(titleFont(13.5f));
        g.drawFittedText(spec.name,12,8,w-20,23,juce::Justification::centredLeft,1);
        g.setColour(Theme::muted);g.setFont(juce::Font(juce::FontOptions(11.4f)));
        g.drawFittedText(spec.description,12,34,w-20,22,juce::Justification::centredLeft,1);
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
        list.updateContent();list.repaint();
    }
    int category=0;
    std::array<juce::TextButton,4> tabs;
    juce::TextEditor search;
    juce::ListBox list;
    std::vector<int> filtered;
    std::array<juce::Image,16> covers;
};
}
