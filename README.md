# Daisy Pod MIDI Vocoder

Barebones **channel vocoder** for the [Electrosmith Daisy Pod](https://www.electro-smith.com/daisy/pod).

Your **mic** is the modulator (formants). A **MIDI keyboard** drives a saw carrier. The filter bank stamps your voice shape onto the notes you play.

Also includes `midi_vocoder_barebones.pd` — the same idea as a [plugdata](https://plugdata.org) / Pure Data patch.

## Controls

| Control | Function |
|--------|----------|
| **Button 1** | Vocoder on/off |
| **Knob 1** | Dry voice → vocoded wet |
| **MIDI in** | Carrier pitch (mono, last note wins) |
| **LED 1** | Vocoder enabled |
| **LED 2** | Note gate |

## Hardware

- Mic / SM57 → Pod line in
- MIDI keyboard → Pod MIDI (via adapter)
- 1/4" out → speakers / mixer

## Build & flash

```bash
git submodule update --init --recursive
make
make program   # ST-Link V3 / Daisy Programmer
```

Requires [libDaisy](https://github.com/electro-smith/libDaisy) + [DaisySP](https://github.com/electro-smith/DaisySP) (included as submodules / vendored trees) and an ARM GCC toolchain.
