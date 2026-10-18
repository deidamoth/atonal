# WORKING VOCODER — hardware v1.0.0

## What is preserved

- Exact source and binary from the firmware the owner confirmed sounds much better, with a functional vocoder and a well-working speech gate.
- Four PolyBLEP saw voices, velocity sensitivity, 5 ms attack and 100 ms note release. Note-on velocity zero releases a note. MIDI changes are queued into the audio callback.
- 20 logarithmic bands, 120 Hz–7.5 kHz; matched cascaded unity-peak bandpasses, Q 3.5. Linear analysis, frequency-dependent 2–8.3 ms attack and 35 ms release; carrier pre-emphasis; smooth broadband speech gate.
- Stereo input averaged to mono; 70 Hz high-pass; analysis/dry gain ×8. Independent dry and wet controls (each maximum 0.55), 10 ms smoothing, output soft-limited to ±0.9.
- Audio block size 48 samples; sample rate follows the Pod default (48 kHz in this libDaisy configuration).
- Pre-edit source preserved in `backups/DaisyPodVocoder.before-dsp-20261017.cpp`. Original changes were uncommitted; this backup includes them.

## Build and flashing evidence

- Local build: `export PATH=/opt/homebrew/bin:$PATH; make` succeeded with ARM GNU 15.2.Rel1 (GCC 15.2.1). No compilation errors. Linker emitted newlib syscall-stub and RWX-segment warnings.
- Flash usage: 92,008 / 131,072 bytes (70.20%). Binary length: **92,008 bytes**.
- SHA-256: `7accf8526cb98655df0c45bf27f32d387e071a4bed3a8d0f2bad7b3544a967d2`.
- DFU identified `0483:df11`, Internal Flash at `0x08000000`.
- `make program-dfu` erased/downloaded all 92,008 bytes and printed **“File downloaded successfully”**.
- On `Submitting leave request...`, dfu-util returned **“Error during download get_status”**, error 74 (make exit 2). A subsequent DFU scan showed no device. This was a successful transfer with an error during DFU exit, not a clean command exit. No byte-for-byte device readback was performed.
- The owner subsequently confirmed the running vocoder and speech gate work well. This supplies the runtime confirmation missing immediately after flashing.

If a future flash produces the same exit error, distinguish a completed download from a failed transfer; press RESET and confirm the application runs. Do not interpret arbitrary download errors as success.

## Dependencies and tests

Git pins libDaisy `f715e4875b2d28445d0c11a0a17f064a248f69d0` and DaisySP `2c72eaf9eac5fc0dca1919d65d606da907832618`. The working local dependency directories were populated without their own `.git` metadata, so the pinned revisions alone do not prove the local trees were pristine. The saved binary is the definitive recovery artifact.

```sh
c++ -std=c++14 -O2 -Wall -Wextra tests/vocoder_dsp_test.cpp -o /tmp/daisy-vocoder-dsp-test
/tmp/daisy-vocoder-dsp-test
```

Host test passed: 39.1 dB channel rejection in its 1 kHz / 3 kHz selectivity case, nonzero four-note wet output, silence/release and bounded overload output. Hardware CPU utilization and objective vowel intelligibility were not measured. Exact microphone, preamp, keyboard model and gain positions were not recorded.

## Version policy

The `v1.0.0` tag and release preserve this hardware baseline and exact binary. Do not move the tag or replace that binary. Future firmware experiments should start in a separate repository copied from this version. Computer-patch additions do not alter the saved hardware firmware. The local folder remains `daisy-pod-midi-vocoder`; the GitHub repository is named `working-vocoder`.
