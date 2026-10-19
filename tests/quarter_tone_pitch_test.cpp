#include "../QuarterTonePitch.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main()
{
    using namespace QuarterTonePitch;
    assert(Ratio(65) == 1.f);
    assert(std::fabs(Frequency(65) - 349.228231433) < 0.0001);
    assert(std::fabs(Ratio(77) - std::sqrt(2.f)) < 0.000001f);
    assert(Ratio(89) == 2.f);
    assert(Ratio(41) == 0.5f);
    for(int note = 0; note < 128; ++note)
    {
        const double expected = std::exp2((note - 65) / 24.0);
        assert(std::fabs(Ratio(note) / expected - 1) < 0.000001);
        assert(std::fabs(Frequency(note) / (349.228231433 * expected) - 1) < 0.000001);
        if(note < 127)
            assert(std::fabs(Ratio(note + 1) / Ratio(note) - std::exp2(1.0 / 24)) < 0.000001);
    }
    std::puts("Quarter-tone pitch: all 128 MIDI notes passed");
}
