#pragma once
#include <cmath>
#include <cstdint>

namespace QuarterTonePitch
{
constexpr int kRootNote = 65;
// Normal 12-TET MIDI 65 frequency, using A4 = 440 Hz.
constexpr float kRootHz = 349.228231433f;

inline float Ratio(uint8_t note)
{
    return powf(2.f, (float(note) - float(kRootNote)) / 24.f);
}

inline float Frequency(uint8_t note)
{
    return kRootHz * Ratio(note);
}
} // namespace QuarterTonePitch
