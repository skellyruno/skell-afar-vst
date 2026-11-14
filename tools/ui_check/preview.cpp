// Renders the REAL editor classes (paint code included) against the structural JUCE mock and writes the
// recorded drawing calls to stdout for render.py. Usage:
//   preview [detail] [key=value ...]   (parameter raw values; meterPeak/meterDistance/... set the meters)
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ui/DetailPanel.h"
#include "ui/Theme.h"
#include "ui/Controls.h"
#include <cstdlib>
#include <cstring>

namespace Theme = afar::ui::Theme;
static void box(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour fill, juce::Colour outline, float corner) {
    g.setColour(fill); g.fillRoundedRectangle(r, corner);
    g.setColour(outline); g.drawRoundedRectangle(r.reduced(.5f), corner, 1.f);
}
// Draws widgets that JUCE paints through the look-and-feel. Sliders and toggles call the REAL AfarLookAndFeel;
// stock JUCE widgets (combo box, text button, text editor) are approximated.
static void drawWidget(juce::Component& c, juce::Graphics& g) {
    using namespace juce;
    auto* laf = dynamic_cast<LookAndFeel_V4*>(c.findLAF());
    const float w = static_cast<float>(c.getWidth()), h = static_cast<float>(c.getHeight());
    if (auto* s = dynamic_cast<Slider*>(&c)) {
        if (!laf) return;
        const float a0 = 3.14159265f * 1.2f, a1 = 3.14159265f * 2.8f;
        const float tw = static_cast<float>(std::min(s->getTextBoxWidth(), c.getWidth())), th = static_cast<float>(s->getTextBoxHeight());
        juce::Rectangle<float> tb;
        if (s->getSliderStyle() == Slider::RotaryHorizontalVerticalDrag) {
            laf->drawRotarySlider(g, 0, 0, c.getWidth(), c.getHeight() - s->getTextBoxHeight(), static_cast<float>(s->prop), a0, a1, *s);
            tb = {(w - tw) / 2, h - th, tw, th};
        } else {
            const auto lay = laf->getSliderLayout(*s);
            const float pos = static_cast<float>(lay.sliderBounds.x) + 7.f + static_cast<float>(s->prop) * (static_cast<float>(lay.sliderBounds.w) - 14.f);
            laf->drawLinearSlider(g, lay.sliderBounds.x, lay.sliderBounds.y, lay.sliderBounds.w, lay.sliderBounds.h, pos, pos, pos, s->getSliderStyle(), *s);
            tb = lay.textBoxBounds.toFloat();
        }
        g.setColour(Theme::panel); g.fillRect(tb); g.setColour(Theme::greenDim); g.drawRect(tb, 1.f);
        g.setColour(Theme::bone); g.setFont(Font(FontOptions(s->getSliderStyle() == Slider::LinearHorizontal ? 12.5f : 10.5f)));
        g.drawText(s->valueText(), tb, Justification::centred);
    } else if (auto* l = dynamic_cast<Label*>(&c)) {
        g.setColour(l->colour); g.setFont(l->font); g.drawText(l->text, juce::Rectangle<float>(0.f, 0.f, w, h), l->just);
    } else if (auto* t = dynamic_cast<ToggleButton*>(&c)) {
        if (laf) laf->drawToggleButton(g, *t, false, false);
    } else if (auto* b = dynamic_cast<TextButton*>(&c)) {
        const auto bg = b->colours.count(TextButton::buttonColourId) ? b->colours[TextButton::buttonColourId] : Theme::panelRaised;
        const auto fg = b->colours.count(TextButton::textColourOffId) ? b->colours[TextButton::textColourOffId] : Theme::bone;
        box(g, {0.f, 0.f, w, h}, bg, Theme::greenDim.brighter(.2f), 4.f);
        g.setColour(fg); g.setFont(Font(FontOptions(std::min(16.f, h * .6f)))); g.drawText(b->text, juce::Rectangle<float>(0.f, 0.f, w, h), Justification::centred);
    } else if (auto* cb = dynamic_cast<ComboBox*>(&c)) {
        box(g, {0.f, 0.f, w, h}, Theme::panelRaised, Theme::greenDim, 3.f);
        g.setColour(Theme::bone); g.setFont(Font(FontOptions(std::min(16.f, h * .6f))));
        g.drawText(cb->text, juce::Rectangle<float>(8.f, 0.f, w - 30.f, h), Justification::centredLeft);
        g.setColour(Theme::greenLight); g.drawLine(w - 20.f, h * .5f - 2.f, w - 14.f, h * .5f + 3.f, 1.6f); g.drawLine(w - 14.f, h * .5f + 3.f, w - 8.f, h * .5f - 2.f, 1.6f);
    } else if (dynamic_cast<TextEditor*>(&c)) {
        box(g, {0.f, 0.f, w, h}, Theme::panelRaised, Theme::greenDim, 2.f);
        g.setColour(Theme::muted); g.setFont(Font(FontOptions(13.f))); g.drawText("Search spaces...", juce::Rectangle<float>(9.f, 0.f, w - 9.f, h), Justification::centredLeft);
    } else if (auto* lb = dynamic_cast<ListBox*>(&c)) {
        if (!lb->model) return;
        const int rows = lb->model->getNumRows();
        for (int r = 0; r < rows && r * lb->rowH < c.getHeight(); ++r) {
            std::printf("PUSH 0 %d\n", r * lb->rowH);
            lb->model->paintListBoxItem(r, g, c.getWidth(), lb->rowH, r == lb->selRow);
            std::printf("POP\n");
        }
    }
}

static void renderTree(juce::Component& c) {
    juce::Graphics g;
    c.paint(g);
    drawWidget(c, g);
    for (auto* child : c.children) {
        if (!child->isVisible()) continue;
        std::printf("PUSH %d %d\n", child->getX(), child->getY());
        renderTree(*child);
        std::printf("POP\n");
    }
}

int main(int argc, char** argv) {
    juce::Graphics::sink = stdout;
    SkellAfarAudioProcessor processor;
    bool detail = false;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "detail")) { detail = true; continue; }
        const char* eq = std::strchr(argv[i], '=');
        if (!eq) continue;
        const std::string key(argv[i], static_cast<size_t>(eq - argv[i])), val(eq + 1);
        const float v = static_cast<float>(std::atof(val.c_str()));
        if (key == "meterPeak") processor.meterPeak = v;
        else if (key == "meterDistance") processor.meterDistance = v;
        else if (key == "meterClarity") processor.meterClarity = v;
        else if (key == "meterDrr") processor.meterDrr = v;
        else if (key == "meterEffort") processor.meterEffort = v;
        else processor.parameters.getRawParameterValue(key)->store(v);
    }
    if (detail) {
        afar::ui::AfarLookAndFeel look;   // the panel normally inherits this from the editor surface
        afar::ui::DetailPanel panel(processor);
        panel.setLookAndFeel(&look);
        panel.setBounds(0, 0, 774, 489);
        renderTree(panel);
    } else {
        SkellAfarAudioProcessorEditor editor(processor);
        for (int i = 0; i < 40; ++i) for (auto* t : juce::Timer::registry()) t->timerCallback();   // settle meters / animation
        renderTree(editor);
    }
}
