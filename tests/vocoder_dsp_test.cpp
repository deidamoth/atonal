#include <initializer_list>

#include "../VocoderDsp.h"
#include <cassert>
#include <cstdio>

int main()
{
    constexpr float sr = 48000.f, pi = 3.14159265359f;
    // Measure channel selectivity independently of the final output limiter.
    ChannelFilter center, far;
    center.Init(sr, 1000); far.Init(sr, 3000);
    double wanted = 0, rejected = 0;
    for(int i = 0; i < 48000; ++i)
    {
        float x = std::sin(2*pi*1000*i/sr);
        float a = center.Process(x), b = far.Process(x);
        if(i > 24000) { wanted += a*a; rejected += b*b; }
    }
    assert(wanted / rejected > 1000);
    VocoderDsp dsp; dsp.Init(sr);
    double energy = 0;
    float peak = 0;
    for(int i = 0; i < 144000; ++i)
    {
        // Four harmonic-rich notes with a two-formant synthetic modulator.
        float carrier = 0;
        for(float f : {130.81f, 164.81f, 196.f, 261.63f})
            carrier += 0.18f * (2.f * std::fmod(i*f/sr, 1.f) - 1.f);
        float input = i < 48000 ? 0.f : i < 96000 ?
            0.04f * (std::sin(2*pi*700*i/sr) + std::sin(2*pi*2200*i/sr)) : 0.f;
        float y = dsp.Process(input, carrier, 0, 0.55f);
        assert(std::isfinite(y) && std::fabs(y) <= 0.9f);
        if(i < 48000 || i > 140000) assert(std::fabs(y) < 1e-5f);
        if(i > 72000 && i < 96000) energy += y*y;
        peak = std::fmax(peak, std::fabs(y));
    }
    assert(energy > 0.01);
    for(int i = 0; i < 48000; ++i)
    {
        float y = dsp.Process(i % 2 ? 100.f : -100.f, 1.f, 0.55f, 0.55f);
        assert(std::isfinite(y) && std::fabs(y) <= 0.900001f);
    }
    std::printf("PASS: channel rejection %.1f dB; wet RMS %.4f, peak %.4f; silence, release, overload bounded\n",
                10*std::log10(wanted/rejected), std::sqrt(energy/24000), peak);
}
