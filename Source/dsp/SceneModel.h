#pragma once
// Pure C++17 scene metadata and acoustic math, also used by headless tests.
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace afar {
constexpr float speedOfSound = 343.0f;
constexpr float pi = 3.14159265358979323846f;

enum class Category { Underground, Outdoor, Indoor };
enum class AmbienceKind { Wind, Water, Traffic, Room, Forest, Insects, Drone };
struct SpaceSpec {
    const char* name;
    const char* description;
    Category category;
    float widthMeters, lengthMeters, heightMeters;
    float rt60Seconds, reflectionGain, diffuseGain, upperRollOffHz;
    float firstEchoMeters, scatter;
    AmbienceKind ambience;
    bool isOpen;
};
// Physically inspired, simplified reference geometries. User-editable JSON is
// a forthcoming feature; factory definitions are deliberately deterministic.
inline constexpr std::array<SpaceSpec, 20> spaces {{
    {"Tunnel", "Long concrete flutter", Category::Underground, 8, 150, 5, 3.2f, .38f, .38f, 5400, 8, .14f, AmbienceKind::Traffic, false},
    {"Sewer", "Wet pipe and drips", Category::Underground, 2, 55, 2, 2.1f, .30f, .34f, 4400, 4, .18f, AmbienceKind::Water, false},
    {"Subway Platform", "Tile, rails and rumble", Category::Underground, 12, 140, 6, 3.0f, .26f, .40f, 7200, 18, .28f, AmbienceKind::Traffic, false},
    {"Cave", "Irregular stone chamber", Category::Underground, 24, 55, 14, 5.1f, .28f, .46f, 3700, 12, .80f, AmbienceKind::Water, false},
    {"Cellar / Bunker", "Close and low-pitched", Category::Underground, 5, 8, 2.5f, .55f, .13f, .14f, 3200, 4, .15f, AmbienceKind::Drone, false},
    {"Graveyard", "Sparse stones, night air", Category::Outdoor, 55, 110, 100, .30f, .16f, .025f, 9500, 22, .20f, AmbienceKind::Insects, true},
    {"Forest", "Foliage and scattering", Category::Outdoor, 110, 150, 100, .65f, .15f, .055f, 3800, 35, .92f, AmbienceKind::Forest, true},
    {"Canyon / Valley", "Long, delayed walls", Category::Outdoor, 210, 900, 150, .60f, .40f, .030f, 8300, 95, .15f, AmbienceKind::Wind, true},
    {"Open Field", "Dry air and wind", Category::Outdoor, 300, 300, 100, .15f, .030f, .005f, 9600, 160, .06f, AmbienceKind::Wind, true},
    {"City Alley", "Brick-wall flutter", Category::Outdoor, 5, 60, 26, 1.3f, .34f, .22f, 7000, 5, .15f, AmbienceKind::Traffic, true},
    {"Parking Garage", "Long concrete wash", Category::Indoor, 45, 90, 3, 3.3f, .25f, .42f, 6100, 18, .34f, AmbienceKind::Room, false},
    {"Stairwell", "Vertical pitched decay", Category::Indoor, 4, 7, 24, 3.9f, .35f, .43f, 7600, 6, .12f, AmbienceKind::Room, false},
    {"Cathedral", "Massive stone enclosure", Category::Indoor, 28, 70, 32, 6.8f, .22f, .62f, 4900, 22, .35f, AmbienceKind::Room, false},
    {"Empty Pool", "Bright tile slapback", Category::Indoor, 17, 28, 6, 2.5f, .43f, .39f, 9600, 8, .20f, AmbienceKind::Room, false},
    {"Warehouse", "Boxy, sparse slaps", Category::Indoor, 35, 90, 12, 2.7f, .28f, .34f, 8000, 30, .28f, AmbienceKind::Room, false},
    {"Bathroom Tile", "Small, bright and ringing", Category::Indoor, 2.5f, 3, 2.5f, 1.1f, .36f, .30f, 9800, 2, .10f, AmbienceKind::Water, false},
    {"Crypt", "Narrow, cold stone", Category::Underground, 3, 30, 3, 4.2f, .30f, .40f, 3000, 10, .35f, AmbienceKind::Drone, false},
    {"Elevator Shaft", "Tall metal echo tube", Category::Indoor, 2, 2, 40, 2.4f, .45f, .35f, 6800, 20, .08f, AmbienceKind::Drone, false},
    {"Snowfield", "Hushed, dead and far", Category::Outdoor, 400, 400, 100, .10f, .02f, .004f, 6200, 180, .05f, AmbienceKind::Wind, true},
    {"Arena", "Huge roar and slap", Category::Indoor, 100, 150, 40, 4.8f, .22f, .52f, 5600, 45, .30f, AmbienceKind::Traffic, false}
}};
// Acoustic personality per space (same order as `spaces`). Fields: flutter delay ms / feedback,
// two resonance centres Hz and their gain dB, low-tilt and high-tilt amounts (+boost / -cut),
// late pre-delay ms, late shimmer depth, far-wall slap delay ms / gain.
struct SpaceCharacter {
    float flutterMs, flutterFb, resonHz1, resonHz2, resonDb, lowDb, highDb, preMs, mod, slapMs, slapGain;
};
inline constexpr std::array<SpaceCharacter, 20> characters {{
    {47, .52f,   0,   0, 0,  .35f,  .10f, 12, 0,    0,   0},      // Tunnel: strong periodic flutter
    {13, .38f, 210, 520, 9,  .40f, -.40f,  4, 0,    0,   0},      // Sewer: pipe resonance
    {72, .30f,   0,   0, 0,  .20f,  .35f, 20, 0,  165, .25f},     // Subway: tile + platform far wall
    { 0,   0,    0,   0, 0,  .50f, -.50f, 28, .80f,  0,   0},     // Cave: dark, swirling
    { 0,   0,  240, 480, 7,  .60f, -.35f,  2, 0,    0,   0},      // Cellar: boxy mid resonance
    { 0,   0,    0,   0, 0, -.20f,  .15f, 30, 0,  230, .38f},     // Graveyard: thin air, stone slap
    { 0,   0,    0,   0, 0, -.10f, -.30f, 15, 1.0f,  0,   0},     // Forest: scattering shimmer
    { 0,   0,    0,   0, 0,  .20f,  0.f,  40, 0,  880, .42f},     // Canyon: long answering echo
    { 0,   0,    0,   0, 0,  0.f,   .10f,  0, 0,  520, .20f},     // Field: distant tree line
    {36, .55f,   0,   0, 0,  .15f,  .25f,  6, 0,   85, .30f},     // Alley: brick slap flutter
    {56, .36f,   0,   0, 0,  .45f,  .15f, 20, 0,    0,   0},      // Garage: concrete wash
    {21, .50f, 330, 660, 6,  0.f,   .30f,  8, .50f,  0,   0},     // Stairwell: pitched spiral
    { 0,   0,    0,   0, 0,  .35f, -.45f, 45, .35f,  0,   0},     // Cathedral: slow dark bloom
    {28, .60f,   0,   0, 0, -.20f,  .55f,  5, 0,    0,   0},      // Pool: bright tile flutter
    { 0,   0,    0,   0, 0,  .50f,  .05f, 25, 0,  130, .30f},     // Warehouse: boomy slaps
    {12, .55f, 380, 760, 6,  0.f,   .50f,  2, 0,    0,   0},      // Bathroom: ringing tile
    { 0,   0,    0,   0, 0,  .50f, -.60f, 18, .30f, 120, .30f},   // Crypt: cold, dark
    {233,.45f, 920,1840, 8,  0.f,   .40f,  4, 0,    0,   0},      // Elevator: vertical metal echo
    { 0,   0,    0,   0, 0, -.30f, -.55f,  0, 0,  700, .18f},     // Snowfield: hushed and dead
    { 0,   0,    0,   0, 0,  .30f, -.20f, 60, .25f, 620, .45f}    // Arena: huge slap and roar
}};
inline const SpaceCharacter& getCharacter(int index) noexcept {
    return characters[static_cast<std::size_t>(std::clamp(index, 0, static_cast<int>(characters.size()) - 1))];
}
inline const SpaceSpec& getSpace(int index) noexcept {
    return spaces[static_cast<std::size_t>(std::clamp(index, 0, static_cast<int>(spaces.size()) - 1))];
}
inline float distanceMeters(float normalized) noexcept {
    // 0..1 -> 0.05 .. 200 m, logarithmic scale.
    return .05f * std::pow(4000.0f, std::clamp(normalized, 0.0f, 1.0f));
}
inline float distanceNormalized(float meters) noexcept {
    return std::clamp(std::log(std::clamp(meters, .05f, 200.0f) / .05f) / std::log(4000.0f), 0.0f, 1.0f);
}
inline float arrivalMilliseconds(float meters) noexcept { return std::max(0.f, meters) / speedOfSound * 1000.f; }
inline float geometricDirectGain(float distance) noexcept {
    return 1.f / std::max(.30f, distance); // close-range safety cap
}
inline float approximateCriticalDistance(const SpaceSpec& s) noexcept {
    const float volume = s.widthMeters * s.lengthMeters * s.heightMeters;
    return .057f * std::sqrt(volume / std::max(.1f, s.rt60Seconds));
}
inline float directReverbDb(const SpaceSpec& s, float distance) noexcept {
    if (s.isOpen && s.diffuseGain < .06f) return 40.0f - 20.0f * std::log10(std::max(.3f, distance));
    return 20.f * std::log10(approximateCriticalDistance(s) / std::max(.3f, distance));
}
inline float echoDelayMs(float oneWayMeters) noexcept {
    return 2000.f * std::max(0.f, oneWayMeters) / speedOfSound;
}
inline float effortSpl(float position) noexcept {
    // Piecewise-interpolated illustrative SPL at 1 m: no input calibration.
    const float p = std::clamp(position, 0.f, 1.f);
    if (p < .25f) return 40.f + p * 60.f;
    if (p < .5f) return 55.f + (p-.25f) * 40.f;
    if (p < .75f) return 65.f + (p-.5f) * 60.f;
    return 80.f + (p-.75f) * 80.f;
}
inline float recordedEffort(int id) noexcept {
    constexpr std::array<float, 5> k {0.f, .25f, .5f, .75f, .5f};
    return k[static_cast<std::size_t>(std::clamp(id,0,4))];
}
} // namespace afar
