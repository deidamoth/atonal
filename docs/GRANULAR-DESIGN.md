# Granular expansion: reference and implementation

## Reference

The reference is **MONTAG-E by Antonio Blanca**, preset **Chord Piano Speak**, as confirmed by the owner. The Reaktor header shows snapshot 019 with an asterisk, suggesting an edited snapshot; the video overlay misspells the preset name. The requested passage is about 3:56 in X3N's “Is This the CRAZIEST Granular Synth Ever?”; no playable video link/audio was supplied during implementation, so that passage has not been auditioned. We cannot infer exact preset parameters or the source piano sample from the screenshot/transcript.

The [developer's overview](https://antblanca.com/pages/montag-e) documents four playheads on one sample, parameter spread, modulation, and a filter/delay section. The screenshot highlights position randomization and pan. These motivate a live four-head texture, not a copy of the proprietary instrument or sample library. The developer also links an [interface overview PDF](https://drive.google.com/file/d/15oRclBgJvsMi5fPHqRHmxxSGxQEJZ3Sd/view); its contents were not accessible to the browser here.

## Signal path

```text
microphone + four MIDI voices → unchanged VocoderDsp → original immediate signal
                                                   ↘ capture → four grain heads
                                                       → stereo delay → add FX
```

- The original `VocoderDsp.h` is byte-identical to baseline commit `7fa383f0bfd8865243ea88831d7b9c5791713bde`.
- Knob 2 becomes the FX amount; the old wet level is fixed at 0.55. This is the necessary control-routing change, not a change to the vocoder synthesis.
- The immediate signal is retained at unity before final peak protection. Grain recording uses FX-only high-pass/low-pass filtering; bypass never passes through it.
- Capture ring: 131,072 floats, about 2.73 s at 48 kHz. Two 32,768-float delay buffers. Total SDRAM: 768 KiB.
- Four independently randomized heads, each with two overlapping Hann-window grains (eight grains maximum). Grain lengths: 110 / 145 / 190 / 160 ms. Nominal read offsets: 100 / 235 / 380 / 510 ms, with ±47.5 ms random displacement.
- Rates: 1× / 1× / 0.5× / 2×; octave layers have lower gain to retain the played harmony. These are our design choices, not decoded preset settings.
- Per-grain position, level and stereo pan variation. Parameters are latched at grain boundaries to avoid abrupt read-pointer jumps.
- Linear-interpolated reads, fourth-order 5.2 kHz low-pass before capture, and a 100 Hz high-pass on the FX feed. Writer/read separation accounts for pitch speed and grain lifetime.
- Damped cross-feedback echoes at 271 / 379 ms, feedback 0.32. Echo is never recaptured into the granular history, so there is no recursive granular feedback loop.
- Knob amount changes only the added FX level; it does not change the base pitch, filter bank or speech gate. A 1.5% endpoint deadband makes full-left reliable.
- FX off/full-left returns to exact bypass after a 30 ms fade. Old delay state is invalidated without clearing a large buffer in the callback. Capture continues while bypassed so new FX can use recent speech.
- Stereo-linked peak protection caps enabled-FX output at ±0.9. Bypass returns the already-bounded original signal exactly.
- No heap allocation or bulk clearing in audio processing. Sine-window generation and buffer clearing happen after hardware/SDRAM initialization and before audio starts.

## Hardware performance

The additional DSP is bounded at eight active grains plus filters/delay. `CpuLoadMeter` measures callbacks; `cpu_load_average` and `cpu_load_peak` can be inspected with a debugger, and a red LED 2 component flags average load above 85%. These are instrumentation, not measurements: actual CPU headroom, SDRAM behavior and perceived quality still require running the new build on the Pod.
