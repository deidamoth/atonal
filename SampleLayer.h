#pragma once
// Parallel one-shot player. Does not touch the vocoder or granular DSP.
// The recording is an F, mapped to F3 (MIDI 65). Playback is one-shot.

#include "ChairSqueakAdpcm.h"
#include "QuarterTonePitch.h"
#include <cmath>
#include <cstdint>

class SampleLayer
{
  public:
    static constexpr int kVoices = 4;
    static constexpr int kRootNote = QuarterTonePitch::kRootNote;

    void Init(float sampleRate, int16_t* pcm)
    {
        sr_  = sampleRate;
        pcm_ = pcm;
        Decode();
        for(int i = 0; i < kVoices; i++)
            voice_[i] = Voice{};
    }

    void NoteOn(uint8_t note)
    {
        int slot = -1;
        for(int i = 0; i < kVoices; i++)
        {
            if(!voice_[i].on)
            {
                slot = i;
                break;
            }
        }
        if(slot < 0)
        {
            slot          = 0;
            uint32_t age  = voice_[0].age;
            for(int i = 1; i < kVoices; i++)
            {
                if(voice_[i].age < age)
                {
                    age  = voice_[i].age;
                    slot = i;
                }
            }
        }
        const float pitch = QuarterTonePitch::Ratio(note);
        voice_[slot].on   = true;
        voice_[slot].pos  = 0.f;
        voice_[slot].inc  = (float(kSqueakRate) / sr_) * pitch;
        voice_[slot].age  = ++clock_;
        voice_[slot].note = note;
    }

    // Advances voices even when the caller discards the result.
    float Process()
    {
        float sum = 0.f;
        for(int i = 0; i < kVoices; i++)
        {
            Voice& v = voice_[i];
            if(!v.on)
                continue;
            const int i0 = (int)v.pos;
            if(i0 >= kSqueakFrames - 1)
            {
                v.on = false;
                continue;
            }
            const float f = v.pos - float(i0);
            const float a = pcm_[i0] * (1.f / 32768.f);
            const float b = pcm_[i0 + 1] * (1.f / 32768.f);
            sum += (a + (b - a) * f) * 0.28f;
            v.pos += v.inc;
        }
        return sum;
    }

  private:
    struct Voice
    {
        bool     on   = false;
        float    pos  = 0.f;
        float    inc  = 1.f;
        uint8_t  note = 0;
        uint32_t age  = 0;
    };

    void Decode()
    {
        static const int step_table[89] = {
            7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,
            107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,
            876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,
            4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,
            22385,24623,27086,29794,32767};
        static const int index_table[16] = {-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};

        int pred  = kSqueakPredictor;
        int index = 0;
        pcm_[0]   = (int16_t)pred;
        int frame = 1;
        for(size_t i = 0; i < kSqueakAdpcmBytes && frame < kSqueakFrames; i++)
        {
            const uint8_t byte = kSqueakAdpcm[i];
            for(int n = 0; n < 2 && frame < kSqueakFrames; n++)
            {
                const int nibble = (n == 0) ? (byte >> 4) : (byte & 0x0f);
                const int step   = step_table[index];
                int       delta  = step >> 3;
                if(nibble & 4)
                    delta += step;
                if(nibble & 2)
                    delta += step >> 1;
                if(nibble & 1)
                    delta += step >> 2;
                if(nibble & 8)
                    pred -= delta;
                else
                    pred += delta;
                if(pred > 32767)
                    pred = 32767;
                if(pred < -32768)
                    pred = -32768;
                index += index_table[nibble & 15];
                if(index < 0)
                    index = 0;
                if(index > 88)
                    index = 88;
                pcm_[frame++] = (int16_t)pred;
            }
        }
    }

    float    sr_    = 48000.f;
    int16_t* pcm_   = nullptr;
    uint32_t clock_ = 0;
    Voice    voice_[kVoices];
};
