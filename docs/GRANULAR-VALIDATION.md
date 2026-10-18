# Granular expansion validation

## Build

ARM GNU 15.2.Rel1: successful firmware compile/link. Target `DaisyPodVocoderGranular`. Flash 95,144 bytes of 131,072 (72.59%); SDRAM 768 KiB of 64 MiB. The existing newlib syscall-stub and RWX load-segment linker warnings remain; no compilation errors.

## DSP checks

```sh
c++ -std=c++14 -O2 -Wall -Wextra -Werror -fsanitize=undefined tests/granular_fx_test.cpp -o /tmp/granular-fx-test
/tmp/granular-fx-test
c++ -std=c++14 -O2 -Wall -Wextra tests/vocoder_dsp_test.cpp -o /tmp/vocoder-test
/tmp/vocoder-test
```

Passed at 44.1 / 48 / 96 kHz:

- Bit-exact bypass versus the baseline vocoder at the same fixed wet setting, including changing dry level and vocoder gate.
- Knob-left bypass even with the FX button enabled.
- Nonzero granular layer and nonzero left/right difference.
- Repeated history/delay wraparound, finite samples, bounded overload output (±0.9).
- Tail energy after input stops, followed by decay to silence.
- Return to exact bypass within 30 ms; no stale echo burst after a long silent bypass.
- Buffer-edge sentinels intact; UndefinedBehaviorSanitizer reports no errors.

Apple Clang AddressSanitizer crashed during runtime initialization both inside and outside the sandbox, before entering the test. An ASan pass is therefore **not** claimed.

## Preview

`previews/granular-comparison.wav` is rendered from the actual C++ vocoder and effect with synthetic vowel harmonics and a four-note carrier. 0–5 s = bypass; 5–10 s = 50% FX; 10–15 s = maximum FX. It is a preview for listening, not a match test against MONTAG-E.

```sh
c++ -std=c++14 -O2 -Wall -Wextra -Werror tools/render_granular_demo.cpp -o /tmp/render-granular-demo
/tmp/render-granular-demo previews/granular-comparison.wav
```

## Device status

The DFU scan during development found no device. **This expansion has not been flashed.** Hardware listening, CPU measurements and comparison to the reference passage remain outstanding. The previously confirmed working vocoder stays on the device unless it is subsequently flashed with another image.

## Baseline protection

The original GitHub repository is not a remote in this checkout. New development is published only to `deidamoth/daisy-pod-vocoder-granular`. Original source/core and recovery binary are preserved in history; original v1.0.0 remains at `deidamoth/working-vocoder`.

New experimental firmware is saved in `firmware/daisy-pod-vocoder-granular-v0.1.0.bin`; its SHA-256 is recorded in `firmware/GRANULAR-SHA256SUMS`.
