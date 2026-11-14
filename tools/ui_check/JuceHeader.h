#pragma once
// Developer-only structural mock of the JUCE API subset used by the plugin UI. It is NOT JUCE: it exists to
// compile-check the editor sources (typos, scoping, wrong overload sets such as mixing int/float
// coordinates in Graphics calls) and to record drawing calls for off-screen rendering (see preview.py).
// Overload sets of Graphics/Rectangle mirror JUCE 8; rarely used members are catch-alls.
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace juce {
enum NotificationType { dontSendNotification, sendNotification, sendNotificationSync, sendNotificationAsync };
template <class T> constexpr T jlimit(T lo, T hi, T v) { return v < lo ? lo : (v > hi ? hi : v); }
inline int roundToInt(double v) { return static_cast<int>(std::floor(v + .5)); }
inline int roundToInt(float v) { return static_cast<int>(std::floor(v + .5f)); }
template <class T> constexpr T jmin(T a, T b) { return a < b ? a : b; }
template <class T> constexpr T jmax(T a, T b) { return a > b ? a : b; }

// ---------------------------------------------------------------- String
class String {
public:
    String() = default;
    String(const char* c) : s(c ? c : "") {}
    String(const std::string& x) : s(x) {}
    String(int v) : s(std::to_string(v)) {}
    String(float v) : s(std::to_string(v)) {}
    String(double v) : s(std::to_string(v)) {}
    String(float v, int dp, bool = false) { char b[64]; std::snprintf(b, 64, "%.*f", dp, static_cast<double>(v)); s = b; }
    String(double v, int dp, bool = false) { char b[64]; std::snprintf(b, 64, "%.*f", dp, v); s = b; }
    static String fromUTF8(const char* c) { return String(c); }
    static String formatted(const char* f, ...) { char b[256]; va_list a; va_start(a, f); std::vsnprintf(b, 256, f, a); va_end(a); return String(b); }
    bool isEmpty() const { return s.empty(); }
    bool isNotEmpty() const { return !s.empty(); }
    String trim() const { size_t a = s.find_first_not_of(' '), b = s.find_last_not_of(' '); return a == std::string::npos ? String() : String(s.substr(a, b - a + 1)); }
    String toUpperCase() const { std::string t = s; for (auto& c : t) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); return String(t); }
    String toLowerCase() const { std::string t = s; for (auto& c : t) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return String(t); }
    bool contains(const String& o) const { return s.find(o.s) != std::string::npos; }
    bool startsWith(const String& o) const { return s.rfind(o.s, 0) == 0; }
    bool containsIgnoreCase(const String& o) const { return toLowerCase().s.find(o.toLowerCase().s) != std::string::npos; }
    String retainCharacters(const String& keep) const { std::string t; for (char c : s) if (keep.s.find(c) != std::string::npos) t += c; return String(t); }
    float getFloatValue() const { return static_cast<float>(std::atof(s.c_str())); }
    double getDoubleValue() const { return std::atof(s.c_str()); }
    int getIntValue() const { return std::atoi(s.c_str()); }
    int compareNatural(const String& o) const { return s.compare(o.s); }
    const char* toRawUTF8() const { return s.c_str(); }
    std::string toStdString() const { return s; }
    friend String operator+(const String& a, const String& b) { return String(a.s + b.s); }
    friend bool operator==(const String& a, const String& b) { return a.s == b.s; }
    friend bool operator!=(const String& a, const String& b) { return a.s != b.s; }
private:
    std::string s;
};

// ---------------------------------------------------------------- geometry / colour
template <class T> class Rectangle {
public:
    Rectangle() = default;
    Rectangle(T x_, T y_, T w_, T h_) : x(x_), y(y_), w(w_), h(h_) {}
    T getX() const { return x; } T getY() const { return y; } T getWidth() const { return w; } T getHeight() const { return h; }
    T getRight() const { return x + w; } T getBottom() const { return y + h; }
    T getCentreX() const { return x + w / 2; } T getCentreY() const { return y + h / 2; }
    Rectangle reduced(T a) const { return {x + a, y + a, w - 2 * a, h - 2 * a}; }
    Rectangle reduced(T dx, T dy) const { return {x + dx, y + dy, w - 2 * dx, h - 2 * dy}; }
    Rectangle expanded(T a) const { return {x - a, y - a, w + 2 * a, h + 2 * a}; }
    Rectangle withTrimmedLeft(T a) const { return {x + a, y, w - a, h}; }
    Rectangle withTrimmedRight(T a) const { return {x, y, w - a, h}; }
    Rectangle withTrimmedTop(T a) const { return {x, y + a, w, h - a}; }
    Rectangle withTrimmedBottom(T a) const { return {x, y, w, h - a}; }
    Rectangle<float> toFloat() const { return {static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h)}; }
    bool contains(T px, T py) const { return px >= x && py >= y && px < x + w && py < y + h; }
    T x{}, y{}, w{}, h{};
};
struct Justification {
    constexpr Justification(int f_ = 0) : f(f_) {}
    static constexpr int centred = 1, centredLeft = 2, centredRight = 3, topLeft = 4, left = 5, right = 6;
    int f;
};
class Colour {
public:
    Colour() = default;
    explicit Colour(uint32_t argb) : a(((argb >> 24) & 255) / 255.f), r(((argb >> 16) & 255) / 255.f), g(((argb >> 8) & 255) / 255.f), b((argb & 255) / 255.f) {}
    Colour withAlpha(float x) const { Colour c = *this; c.a = x; return c; }
    Colour brighter(float k = .4f) const { Colour c = *this; auto f = [&](float v) { return std::min(1.f, v + (1.f - v) * k); }; c.r = f(r); c.g = f(g); c.b = f(b); return c; }
    Colour darker(float k = .4f) const { Colour c = *this; c.r *= 1.f - k; c.g *= 1.f - k; c.b *= 1.f - k; return c; }
    Colour interpolatedWith(Colour o, float t) const { Colour c; c.r = r + (o.r - r) * t; c.g = g + (o.g - g) * t; c.b = b + (o.b - b) * t; c.a = a + (o.a - a) * t; return c; }
    float a = 1.f, r = 0.f, g = 0.f, b = 0.f;
};
struct Colours { static inline const Colour black {0xff000000u}, white {0xffffffffu}; };
class ColourGradient {
public:
    ColourGradient(Colour c1_, float x1_, float y1_, Colour c2_, float x2_, float y2_, bool radial_) : c1(c1_), c2(c2_), x1(x1_), y1(y1_), x2(x2_), y2(y2_), radial(radial_) {}
    Colour c1, c2; float x1, y1, x2, y2; bool radial;
};
class AffineTransform { public: static AffineTransform scale(float) { return {}; } };
class FontOptions { public: FontOptions(float s = 14.f) : size(s) {} FontOptions withStyle(const char* st) const { FontOptions f = *this; f.bold = std::string(st) == "Bold"; return f; } float size; bool bold = false; };
class Font { public: Font() = default; Font(FontOptions o) : opt(o) {} FontOptions opt; };
template <class T> class BorderSize { public: BorderSize(T, T, T, T) {} };
class Path {
public:
    void startNewSubPath(float x, float y) { sub.push_back({{x, y}}); }
    void lineTo(float x, float y) { sub.back().push_back({x, y}); }
    void quadraticTo(float cx, float cy, float x, float y) { auto p = sub.back().back(); for (int i = 1; i <= 16; ++i) { float t = i / 16.f, u = 1 - t; sub.back().push_back({u * u * p.first + 2 * u * t * cx + t * t * x, u * u * p.second + 2 * u * t * cy + t * t * y}); } }
    void cubicTo(float c1x, float c1y, float c2x, float c2y, float x, float y) { auto p = sub.back().back(); for (int i = 1; i <= 24; ++i) { float t = i / 24.f, u = 1 - t; sub.back().push_back({u*u*u*p.first + 3*u*u*t*c1x + 3*u*t*t*c2x + t*t*t*x, u*u*u*p.second + 3*u*u*t*c1y + 3*u*t*t*c2y + t*t*t*y}); } }
    void addTriangle(float a, float b, float c, float d, float e, float f) { sub.push_back({{a, b}, {c, d}, {e, f}, {a, b}}); }
    void addRoundedRectangle(Rectangle<float>, float) {}
    void closeSubPath() { if (!sub.empty()) sub.back().push_back(sub.back().front()); }
    // angles clockwise from 12 o'clock, as in JUCE
    void addCentredArc(float cx, float cy, float rx, float ry, float /*rotation*/, float from, float to, bool startAsNew) {
        if (startAsNew || sub.empty()) sub.push_back({});
        for (int i = 0; i <= 32; ++i) { float a = from + (to - from) * i / 32.f; sub.back().push_back({cx + rx * std::sin(a), cy - ry * std::cos(a)}); }
    }
    std::vector<std::vector<std::pair<float, float>>> sub;
};
class PathStrokeType { public: enum JointStyle { mitered, curved, beveled }; enum EndCapStyle { butt, square, rounded };
    PathStrokeType(float t_, JointStyle = mitered, EndCapStyle = butt) : t(t_) {} float t; };
class Image { public: enum PixelFormat { ARGB, RGB, SingleChannel }; Image() = default; Image(PixelFormat, int w_, int h_, bool) : w(w_), h(h_), recs(std::make_shared<std::string>()) {}
    bool isValid() const { return w > 0; } int getWidth() const { return w; } int getHeight() const { return h; } int w = 0, h = 0;
    std::string tag; std::shared_ptr<std::string> recs; };
struct ImageFileFormat {
    // The mock "bytes" are the asset name; width/height come from the real PNG header in UI_ASSETS_DIR.
    static Image loadFrom(const void* data, size_t) {
        Image i; i.tag = static_cast<const char*>(data); i.w = i.h = 96;
#ifdef UI_ASSETS_DIR
        if (std::FILE* f = std::fopen((std::string(UI_ASSETS_DIR) + "/" + i.tag + ".png").c_str(), "rb")) {
            unsigned char h[24]; if (std::fread(h, 1, 24, f) == 24) { i.w = (h[16] << 24) | (h[17] << 16) | (h[18] << 8) | h[19]; i.h = (h[20] << 24) | (h[21] << 16) | (h[22] << 8) | h[23]; }
            std::fclose(f);
        }
#endif
        return i;
    }
};
class LowLevelGraphicsContext { public: float getPhysicalPixelScaleFactor() const { return 1.f; } };

// ---------------------------------------------------------------- Graphics (recording)
class Graphics {
public:
    Graphics() = default;
    explicit Graphics(Image& im) : buf(im.recs.get()) {}
    void setColour(Colour c) { col = c; grad = nullptr; }
    void setGradientFill(const ColourGradient& g) { gradient = g; grad = &gradient; }
    void setFont(Font f) { font = f; }
    void setFont(float s) { font = Font(FontOptions(s)); }
    void fillAll() {} void fillAll(Colour) {}
    void saveState() {} void restoreState() {}
    void reduceClipRegion(const Path&, const AffineTransform& = {}) {}
    void addTransform(const AffineTransform&) {}
    LowLevelGraphicsContext& getInternalContext() { return ctx; }
    // rectangles
    void fillRect(Rectangle<int> r) { rec("FR", {(float)r.x, (float)r.y, (float)r.w, (float)r.h, 0}); }
    void fillRect(Rectangle<float> r) { rec("FR", {r.x, r.y, r.w, r.h, 0}); }
    void fillRect(int x, int y, int w, int h) { rec("FR", {(float)x, (float)y, (float)w, (float)h, 0}); }
    void fillRect(float x, float y, float w, float h) { rec("FR", {x, y, w, h, 0}); }
    void drawRect(int x, int y, int w, int h, int t = 1) { rec("DR", {(float)x, (float)y, (float)w, (float)h, 0, (float)t}); }
    void drawRect(float x, float y, float w, float h, float t = 1.f) { rec("DR", {x, y, w, h, 0, t}); }
    void drawRect(Rectangle<int> r, int t = 1) { drawRect(r.x, r.y, r.w, r.h, t); }
    void drawRect(Rectangle<float> r, float t = 1.f) { drawRect(r.x, r.y, r.w, r.h, t); }
    void fillRoundedRectangle(float x, float y, float w, float h, float c) { rec("FR", {x, y, w, h, c}); }
    void fillRoundedRectangle(Rectangle<float> r, float c) { rec("FR", {r.x, r.y, r.w, r.h, c}); }
    void drawRoundedRectangle(float x, float y, float w, float h, float c, float t) { rec("DR", {x, y, w, h, c, t}); }
    void drawRoundedRectangle(Rectangle<float> r, float c, float t) { rec("DR", {r.x, r.y, r.w, r.h, c, t}); }
    void fillEllipse(float x, float y, float w, float h) { rec("FE", {x, y, w, h}); }
    void fillEllipse(Rectangle<float> r) { rec("FE", {r.x, r.y, r.w, r.h}); }
    void drawEllipse(float x, float y, float w, float h, float t) { rec("DE", {x, y, w, h, t}); }
    void drawEllipse(Rectangle<float> r, float t) { rec("DE", {r.x, r.y, r.w, r.h, t}); }
    void drawLine(float a, float b, float c, float d) { rec("L", {a, b, c, d, 1.f}); }
    void drawLine(float a, float b, float c, float d, float t) { rec("L", {a, b, c, d, t}); }
    void drawHorizontalLine(int y, float l, float r) { rec("L", {l, (float)y + .5f, r, (float)y + .5f, 1.f}); }
    void drawVerticalLine(int x, float t, float b) { rec("L", {(float)x + .5f, t, (float)x + .5f, b, 1.f}); }
    void fillPath(const Path& p, const AffineTransform& = {}) { for (auto& s : p.sub) { std::vector<float> v; for (auto& q : s) { v.push_back(q.first); v.push_back(q.second); } rec("FP", v); } }
    void strokePath(const Path& p, PathStrokeType st, const AffineTransform& = {}) { for (auto& s : p.sub) { std::vector<float> v {st.t}; for (auto& q : s) { v.push_back(q.first); v.push_back(q.second); } rec("SP", v); } }
    // text: all-int or all-float coordinates, exactly like JUCE
    void drawText(const String& t, int x, int y, int w, int h, Justification j, bool = true) { text(t, (float)x, (float)y, (float)w, (float)h, j); }
    void drawText(const String& t, float x, float y, float w, float h, Justification j, bool = true) { text(t, x, y, w, h, j); }
    void drawText(const String& t, Rectangle<int> r, Justification j, bool = true) { text(t, (float)r.x, (float)r.y, (float)r.w, (float)r.h, j); }
    void drawText(const String& t, Rectangle<float> r, Justification j, bool = true) { text(t, r.x, r.y, r.w, r.h, j); }
    void drawFittedText(const String& t, int x, int y, int w, int h, Justification j, int lines, float = 0.f) { text(t, (float)x, (float)y, (float)w, (float)h, j, lines); }
    void drawFittedText(const String& t, Rectangle<int> r, Justification j, int lines, float = 0.f) { text(t, (float)r.x, (float)r.y, (float)r.w, (float)r.h, j, lines); }
    void drawImage(const Image& im, int dx, int dy, int dw, int dh, int sx, int sy, int sw, int sh, bool = false) { image(im, (float)dx, (float)dy, (float)dw, (float)dh, (float)sx, (float)sy, (float)sw, (float)sh); }
    void drawImage(const Image& im, Rectangle<float> r) { image(im, r.x, r.y, r.w, r.h, 0, 0, (float)im.w, (float)im.h); }
    void drawImageAt(const Image& im, int x, int y, bool = false) { image(im, (float)x, (float)y, (float)im.w, (float)im.h, 0, 0, (float)im.w, (float)im.h); }
    static inline std::FILE* sink = nullptr;
private:
    void emit(const char* fmt, ...) {
        char line[1024]; va_list a; va_start(a, fmt); std::vsnprintf(line, sizeof line, fmt, a); va_end(a);
        if (buf) *buf += line; else if (sink) std::fputs(line, sink);
    }
    void image(const Image& im, float dx, float dy, float dw, float dh, float sx, float sy, float sw, float sh) {
        if (!im.tag.empty()) { emit("I %s %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n", im.tag.c_str(), dx, dy, dw, dh, sx, sy, sw, sh); return; }
        if (im.recs && !im.recs->empty()) { emit("PUSH %.3f %.3f\n", dx, dy); if (buf) *buf += *im.recs; else if (sink) std::fputs(im.recs->c_str(), sink); emit("POP\n"); }
    }
    void rec(const char* k, const std::vector<float>& v) {
        emit("%s %.3f %.3f %.3f %.3f", k, col.r, col.g, col.b, col.a);
        if (grad) emit(" G %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %d", grad->c1.r, grad->c1.g, grad->c1.b, grad->c1.a,
                       grad->c2.r, grad->c2.g, grad->c2.b, grad->c2.a, grad->x1, grad->y1, grad->x2, grad->y2, grad->radial ? 1 : 0);
        else emit(" N");
        for (float f : v) emit(" %.3f", static_cast<double>(f));
        emit("\n");
    }
    void text(const String& t, float x, float y, float w, float h, Justification j, int maxLines = 1) {
        emit("T %d %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.2f %d %d %s\n", maxLines, col.r, col.g, col.b, col.a, static_cast<double>(x), static_cast<double>(y),
             static_cast<double>(w), static_cast<double>(h), static_cast<double>(font.opt.size), font.opt.bold ? 1 : 0, j.f, t.toRawUTF8());
    }
    std::string* buf = nullptr;
    Colour col; Font font; ColourGradient gradient {Colour(), 0, 0, Colour(), 0, 0, false}; const ColourGradient* grad = nullptr; LowLevelGraphicsContext ctx;
};

// ---------------------------------------------------------------- components and widgets
class MouseCursor { public: enum StandardCursorType { NormalCursor, DraggingHandCursor }; MouseCursor() = default; MouseCursor(StandardCursorType) {} };
template <class T> struct Point { T x{}, y{}; };
struct MouseEvent { int x = 0, y = 0; Point<float> position; };
class KeyPress { public: static constexpr int returnKey = 1, escapeKey = 2; KeyPress() = default; KeyPress(int) {} };
class LookAndFeel;
class SettableTooltipClient { public: void setTooltip(const String& t) { tip = t; } String getTooltip() const { return tip; } private: String tip; };
class Timer {
public:
    static std::vector<Timer*>& registry() { static std::vector<Timer*> r; return r; }
    virtual ~Timer() { auto& r = registry(); r.erase(std::remove(r.begin(), r.end(), this), r.end()); }
    virtual void timerCallback() = 0;
    void startTimerHz(int) { registry().push_back(this); }
    void stopTimer() {}
};

class Component {
public:
    virtual ~Component() = default;
    virtual void paint(Graphics&) {}
    virtual void resized() {}
    virtual void mouseDown(const MouseEvent&) {}
    virtual void mouseDrag(const MouseEvent&) {}
    virtual void mouseUp(const MouseEvent&) {}
    void setBounds(int x, int y, int w, int h) { bx = x; by = y; bw = w; bh = h; resized(); }
    void setBounds(Rectangle<int> r) { setBounds(r.x, r.y, r.w, r.h); }
    void setSize(int w, int h) { setBounds(bx, by, w, h); }
    Rectangle<int> getBounds() const { return {bx, by, bw, bh}; }
    Rectangle<int> getLocalBounds() const { return {0, 0, bw, bh}; }
    int getWidth() const { return bw; } int getHeight() const { return bh; } int getX() const { return bx; } int getY() const { return by; }
    void repaint() {} void repaint(int, int, int, int) {} void repaint(Rectangle<int>) {}
    void addAndMakeVisible(Component& c) { children.push_back(&c); c.parent = this; c.visible = true; }
    void addAndMakeVisible(Component* c, int = -1) { addAndMakeVisible(*c); }
    void addChildComponent(Component& c) { children.push_back(&c); c.parent = this; }
    void setVisible(bool v) { visible = v; }
    bool isVisible() const { return visible; }
    void toFront(bool) {}
    void setOpaque(bool) {}
    void setMouseCursor(const MouseCursor&) {}
    void setLookAndFeel(LookAndFeel* l) { laf = l; }
    LookAndFeel* findLAF() const { for (const Component* c = this; c; c = c->parent) if (c->laf) return c->laf; return nullptr; }
    void setComponentID(const String& id) { cid = id; }
    String getComponentID() const { return cid; }
    void setTransform(const AffineTransform&) {}
    void setEnabled(bool e) { enabled = e; }
    bool isEnabled() const { return enabled; }
    template <class T> class SafePointer {
    public:
        SafePointer() = default;
        SafePointer(T* c) : p(c) {}
        T* getComponent() const { return p; }
        T* operator->() const { return p; }
    private:
        T* p = nullptr;
    };
    std::vector<Component*> children; Component* parent = nullptr; LookAndFeel* laf = nullptr;
protected:
    int bx = 0, by = 0, bw = 0, bh = 0; bool visible = true, enabled = true; String cid;
};

class LookAndFeel { public: virtual ~LookAndFeel() = default; void setColour(int, Colour) {} };

class Label : public Component {
public:
    enum ColourIds { textColourId = 0x1000281 };
    void setText(const String& t, NotificationType) { text = t; }
    String getText() const { return text; }
    void setFont(const Font& f) { font = f; } void setJustificationType(Justification j) { just = j; } void setBorderSize(BorderSize<int>) {} void setMinimumHorizontalScale(float) {}
    void setColour(int, Colour c) { colour = c; }
    String text; Font font; Justification just {Justification::centredLeft}; Colour colour {0xffe8e6dau};
};
class Button : public Component, public SettableTooltipClient {
public:
    void setButtonText(const String& t) { text = t; }
    String getButtonText() const { return text; }
    bool getToggleState() const { return on; }
    void setToggleState(bool v, NotificationType) { on = v; }
    void setColour(int id, Colour c) { colours[id] = c; }
    std::function<void()> onClick;
    std::map<int, Colour> colours;
    String text; bool on = false;
};
class TextButton : public Button { public: enum ColourIds { buttonColourId = 0x1000100, buttonOnColourId, textColourOffId = 0x1000103, textColourOnId }; };
class ToggleButton : public Button { public: enum ColourIds { textColourId = 0x1006501 }; };
class Slider : public Component, public SettableTooltipClient {
public:
    enum SliderStyle { LinearHorizontal, LinearVertical, RotaryHorizontalVerticalDrag };
    enum TextEntryBoxPosition { NoTextBox, TextBoxLeft, TextBoxRight, TextBoxAbove, TextBoxBelow };
    enum ColourIds { thumbColourId = 0x1001300, textBoxTextColourId = 0x1001700, textBoxBackgroundColourId, textBoxHighlightColourId, textBoxOutlineColourId };
    struct SliderLayout { Rectangle<int> sliderBounds, textBoxBounds; };
    void setSliderStyle(SliderStyle s) { style = s; }
    SliderStyle getSliderStyle() const { return style; }
    void setTextBoxStyle(TextEntryBoxPosition p, bool, int w, int h) { tbPos = p; tbW = w; tbH = h; }
    TextEntryBoxPosition getTextBoxPosition() const { return tbPos; }
    int getTextBoxWidth() const { return tbW; } int getTextBoxHeight() const { return tbH; }
    double getValue() const { return value; }
    void setValue(double v, NotificationType) { value = v; }
    void setRange(double lo, double hi, double) { minV = lo; maxV = hi; }
    void setDoubleClickReturnValue(bool, double) {}
    void setColour(int, Colour) {}
    void updateText() {}
    String valueText() const { return textFromValueFunction ? textFromValueFunction(value) : String(value, 2); }
    std::function<String(double)> textFromValueFunction; std::function<double(const String&)> valueFromTextFunction; double prop = 0.0;
    SliderStyle style = LinearHorizontal; TextEntryBoxPosition tbPos = TextBoxRight; int tbW = 80, tbH = 20; double value = 0, minV = 0, maxV = 1;
};
class ComboBox : public Component, public SettableTooltipClient {
public:
    enum ColourIds { backgroundColourId = 0x1000b00, textColourId, outlineColourId, arrowColourId, focusedOutlineColourId = 0x1000b05 };
    void addItem(const String& t, int id) { items.push_back({t, id}); }
    void addSeparator() {} void addSectionHeading(const String&) {}
    void clear(NotificationType) { items.clear(); }
    int getNumItems() const { return static_cast<int>(items.size()); }
    int getSelectedId() const { return selId; }
    void setSelectedId(int id, NotificationType) { selId = id; }
    int getSelectedItemIndex() const { return selId - 1; }
    void setSelectedItemIndex(int i, NotificationType) { selId = i + 1; if (onChange) onChange(); }
    void setText(const String& t, NotificationType) { text = t; }
    String getText() const { return text; }
    void setColour(int, Colour) {}
    std::function<void()> onChange;
    std::vector<std::pair<String, int>> items; int selId = 0; String text;
};
class TextEditor : public Component {
public:
    enum ColourIds { backgroundColourId = 0x1000200, textColourId, highlightColourId, highlightedTextColourId, outlineColourId, focusedOutlineColourId, shadowColourId };
    void setIndents(int, int) {} void setFont(const Font&) {} void setTextToShowWhenEmpty(const String&, Colour) {} void setColour(int, Colour) {}
    void setJustification(Justification) {}
    String getText() const { return {}; }
    std::function<void()> onTextChange;
};
class CaretComponent { public: enum ColourIds { caretColourId = 0x1000204 }; };
class ScrollBar { public: enum ColourIds { backgroundColourId = 0x1000300, thumbColourId, trackColourId }; };
class ListBoxModel { public: virtual ~ListBoxModel() = default; virtual int getNumRows() = 0; virtual void paintListBoxItem(int, Graphics&, int, int, bool) = 0; virtual void selectedRowsChanged(int) {} };
class ListBox : public Component {
public:
    enum ColourIds { backgroundColourId = 0x1002800, outlineColourId, textColourId };
    ListBox(const String& = {}, ListBoxModel* m = nullptr) : model(m) {}
    void setModel(ListBoxModel* m) { model = m; } void setRowHeight(int h) { rowH = h; } void updateContent() {} void selectRow(int r, bool = false, bool = true) { selRow = r; } void setColour(int, Colour) {}
    void setOutlineThickness(int) {} int getSelectedRow() const { return selRow; }
    ListBoxModel* model = nullptr; int rowH = 30, selRow = 0; void scrollToEnsureRowIsOnscreen(int) {}
};
class PopupMenu {
public:
    enum ColourIds { backgroundColourId = 0x1000700, textColourId, headerTextColourId, highlightedBackgroundColourId, highlightedTextColourId };
    class Options { public: Options withTargetComponent(Component&) const { return *this; } Options withTargetComponent(Component*) const { return *this; } };
    void addItem(int, const String&) {} void addSeparator() {} void addItem(const String&, std::function<void()>) {}
    void showMenuAsync(const Options&, std::function<void(int)>) {}
};
class TooltipWindow : public Component { public: enum ColourIds { backgroundColourId = 0x1001b00, textColourId, outlineColourId }; TooltipWindow(Component* = nullptr, int = 700) {} };
class MessageBoxIconType { public: static constexpr int NoIcon = 0; };
class ModalCallbackFunction { public: static std::function<void(int)> create(std::function<void(int)> f) { return f; } };
class AlertWindow : public Component {
public:
    AlertWindow(const String&, const String&, int, Component* = nullptr) {}
    void addTextEditor(const String&, const String&) {} void addButton(const String&, int, const KeyPress& = {}) {}
    void enterModalState(bool, std::function<void(int)>, bool) {}
    String getTextEditorContents(const String&) const { return {}; }
};
struct Decibels { static float gainToDecibels(float g, float minusInf = -100.f) { return g > 0.f ? std::max(minusInf, 20.f * std::log10(g)) : minusInf; } };
struct Random { static Random& getSystemRandom() { static Random r; return r; } int nextInt(int n) { return std::rand() % std::max(1, n); } };

class LookAndFeel_V4 : public LookAndFeel {
public:
    virtual void drawLinearSlider(Graphics&, int, int, int, int, float, float, float, Slider::SliderStyle, Slider&) {}
    virtual void drawRotarySlider(Graphics&, int, int, int, int, float, float, float, Slider&) {}
    virtual void drawToggleButton(Graphics&, ToggleButton&, bool, bool) {}
    virtual void drawCornerResizer(Graphics&, int, int, bool, bool) {}
    virtual Slider::SliderLayout getSliderLayout(Slider&) { return {}; }
    virtual Label* createSliderTextBox(Slider&) { return new Label(); }
};

// ---------------------------------------------------------------- files / xml / values
class var { public: var() = default; var(const String& s_) : s(s_) {} var(const char* c) : s(c) {} var(int v) : s(v) {} var(float v) : s(v) {} var(double v) : s(v) {} String toString() const { return s; } operator double() const { return s.getDoubleValue(); } private: String s; };
using Identifier = String;
class XmlElement { public: String toString() const { return {}; } bool hasTagName(const String&) const { return true; } };
template <class T> class Array : public std::vector<T> { public: bool isEmpty() const { return this->empty(); } };
class Result { public: bool wasOk() const { return true; } };
class File {
public:
    enum SpecialLocationType { userApplicationDataDirectory };
    enum TypesOfFileToFind { findFiles = 2 };
    File() = default;
    static File getSpecialLocation(SpecialLocationType) { return {}; }
    static String createLegalFileName(const String& s) { return s; }
    File getChildFile(const String&) const { return {}; }
    bool isDirectory() const { return false; }
    Result createDirectory() const { return {}; }
    Array<File> findChildFiles(int, bool, const String&) const { return {}; }
    String getFileName() const { return {}; }
    String getFileNameWithoutExtension() const { return {}; }
    bool replaceWithText(const String&) const { return true; }
};
struct XmlDocument { static std::unique_ptr<XmlElement> parse(const File&) { return nullptr; } };
class ValueTree {
public:
    ValueTree() = default;
    ValueTree(const char*) {}
    Identifier getType() const { return {}; }
    bool isValid() const { return valid; }
    ValueTree createCopy() const { return *this; }
    std::unique_ptr<XmlElement> createXml() const { return std::make_unique<XmlElement>(); }
    static ValueTree fromXml(const XmlElement&) { return {}; }
    var getProperty(const Identifier&, const var& d = {}) const { return d; }
    var operator[](const Identifier&) const { return {}; }
    void setProperty(const Identifier&, const var&, void*) {}
    void addChild(const ValueTree&, int, void*) {} void removeChild(const ValueTree&, void*) {}
    ValueTree getChildWithName(const Identifier&) const { return {}; }
    bool valid = false;
};
class MemoryBlock {};

// ---------------------------------------------------------------- parameters
class RangedAudioParameter {
public:
    virtual ~RangedAudioParameter() = default;
    void beginChangeGesture() {} void endChangeGesture() {}
    void setValueNotifyingHost(float n) { value = convertFrom0to1(n); }
    float getDefaultValue() const { return (def - lo) / (hi - lo); }
    float convertTo0to1(float v) const { return logSkew ? std::log(std::max(v, lo) / lo) / std::log(hi / lo) : (v - lo) / (hi - lo); }
    float convertFrom0to1(float n) const { return logSkew ? lo * std::pow(hi / lo, n) : lo + n * (hi - lo); }
    float getValue() const { return convertTo0to1(value); }
    String id; std::atomic<float> value {0.f}; float lo = 0.f, hi = 1.f, def = 0.f; bool logSkew = false;
};
struct ParameterID { ParameterID(const String& s_, int) : s(s_) {} ParameterID(const char* c, int) : s(c) {} String s; };
template <class T> class NormalisableRange { public: using Fn = std::function<T(T, T, T)>; NormalisableRange(T a, T b) : lo(a), hi(b) {} NormalisableRange(T a, T b, T) : lo(a), hi(b) {} NormalisableRange(T a, T b, Fn, Fn) : lo(a), hi(b), skewed(true) {} void setSkewForCentre(T) {} T lo, hi; bool skewed = false; };
class AudioParameterFloat : public RangedAudioParameter { public: AudioParameterFloat(const ParameterID& i, const String&, NormalisableRange<float> r, float d) { id = i.s; lo = r.lo; hi = r.hi; def = d; value = d; logSkew = r.skewed; } };
class AudioParameterBool : public RangedAudioParameter { public: AudioParameterBool(const ParameterID& i, const String&, bool d) { id = i.s; lo = 0; hi = 1; def = d ? 1.f : 0.f; value = def; } };
class StringArray : public std::vector<String> { public: StringArray() = default; StringArray(std::initializer_list<const char*> l) { for (auto* c : l) push_back(String(c)); } };
class AudioParameterChoice : public RangedAudioParameter { public: AudioParameterChoice(const ParameterID& i, const String&, const StringArray& c, int d) { id = i.s; lo = 0; hi = static_cast<float>(c.size() - 1); def = static_cast<float>(d); value = def; } };
class AudioProcessor;
class UndoManager {};
class AudioProcessorValueTreeState {
public:
    struct ParameterLayout { void add(std::unique_ptr<RangedAudioParameter> p) { items.push_back(std::move(p)); } std::vector<std::unique_ptr<RangedAudioParameter>> items; };
    AudioProcessorValueTreeState(AudioProcessor&, UndoManager*, const Identifier&, ParameterLayout&& l) { for (auto& p : l.items) { auto key = p->id.toStdString(); params[key] = std::move(p); } }
    std::atomic<float>* getRawParameterValue(const String& id) const { return &params.at(id.toStdString())->value; }
    RangedAudioParameter* getParameter(const String& id) const { return params.at(id.toStdString()).get(); }
    ValueTree copyState() { return state; }
    void replaceState(const ValueTree&) {}
    ValueTree state;
    std::map<std::string, std::unique_ptr<RangedAudioParameter>> params;
    class SliderAttachment { public: SliderAttachment(AudioProcessorValueTreeState& a, const String& id, Slider& sl) { auto* p = a.getParameter(id); sl.minV = p->lo; sl.maxV = p->hi; sl.value = p->value; sl.prop = p->getValue(); } };
    class ComboBoxAttachment { public: ComboBoxAttachment(AudioProcessorValueTreeState& a, const String& id, ComboBox& b) { b.selId = static_cast<int>(a.getParameter(id)->value) + 1; for (auto& it : b.items) if (it.second == b.selId) b.text = it.first; } };
    class ButtonAttachment { public: ButtonAttachment(AudioProcessorValueTreeState& a, const String& id, Button& b) { b.on = a.getParameter(id)->value > .5f; } };
};

// ---------------------------------------------------------------- audio / processor / editor
template <class T> class AudioBuffer {
public:
    int getNumSamples() const { return n; } int getNumChannels() const { return ch; }
    T* getWritePointer(int) { return nullptr; } const T* getReadPointer(int) const { return nullptr; }
    void copyFrom(int, int, const AudioBuffer&, int, int, int) {} void clear() {} void clear(int, int, int) {}
    int n = 0, ch = 2;
};
class MidiBuffer {};
struct ScopedNoDenormals { ScopedNoDenormals() {} ~ScopedNoDenormals() {} };
struct ValueSmoothingTypes { struct Linear {}; struct Multiplicative {}; };
template <class T, class Kind = ValueSmoothingTypes::Linear> class SmoothedValue {
public:
    void reset(double, double) {} void setCurrentAndTargetValue(T v) { cur = v; } void setTargetValue(T v) { cur = v; } T getNextValue() { return cur; }
    T getCurrentValue() const { return cur; } void skip(int) {}
private:
    T cur {};
};
class AudioChannelSet { public: static AudioChannelSet stereo() { return AudioChannelSet(2); } static AudioChannelSet mono() { return AudioChannelSet(1); } int size() const { return n; } bool operator==(const AudioChannelSet& o) const { return n == o.n; } AudioChannelSet() : n(2) {} explicit AudioChannelSet(int n_) : n(n_) {} private: int n; };
class AudioProcessor {
public:
    class BusesProperties { public: BusesProperties withInput(const String&, AudioChannelSet, bool) const { return *this; } BusesProperties withOutput(const String&, AudioChannelSet, bool) const { return *this; } };
    struct BusesLayout { AudioChannelSet getMainInputChannelSet() const { return {}; } AudioChannelSet getMainOutputChannelSet() const { return {}; } };
    AudioProcessor() = default;
    explicit AudioProcessor(const BusesProperties&) {}
    virtual ~AudioProcessor() = default;
    virtual void prepareToPlay(double, int) = 0;
    virtual void releaseResources() = 0;
    virtual bool isBusesLayoutSupported(const BusesLayout&) const { return true; }
    virtual void processBlock(AudioBuffer<float>&, MidiBuffer&) = 0;
    virtual class AudioProcessorEditor* createEditor() = 0;
    virtual bool hasEditor() const = 0;
    virtual const String getName() const = 0;
    virtual bool acceptsMidi() const = 0; virtual bool producesMidi() const = 0; virtual bool isMidiEffect() const = 0;
    virtual double getTailLengthSeconds() const = 0;
    virtual int getNumPrograms() = 0; virtual int getCurrentProgram() = 0; virtual void setCurrentProgram(int) = 0;
    virtual const String getProgramName(int) = 0; virtual void changeProgramName(int, const String&) = 0;
    virtual void getStateInformation(MemoryBlock&) = 0; virtual void setStateInformation(const void*, int) = 0;
    void setLatencySamples(int) {}
    int getTotalNumInputChannels() const { return 2; } int getTotalNumOutputChannels() const { return 2; }
    double getSampleRate() const { return 48000.0; }
    struct PositionInfo { bool getIsPlaying() const { return false; } std::optional<double> getBpm() const { return std::nullopt; } std::optional<double> getPpqPosition() const { return std::nullopt; } };
    struct PlayHead { std::optional<PositionInfo> getPosition() const { return std::nullopt; } };
    PlayHead* getPlayHead() const { return nullptr; }
protected:
    static void copyXmlToBinary(const XmlElement&, MemoryBlock&) {}
    static std::unique_ptr<XmlElement> getXmlFromBinary(const void*, int) { return nullptr; }
};
class ComponentBoundsConstrainer { public: void setFixedAspectRatio(double) {} };
class AudioProcessorEditor : public Component {
public:
    explicit AudioProcessorEditor(AudioProcessor&) {}
    explicit AudioProcessorEditor(AudioProcessor*) {}
    void setResizable(bool, bool) {} void setResizeLimits(int, int, int, int) {}
    ComponentBoundsConstrainer* getConstrainer() { return &constrainer; }
private:
    ComponentBoundsConstrainer constrainer;
};
}  // namespace juce
#define JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(x)
#define JUCE_CALLTYPE
