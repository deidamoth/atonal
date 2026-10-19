# Atonal

Based on [working-vocoder-plus-gran-plus-sample at 7db4bb0](https://github.com/deidamoth/working-vocoder-plus-gran-plus-sample/commit/7db4bb0).

The four saw carriers and embedded `CHAIR_NIME.wav` one-shot share a
**seven-note microtonal scale on the white keys**, with a neutral third and
neutral seventh. It uses pitches from 24-tone equal temperament, but no longer
moves one quarter tone per MIDI key. A keyboard octave now sounds an octave.
The anchor remains **MIDI 65 (F, 349.228231 Hz)**, called F3 in this project;
octave labels vary between keyboards. The sample plays at recorded speed there.

| White key (MIDI) | Sound relative to F | Cents above anchor |
|---|---|---|
| F (65) | F | 0 |
| G (67) | G | 200 |
| A (69) | A half-flat (neutral third) | 350 |
| B (71) | B-flat (fourth) | 500 |
| C (72) | C (fifth) | 700 |
| D (74) | D (sixth) | 900 |
| E (76) | E half-flat (neutral seventh) | 1050 |
| F (77) | F, octave above | 1200 |

Black keys provide extra color between adjacent white-key pitches, using the
midpoint rounded upward to the nearest quarter tone: MIDI 66/68/70/73/75 map to
100/300/450/800/1000 cents above the anchor. This pattern repeats every 12 MIDI
keys, including below the anchor. Note 89 is now two octaves above the anchor.
Both sample ratio and saw frequency use the same mapping in `QuarterTonePitch.h`.
This is our chosen scale, not a verified transcription of Erica Eso.

The vocoder DSP, four saw waveforms, speech gate, granular trail, and controls
are preserved. This changes carrier and sample tuning; it does not add a pitch
correction stage to the microphone input.

The sample triggers on the same MIDI notes while Button 1 enables the vocoder.
Its four independent one-shot voices play through after note-off (a fifth
trigger steals the oldest voice). Button 1 also mutes the sample when off.
The sample is mixed after the vocoder and grains, and never passes through either.

**Validation:** revised-scale build passed, along with host checks of white-key
intervals, black-key pitches, anchor speed, monotonicity and octave repetition
across all 128 MIDI notes. Vocoder/granular DSP and embedded sample remain
unchanged from 7db4bb0; inherited DSP tests passed for the initial atonal version.
USB DFU detected `0483:df11` Internal Flash and successfully transferred all
**128,280 bytes** to `0x08000000`. The leave request then reported
`Error during download get_status` / error 74. Hardware startup and listening
tests of this revision remain pending; tap RESET if needed.
Inherited validation documents and recovery binaries describe earlier firmware.

## Controls

| Control | Function |
|---|---|
| Button 1 | Vocoder and sample on/off; starts off |
| Knob 1 | Dry voice level, unchanged |
| Button 2 | Granular FX on/off; starts off |
| Knob 2 | Granular amount: left = original sound, right = most added texture |
| LED 1 | Blue/cyan = vocoder enabled |
| LED 2 | Green = FX enabled; blue = MIDI held; cyan = both; red component = average CPU use above 85% |
| Encoder turn/press | Unassigned |

**Knob 2 was the original vocoder wet-level control.** To repurpose it, the base vocoder wet level is fixed at its former maximum, **0.55**. Button 1 still mutes that wet path. The original vocoder DSP, gate, four saw voices, Knob 1 and note handling are preserved; only the pitch mapping changes. FX bypass matches the baseline with its original Knob 2 at maximum; it cannot preserve arbitrary previous wet-knob positions simultaneously.

Start with Knob 1 left, Knob 2 left. Press Button 1, hold a chord and sing. Press Button 2 and gradually raise Knob 2. The immediate voice stays present while small overlapping fragments move around it. Returning Knob 2 left or switching FX off fades back to the original mono signal over 30 ms.

The effect processes the **whole vocoder output**, including dry voice if Knob 1 is up. The original speech gate still works; FX tails can continue briefly after you stop singing. With FX bypassed, the original silence behavior returns. Stereo output reveals the movement best; both channels also work summed to mono.

## Connections and build

Mic → preamp → Pod line input; keyboard → Pod MIDI input; Pod output → headphones/mixer. Left/right inputs are averaged as before. USB data cable → Seed for DFU.

```sh
git clone --recurse-submodules https://github.com/deidamoth/atonal.git
cd atonal
export PATH=/opt/homebrew/bin:$PATH
make -C libDaisy
make -C DaisySP
make
```

To flash **only after a successful build**: hold Seed BOOT, tap RESET, keep holding BOOT for about two seconds, then release BOOT; then:

```sh
dfu-util -l       # must identify 0483:df11, Internal Flash
make program-dfu  # writes build/DaisyPodVocoderGranular.bin
```

A completed transfer prints `File downloaded successfully` and the full byte count.
An ensuing error 74 / `Error during download get_status` can be the Pod leaving
DFU after that successful write. Tap RESET if needed. A successful transfer does
not establish startup or listening-test success. Use USB DFU, not ST-Link.

The original working binary is included for recovery (this is the old baseline, not atonal):

```sh
shasum -a 256 -c firmware/SHA256SUMS
# Enter DFU again before restoring:
dfu-util -a 0 -s 0x08000000:leave -D firmware/working-vocoder-v1.0.0.bin -d ,0483:df11
```

[Design/research](docs/GRANULAR-DESIGN.md) · [Validation](docs/GRANULAR-VALIDATION.md) · [15-second synthetic preview](previews/granular-comparison.wav): 0–5 s bypass, 5–10 s half FX, 10–15 s full FX. Each section includes pauses/tails. It demonstrates this code with a synthetic vowel/chord, not a microphone recording or the reference video's audio.

The inherited `pd/WORKING-VOCODER.pd` remains the **baseline computer vocoder**; this expansion currently targets Daisy firmware only.
