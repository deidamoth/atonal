#include "../QuarterTonePitch.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main()
{
    using namespace QuarterTonePitch;
    assert(Ratio(65) == 1.f);
    assert(std::fabs(Frequency(65) - 349.228231433) < 0.0001);
    assert(Ratio(77) == 2.f);
    assert(Ratio(89) == 4.f);
    assert(Ratio(53) == 0.5f);
    const int white[] = {65, 67, 69, 71, 72, 74, 76, 77};
    const int cents[] = {0, 200, 350, 500, 700, 900, 1050, 1200};
    for(int i = 0; i < 8; ++i)
        assert(Steps(white[i]) * 50 == cents[i]);
    const int black[] = {66, 68, 70, 73, 75};
    const int black_cents[] = {100, 300, 450, 800, 1000};
    for(int i = 0; i < 5; ++i)
        assert(Steps(black[i]) * 50 == black_cents[i]);
    for(int note = 0; note < 128; ++note)
    {
        assert(std::isfinite(Frequency(note)) && Frequency(note) > 0.f);
        assert(std::fabs(Frequency(note) / Ratio(note) - kRootHz) < 0.0001f);
        if(note < 127) assert(Ratio(note + 1) > Ratio(note));
        if(note < 116)
        {
            assert(Steps(note + 12) - Steps(note) == 24);
            assert(std::fabs(Ratio(note + 12) / Ratio(note) - 2.f) < 0.000001f);
        }
    }
    std::puts("PASS: neutral scale, black keys, root, and octave mapping across all 128 MIDI notes");
}
