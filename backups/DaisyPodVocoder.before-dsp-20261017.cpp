// Daisy Pod MIDI channel vocoder (polyphonic)
// Mic formants shape a summed saw carrier — silent when you're not singing
// Button 1: vocoder path on/off
// Knob 1: dry voice level
// Knob 2: wet (vocoded) level
// Carrier: WAVE_POLYBLEP_SAW × 4 voices

#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPod hw;

static constexpr int kBands  = 12;
static constexpr int kVoices = 4;

static const float kBandFreqs[kBands] = {150.f,
                                         220.f,
                                         330.f,
                                         470.f,
                                         680.f,
                                         990.f,
                                         1400.f,
                                         2000.f,
                                         2800.f,
                                         3900.f,
                                         5200.f,
                                         7000.f};

struct Voice
{
    Oscillator osc;
    Adsr       env;
    uint8_t    note;
    bool       on;
    uint32_t   age;
};

Voice     voices[kVoices];
Svf       mod_bp[kBands];
Svf       car_bp[kBands];
float     band_env[kBands] = {};
Parameter p_dry;
Parameter p_wet;

bool     vocoder_on  = false;
uint32_t voice_age   = 0;
float    env_attack  = 0.f;
float    env_release = 0.f;
float    speech_env  = 0.f; // overall mic gate for wet path

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
    hw.ProcessAllControls();

    if(hw.button1.RisingEdge())
        vocoder_on = !vocoder_on;

    float dry_amt = p_dry.Process() * 0.55f;
    float wet_amt = vocoder_on ? p_wet.Process() * 0.55f : 0.f;

    for(size_t i = 0; i < size; i++)
    {
        float raw     = (in[0][i] + in[1][i]) * 0.5f;
        float mic_an  = SoftClip(raw * 22.f);
        float mic_dry = SoftClip(raw * 8.f);

        // speech gate — wet only when you're actually making sound
        float level = fabsf(mic_an);
        float scoef = (level > speech_env) ? env_attack : env_release;
        speech_env += scoef * (level - speech_env);
        // threshold + curve: silence → 0, talking → 1
        float speech_gate = speech_env - 0.04f;
        if(speech_gate < 0.f)
            speech_gate = 0.f;
        speech_gate = SoftClip(speech_gate * 4.f);
        speech_gate = speech_gate * speech_gate; // snappier open

        float car = 0.f;
        for(int v = 0; v < kVoices; v++)
            car += voices[v].osc.Process() * voices[v].env.Process(voices[v].on);
        car *= 0.35f;

        float wet = 0.f;
        for(int b = 0; b < kBands; b++)
        {
            mod_bp[b].Process(mic_an);
            float mag  = fabsf(mod_bp[b].Band());
            float coef = (mag > band_env[b]) ? env_attack : env_release;
            band_env[b] += coef * (mag - band_env[b]);

            // kill band noise floor so carrier doesn't leak
            float e = band_env[b] - 0.02f;
            if(e < 0.f)
                e = 0.f;
            e = e * (0.4f + 0.6f * e);

            car_bp[b].Process(car);
            wet += car_bp[b].Band() * e;
        }
        wet = SoftClip(wet * (2.2f / (float)kBands));
        wet *= speech_gate; // no singing → no synth

        float sig = mic_dry * dry_amt + wet * wet_amt;
        sig       = SoftClip(sig);
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

    env_attack  = 1.f - expf(-1.f / (0.003f * sr));
    env_release = 1.f - expf(-1.f / (0.045f * sr));

    for(int b = 0; b < kBands; b++)
    {
        mod_bp[b].Init(sr);
        mod_bp[b].SetFreq(kBandFreqs[b]);
        mod_bp[b].SetRes(0.88f);
        mod_bp[b].SetDrive(0.5f);

        car_bp[b].Init(sr);
        car_bp[b].SetFreq(kBandFreqs[b]);
        car_bp[b].SetRes(0.88f);
        car_bp[b].SetDrive(0.5f);
    }

    p_dry.Init(hw.knob1, 0.f, 1.f, Parameter::LINEAR);
    p_wet.Init(hw.knob2, 0.f, 1.f, Parameter::LINEAR);

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
