#pragma once
#include <array>
struct FactoryPreset
{
    const char *name;
    float threshold, ceiling, release;
    bool arc;
    int bits = 0, dither = 0, shaping = 0;
};
// Original starting points: source level determines the actual gain reduction.
inline constexpr std::array<FactoryPreset, 33> factoryPresets{
    {{"Default", 0, -.3f, 100, true},
     {"Female Vocal - Natural", -3, -.8f, 100, true},
     {"Female Vocal - Pop Forward", -5, -.8f, 80, true},
     {"Female Vocal - Soft Ballad", -2, -1, 180, false},
     {"Male Vocal - Natural", -3, -.8f, 150, true},
     {"Male Vocal - Deep & Warm", -4, -1, 220, false},
     {"Male Vocal - Pop Forward", -5, -.8f, 100, true},
     {"Backing Vocals - Smooth", -3.5f, -1, 250, true},
     {"Bass - Electric Round", -4, -1, 250, false},
     {"Bass - Pick Attack", -3, -.8f, 150, true},
     {"Bass - Synth & Sub", -3, -1.2f, 350, false},
     {"Acoustic - Gentle Dynamics", -2, -1, 200, true},
     {"Acoustic Guitar - Fingerstyle", -2.5f, -.8f, 180, true},
     {"Acoustic Guitar - Strumming", -4, -1, 120, true},
     {"Nylon Guitar - Warm", -2, -1, 220, false},
     {"Piano - Natural", -2.5f, -1, 250, true},
     {"Piano - Pop Presence", -4, -.8f, 180, true},
     {"Electric Piano - Smooth", -3, -1, 200, false},
     {"Electric Guitar - Clean", -3, -.8f, 100, true},
     {"Drums - Transient Control", -3, -1, 60, true},
     {"Strings - Gentle", -2, -1, 400, false},
     {"Pads - Sustained", -2.5f, -1, 500, false},
     {"Podcast - Voice", -4, -1, 150, true},
     {"Mix Bus - Gentle", -1.5f, -1, 250, true},
     {"CD Master - 16 Bit", -2, -.5f, 150, true, 4, 1, 2},
     {"Master Bus - Transparent", -1, -1, 250, true},
     {"Master Bus - Streaming", -2, -1, 200, true},
     {"Master Bus - Pop Punch", -3.5f, -.8f, 120, true},
     {"Master Bus - Rock Glue", -3, -1, 180, true},
     {"Master Bus - Electronic", -4, -1, 100, true},
     {"Master Bus - Acoustic", -1.5f, -1, 300, true},
     {"Master Bus - Jazz & Classical", -.5f, -1, 400, false},
     {"Master Bus - Gentle Safety", -.3f, -1, 350, true}}};
