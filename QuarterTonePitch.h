#pragma once
#include <cmath>
#include <cstdint>

namespace QuarterTonePitch
{
constexpr int kRootNote = 65;
// Normal 12-TET MIDI 65 frequency, using A4 = 440 Hz.
constexpr float kRootHz = 349.228231433f;

// F G A B C D E white keys -> F G A-half-flat Bb C D E-half-flat.
// Black keys sit between white-key pitches, rounded up to the 24-TET grid.
inline int Steps(uint8_t note)
{
    constexpr int steps[12] = {0, 2, 4, 6, 7, 9, 10, 14, 16, 18, 20, 21};
    const int relative = int(note) - kRootNote;
    int octave = relative / 12;
    int key = relative % 12;
    if(key < 0)
    {
        key += 12;
        --octave;
    }
    return octave * 24 + steps[key];
}

inline float Ratio(uint8_t note)
{
    return powf(2.f, float(Steps(note)) / 24.f);
}

inline float Frequency(uint8_t note)
{
    return kRootHz * Ratio(note);
}
} // namespace QuarterTonePitch
