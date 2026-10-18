# WORKING VOCODER

**Working Daisy Pod MIDI vocoder — v1.0.0.** Owner-confirmed working audio and speech gate after flashing. This is the saved hardware baseline; future firmware experiments belong in a separate repository.

## Daisy Pod setup

Mic → microphone preamp → Pod line input. MIDI keyboard → Pod MIDI input using the correct adapter. Pod audio output → headphones / mixer / speakers. Left and right inputs are averaged to mono; output is the same on both channels. A left-only input is reduced by 6 dB. USB connects the Seed to the computer for flashing.

| Control | Saved firmware mapping |
|---|---|
| Button 1 | Toggle vocoder wet path; starts **off** |
| Knob 1 | Dry voice level: left = none, right = maximum |
| Knob 2 | Vocoder wet level: left = none, right = maximum |
| MIDI keys | Four velocity-sensitive saw voices; oldest held voice stolen when full |
| LED 1 | Blue/cyan when vocoder enabled |
| LED 2 | Blue while at least one MIDI note is held |
| Button 2 / encoder turn / encoder press | Unassigned |
| Seed BOOT + RESET | Enter DFU bootloader; RESET alone restarts firmware |

For fully wet sound: Button 1 on, Knob 1 left, Knob 2 up; hold notes and speak/sing. For dry only: Button 1 off, Knob 1 up. No sustain/pitch-bend/CC mapping; notes from all MIDI channels share four voices.

**Listening note:** the owner reports Knob 1 feels like “left = vocoder, right = just audio.” The saved code adds dry audio as Knob 1 rises; it does **not** crossfade away the wet signal. The working firmware has been preserved unchanged.

## Build and flash (macOS)

Requires ARM GCC, Make, dfu-util and the pinned libDaisy/DaisySP dependencies. On a fresh clone:

```sh
git clone --recurse-submodules https://github.com/deidamoth/working-vocoder.git
cd working-vocoder
export PATH=/opt/homebrew/bin:$PATH
make -C libDaisy
make -C DaisySP
make
```

Connect the Seed with a USB data cable. Hold **BOOT**, press/release **RESET**, release **BOOT**. Then:

```sh
dfu-util -l       # Must show 0483:df11 and Internal Flash
make program-dfu  # Run only after make succeeds
```

To restore the exact saved binary without rebuilding:

```sh
shasum -a 256 -c firmware/SHA256SUMS
dfu-util -l
dfu-util -a 0 -s 0x08000000:leave -D firmware/working-vocoder-v1.0.0.bin -d ,0483:df11
```

[Build/flash record and DSP details](docs/WORKING-VOCODER-v1.0.0.md). [Exact firmware](firmware/working-vocoder-v1.0.0.bin).

## Computer version (no Daisy needed)

Open **[pd/WORKING-VOCODER.pd](pd/WORKING-VOCODER.pd)** in Pure Data Vanilla or plugdata standalone. It is self-contained; no external objects or extra patch files are needed. Select your audio input/output and MIDI keyboard in the application's settings, then enable audio/DSP. Start with headphones, dry at zero, wet at 0.55 and the vocoder enabled. Hold a chord and speak. [Computer setup and controls](pd/README.md).

The old `midi_vocoder_barebones.pd` is a historical prototype, not the finished computer version.
