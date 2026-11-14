#pragma once
// Mock of the JUCE-generated BinaryData header. Each resource's "bytes" are its asset name, so the
// mock Image remembers which PNG it stands for and the preview renderer can draw the real file.
#include <map>
#include <string>
namespace BinaryData {
inline const char bg_panel_png[] = "bg_panel"; inline constexpr int bg_panel_pngSize = 9;
inline const char logo_skull_png[] = "logo_skull"; inline constexpr int logo_skull_pngSize = 11;
inline const char* getNamedResource(const char* name, int& size) {
    static std::map<std::string, std::string> names;
    std::string n(name); const auto cut = n.rfind("_png");
    auto& v = names[n]; if (v.empty()) v = n.substr(0, cut == std::string::npos ? n.size() : cut);
    size = static_cast<int>(v.size()) + 1; return v.c_str();
}
}
