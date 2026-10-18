// Daisy Pod MIDI channel vocoder (polyphonic)
// Mic formants shape a summed saw carrier — silent when you're not singing
// Button 1: vocoder path on/off
// Knob 1: dry voice level
// Knob 2: wet (vocoded) level
// Carrier: WAVE_POLYBLEP_SAW × 4 voices

#include "daisy_pod.h"
#include "daisysp.h"
#include "VocoderDsp.h"
#include "util/scopedirqblocker.h"

using namespace daisy;
using namespace daisysp;

DaisyPod hw;

static constexpr int kVoices = 4;

struct Voice
{
    Oscillator osc;
    Adsr       env;
    uint8_t    note;
    bool       on;
    uint32_t   age;
};

Voice     voices[kVoices];
VocoderDsp dsp;
RingBuffer<MidiEvent, 64> midi_queue;
Parameter p_dry;
Parameter p_wet;

bool     vocoder_on  = false;
uint32_t voice_age   = 0;
int FindVoice(uint8_t note)
{
    for(int i = 0; i < kVoices; i++)
        if(voices[i].on && voices[i].note == note)
            return i;
    return -1;
}

int AllocVoice()
{
    for(int i = 0; i < kVoices; i++)
        if(!voices[i].on)
            return i;
    int      oldest = 0;
    uint32_t age    = voices[0].age;
    for(int i = 1; i < kVoices; i++)
    {
        if(voices[i].age < age)
        {
            age    = voices[i].age;
            oldest = i;
        }
    }
    return oldest;
}

void VoiceNoteOn(uint8_t note, uint8_t vel)
{
    if(vel == 0)
    {
        int v = FindVoice(note);
        if(v >= 0)
            voices[v].on = false;
        return;
    }
    int v = FindVoice(note);
    if(v < 0)
        v = AllocVoice();

    voices[v].note = note;
    voices[v].on  = true;
    voices[v].age = ++voice_age;
    voices[v].osc.SetFreq(mtof(note));
    voices[v].osc.SetAmp(0.15f + 0.25f * (vel / 127.f));
}

void VoiceNoteOff(uint8_t note)
{
    int v = FindVoice(note);
    if(v >= 0)
        voices[v].on = false;
}

void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case MidiMessageType::NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            VoiceNoteOn(p.note, p.velocity);
        }
        break;
        case MidiMessageType::NoteOff:
        {
            NoteOffEvent p = m.AsNoteOff();
            VoiceNoteOff(p.note);
        }
        break;
        default: break;
    }
}

bool AnyGate()
{
    for(int i = 0; i < kVoices; i++)
        if(voices[i].on)
            return true;
    return false;
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    // Only the audio callback mutates oscillator and envelope state.
    while(midi_queue.readable())
        HandleMidiMessage(midi_queue.Read());
    hw.ProcessAllControls();

    if(hw.button1.RisingEdge())
        vocoder_on = !vocoder_on;

    float dry_amt = p_dry.Process() * 0.55f;
    float wet_amt = vocoder_on ? p_wet.Process() * 0.55f : 0.f;

    for(size_t i = 0; i < size; i++)
    {
        float car = 0.f;
        for(int v = 0; v < kVoices; v++)
            car += voices[v].osc.Process() * voices[v].env.Process(voices[v].on);
        // Fixed headroom avoids gain pumping when the chord changes.
        car *= 0.5f;
        const float raw = (in[0][i] + in[1][i]) * 0.5f;
        const float sig = dsp.Process(raw, car, dry_amt, wet_amt);
        out[0][i] = sig;
        out[1][i] = sig;
    }

    float led = vocoder_on ? 0.55f : 0.f;
    hw.led1.Set(0.f, led * 0.2f, led);
    hw.led2.Set(0.f, 0.f, AnyGate() ? 0.35f : 0.f);
    hw.UpdateLeds();
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(48);
    float sr = hw.AudioSampleRate();

    for(int v = 0; v < kVoices; v++)
    {
        voices[v].osc.Init(sr);
        voices[v].osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
        voices[v].osc.SetAmp(0.3f);
        voices[v].env.Init(sr);
        voices[v].env.SetAttackTime(0.005f);
        voices[v].env.SetDecayTime(0.04f);
        voices[v].env.SetSustainLevel(1.f);
        voices[v].env.SetReleaseTime(0.1f);
        voices[v].note = 0;
        voices[v].on   = false;
        voices[v].age  = 0;
    }

    dsp.Init(sr);
    midi_queue.Init();

    p_dry.Init(hw.knob1, 0.f, 1.f, Parameter::LINEAR);
    p_wet.Init(hw.knob2, 0.f, 1.f, Parameter::LINEAR);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    hw.midi.StartReceive();

    for(;;)
    {
        hw.midi.Listen();
        // Backpressure preserves note-offs; the callback drains this each millisecond.
        while(hw.midi.HasEvents() && midi_queue.writable())
        {
            const MidiEvent event = hw.midi.PopEvent();
            ScopedIrqBlocker lock;
            midi_queue.Write(event);
        }
    }
}
