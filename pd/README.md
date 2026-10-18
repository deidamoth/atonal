# WORKING VOCODER — computer edition

**Download/open `WORKING-VOCODER.pd`.** One self-contained file. No Daisy, extra abstractions, paid plugins or external Pd libraries needed.

## Start

1. Open the file in **Pure Data Vanilla** or **plugdata standalone**. In Pd, leave Edit Mode off to use the controls.
2. In audio settings, select your microphone / interface input and headphones / interface output. Use **44.1, 48 or 96 kHz**. Grant microphone permission if the OS asks.
3. In MIDI settings, select/enable your keyboard. All incoming channels share four voices.
4. Click **DSP_audio**, leave **Vocoder_on** enabled, hold a chord and speak/sing.

Defaults: input channel 1, input gain ×8, dry 0, wet 0.55. Use the input selector for channel 2 or the average of channels 1+2. If distorted or noisy, lower input gain and/or interface gain. If the gate will not open, raise the input level. Hardware microphones need a suitable interface/preamp; built-in microphones work through the OS input device.

| Control | Function |
|---|---|
| DSP_audio | Global Pd audio processing on/off |
| Vocoder_on | Enable/mute wet path; dry remains available |
| Dry_voice | Independent dry level, 0–0.55 |
| Wet_vocoder | Independent vocoded level, 0–0.55 |
| Input_gain | Gain before analysis and dry path, ×0–16 |
| Optional_mix | Left = wet only; right = dry only; sets both level sliders |
| Input selector | First input channel / second / average |
| Panic_notes_off | Release every held note |

Start with headphones to avoid microphone/speaker feedback. Wet sound needs **both audio input and held MIDI notes**. Dry sound does not need MIDI. A keyboard must appear as a MIDI input to the host (USB or interface MIDI); a DIN-only keyboard needs a MIDI interface. Audio from another app needs OS loopback routing or an audio interface loopback input; this patch does not automatically capture system playback.

## Relationship to the Daisy

Same 20 center frequencies, cascaded Q 3.5 bandpasses, per-band envelope attack/35 ms release, 70 Hz high-pass, carrier brightness, speech-gate curve and ±0.9 output bound. Four velocity-sensitive PolyBLEP saws have 5 ms attack / 100 ms release. Unlike the Daisy's ADSR implementation, Pd uses linear voice ramps and Pd's own four-voice allocator; audio scheduling also differs. This is a working counterpart, **not a bit-identical emulation**. Vocoder starts enabled here for convenience; the Daisy starts disabled.

The optional crossfade is a desktop convenience matching the requested wet-left/dry-right behavior. The archived firmware still has independent dry/wet knobs. Sustain pedal, bend and other MIDI CCs are not mapped.

## Validation / maintenance

Validated in **Pure Data 0.56-2** with offline audio renders at 44.1/48/96 kHz: nonzero chord/vocoder output, gate closure, note release, wet bypass, dry path, and finite output bounded to ±0.9 under overload. Device-specific microphone/MIDI routing and plugdata operation still need testing on the user's machine.

```sh
python3 tools/generate_pd.py   # rebuild the self-contained .pd from readable generator
python3 tests/test_pd.py       # needs pd on PATH, or PD=/path/to/pd
```

The test opens the same embedded DSP graph in real Pd, without opening audio or MIDI hardware. Basic Pd documentation: [official manual](https://msp.ucsd.edu/Pd_documentation/).
