#pragma once
#include <cmath>

// Unity-peak bandpass, transposed direct form II. Two sections provide
// useful channel separation without the nonlinear resonance of the old SVFs.
struct ChannelFilter
{
    float b0 = 0, a1 = 0, a2 = 0;
    float z1[2] = {}, z2[2] = {};
    void Init(float sr, float hz)
    {
        const float w = 6.28318530718f * hz / sr;
        const float alpha = std::sin(w) / (2.f * 3.5f);
        b0 = alpha / (1.f + alpha);
        a1 = -2.f * std::cos(w) / (1.f + alpha);
        a2 = (1.f - alpha) / (1.f + alpha);
        for(int s = 0; s < 2; ++s) z1[s] = z2[s] = 0;
    }
    float Process(float x)
    {
        for(int s = 0; s < 2; ++s)
        {
            const float y = b0 * x + z1[s];
            z1[s] = -a1 * y + z2[s];
            z2[s] = -b0 * x - a2 * y;
            x = y;
        }
        return x;
    }
};

struct VocoderDsp
{
    static constexpr int bands = 20;
    ChannelFilter analysis[bands], synthesis[bands];
    float envelope[bands] = {}, attack[bands] = {};
    float release = 0, smooth = 0, dcPole = 0;
    float previousInput = 0, highpass = 0, previousCarrier = 0;
    float speech = 0, gate = 0, dry = 0, wet = 0;

    void Init(float sr)
    {
        release = 1.f - std::exp(-1.f / (0.035f * sr));
        smooth = 1.f - std::exp(-1.f / (0.010f * sr));
        dcPole = std::exp(-6.28318530718f * 70.f / sr);
        for(int b = 0; b < bands; ++b)
        {
            const float hz = 120.f * std::pow(7500.f / 120.f, b / float(bands - 1));
            analysis[b].Init(sr, hz);
            synthesis[b].Init(sr, hz);
            // Slower bass detection suppresses pitch ripple; upper bands track consonants.
            const float seconds = std::fmax(0.002f, 1.f / hz);
            attack[b] = 1.f - std::exp(-1.f / (seconds * sr));
        }
    }
    float Process(float input, float carrier, float dryTarget, float wetTarget)
    {
        highpass = input - previousInput + dcPole * highpass;
        previousInput = input;
        // Keep analysis linear: clipping here creates false formants.
        const float mic = highpass * 8.f;
        const float level = std::fabs(mic);
        speech += (level > speech ? attack[bands - 1] : release) * (level - speech);
        const float open = std::fmin(1.f, std::fmax(0.f, (speech - 0.004f) / 0.016f));
        gate += smooth * (open * open * (3.f - 2.f * open) - gate);
        // Flatten the saw's harmonic rolloff so F2/F3 aren't masked by its bass.
        const float bright = (carrier - 0.96f * previousCarrier) * 4.f;
        previousCarrier = carrier;
        float sum = 0;
        for(int b = 0; b < bands; ++b)
        {
            const float magnitude = std::fabs(analysis[b].Process(mic));
            envelope[b] += (magnitude > envelope[b] ? attack[b] : release)
                           * (magnitude - envelope[b]);
            sum += synthesis[b].Process(bright) * envelope[b];
        }
        dry += smooth * (dryTarget - dry);
        wet += smooth * (wetTarget - wet);
        const float mixed = mic * dry + sum * 16.f * gate * wet;
        // Unity small-signal gain, bounded to +/-0.9 even with dry+wet at maximum.
        return 0.9f * mixed / (0.9f + std::fabs(mixed));
    }
};
