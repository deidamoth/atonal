# WORKING VOCODER

Owner-confirmed working Daisy Pod vocoder and speech gate, preserved as hardware v1.0.0.

- Four MIDI voices; 20-band speech analysis/synthesis; smoothed gate and controls; bounded output.
- Button 1 toggles wet. Knob 1 = dry level, Knob 2 = wet level. Button 2 and encoder are unassigned.
- Exact flashed firmware attached: 92,008 bytes. SHA-256: `7accf8526cb98655df0c45bf27f32d387e071a4bed3a8d0f2bad7b3544a967d2`.
- Transfer reached 100% and reported success; dfu-util errored during exit from DFU. The owner subsequently confirmed the firmware runs and sounds good.
- Self-contained `WORKING-VOCODER.pd` for computer audio + MIDI, with setup guide and real Pd rendering tests.

See the repository README for connections, all controls, build/DFU steps and restoring the exact binary. Future firmware experiments belong in a separate repository; keep this release/tag unchanged.
