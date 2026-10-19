# Working Vocoder Plus Gran Plus Sample

Same firmware as [working-vocoder-plus-gran](https://github.com/deidamoth/working-vocoder-plus-gran), plus a parallel one-shot.

The vocoder DSP, saw voices, speech gate, Knob 1, Knob 2, and granular trail are unchanged. The squeak is mixed in **after** that chain. It does not go through the vocoder or the grains.

| Extra behavior | |
|---|---|
| Sample | `CHAIR_NIME.wav`, treated as **F4** (MIDI 77) |
| Trigger | Same MIDI notes, only while the vocoder (Button 1) is on |
| Pitch | Chromatic, like Simpler: each note retunes the one-shot |
| Voices | 4 one-shots, independent of the saws |
| Note off | Does not cut the squeak; it plays through |

Button 1 still turns the vocoder on/off, and that also opens or mutes the sample layer.

# Working Vocoder Plus Gran

A **separate expansion** of [WORKING VOCODER v1.0.0](https://github.com/deidamoth/working-vocoder/releases/tag/v1.0.0). The original repository, release and firmware remain untouched. This version adds a stereo granular layer after the working four-voice vocoder.

**Status:** builds and passes host DSP tests; firmware transfer completed; Pod startup and listening tests still pending. This is an experimental version, not a claim of an exact MONTAG-E preset recreation.

## Controls

| Control | Function |
|---|---|
| Button 1 | Vocoder on/off, unchanged; starts off |
| Knob 1 | Dry voice level, unchanged |
| Button 2 | Granular FX on/off; starts off |
| Knob 2 | Granular amount: left = original sound, right = most added texture |
| LED 1 | Blue/cyan = vocoder enabled |
| LED 2 | Green = FX enabled; blue = MIDI held; cyan = both; red component = average CPU use above 85% |
| Encoder turn/press | Unassigned |

**Knob 2 was the original vocoder wet-level control.** To repurpose it, the base vocoder wet level is fixed at its former maximum, **0.55**. Button 1 still mutes that wet path. The original vocoder DSP, gate, MIDI voices, Knob 1 and note handling are preserved. FX bypass matches the baseline with its original Knob 2 at maximum; it cannot preserve arbitrary previous wet-knob positions simultaneously.

Start with Knob 1 left, Knob 2 left. Press Button 1, hold a chord and sing. Press Button 2 and gradually raise Knob 2. The immediate voice stays present while small overlapping fragments move around it. Returning Knob 2 left or switching FX off fades back to the original mono signal over 30 ms.

The effect processes the **whole vocoder output**, including dry voice if Knob 1 is up. The original speech gate still works; FX tails can continue briefly after you stop singing. With FX bypassed, the original silence behavior returns. Stereo output reveals the movement best; both channels also work summed to mono.

## Connections and build

Mic → preamp → Pod line input; keyboard → Pod MIDI input; Pod output → headphones/mixer. Left/right inputs are averaged as before. USB data cable → Seed for DFU.

```sh
git clone --recurse-submodules https://github.com/deidamoth/working-vocoder-plus-gran.git
cd working-vocoder-plus-gran
export PATH=/opt/homebrew/bin:$PATH
make -C libDaisy
make -C DaisySP
make
```

To flash **only after a successful build**: hold Seed BOOT, press/release RESET, release BOOT; then:

```sh
dfu-util -l       # must identify 0483:df11, Internal Flash
make program-dfu  # writes build/DaisyPodVocoderGranular.bin
```

The original working binary is included for recovery:

```sh
shasum -a 256 -c firmware/SHA256SUMS
# Enter DFU again before restoring:
dfu-util -a 0 -s 0x08000000:leave -D firmware/working-vocoder-v1.0.0.bin -d ,0483:df11
```

[Design/research](docs/GRANULAR-DESIGN.md) · [Validation](docs/GRANULAR-VALIDATION.md) · [15-second synthetic preview](previews/granular-comparison.wav): 0–5 s bypass, 5–10 s half FX, 10–15 s full FX. Each section includes pauses/tails. It demonstrates this code with a synthetic vowel/chord, not a microphone recording or the reference video's audio.

The inherited `pd/WORKING-VOCODER.pd` remains the **baseline computer vocoder**; this expansion currently targets Daisy firmware only.
