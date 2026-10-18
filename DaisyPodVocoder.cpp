// Daisy Pod vocoder
// Mic (line in L) = modulator (voice formants)
// MIDI keyboard = carrier (saw)
// Button 1: toggle vocoder on/off
// Knob 1: dry (CCW) → wet/vocoded (CW)
// LED 1: vocoder enabled

#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPod hw;

static constexpr int kBands = 10;
static const float   kBandFreqs[kBands] = {
    120.f, 180.f, 270.f, 400.f, 600.f, 900.f, 1350.f, 2000.f, 3000.f, 4500.f};

Oscillator osc;
Adsr       note_env;
Svf        mod_bp[kBands];
Svf        car_bp[kBands];
float      band_env[kBands] = {};
Parameter  p_mix;

bool    vocoder_on = false;
bool    gate       = false;
uint8_t cur_note   = 0;
float   velocity   = 0.f;
float   env_coef   = 0.f;

void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case MidiMessageType::NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            if(p.velocity == 0)
            {
                if(p.note == cur_note)
                    gate = false;
                break;
            }
            cur_note = p.note;
            velocity = p.velocity / 127.f;
            osc.SetFreq(mtof(cur_note));
            gate = true;
        }
        break;
        case MidiMessageType::NoteOff:
        {
            NoteOffEvent p = m.AsNoteOff();
            if(p.note == cur_note)
                gate = false;
        }
        break;
        default: break;
    }
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();

    if(hw.button1.RisingEdge())
        vocoder_on = !vocoder_on;

    float mix = p_mix.Process();
    if(!vocoder_on)
        mix = 0.f;

    for(size_t i = 0; i < size; i++)
    {
        float dry = in[0][i];
        float car = osc.Process() * note_env.Process(gate) * velocity;

        float wet = 0.f;
        for(int b = 0; b < kBands; b++)
        {
            mod_bp[b].Process(dry);
            float mag = fabsf(mod_bp[b].Band());
            band_env[b] += env_coef * (mag - band_env[b]);

            car_bp[b].Process(car);
            wet += car_bp[b].Band() * band_env[b];
        }
        wet *= 4.5f;

        float sig = dry * (1.f - mix) + wet * mix;
        out[0][i] = sig;
        out[1][i] = sig;
    }

    float led = vocoder_on ? 0.55f : 0.f;
    hw.led1.Set(0.f, led * 0.2f, led);
    hw.led2.Set(0.f, 0.f, gate ? 0.35f : 0.f);
    hw.UpdateLeds();
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(48);
    float sr = hw.AudioSampleRate();

    osc.Init(sr);
    osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    osc.SetAmp(1.f);

    note_env.Init(sr);
    note_env.SetAttackTime(0.005f);
    note_env.SetDecayTime(0.05f);
    note_env.SetSustainLevel(0.85f);
    note_env.SetReleaseTime(0.12f);

    // ~5ms envelope follower
    env_coef = 1.f - expf(-1.f / (0.005f * sr));

    for(int b = 0; b < kBands; b++)
    {
        mod_bp[b].Init(sr);
        mod_bp[b].SetFreq(kBandFreqs[b]);
        mod_bp[b].SetRes(0.78f);
        mod_bp[b].SetDrive(0.5f);

        car_bp[b].Init(sr);
        car_bp[b].SetFreq(kBandFreqs[b]);
        car_bp[b].SetRes(0.78f);
        car_bp[b].SetDrive(0.5f);
    }

    p_mix.Init(hw.knob1, 0.f, 1.f, Parameter::LINEAR);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    hw.midi.StartReceive();

    for(;;)
    {
        hw.midi.Listen();
        while(hw.midi.HasEvents())
            HandleMidiMessage(hw.midi.PopEvent());
    }
}
