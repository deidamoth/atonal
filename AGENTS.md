# Project boundaries

This is the granular expansion repository, `deidamoth/working-vocoder-plus-gran`.
The baseline `/Users/justinasoto/Documents/daisy-pod-midi-vocoder` and GitHub
`deidamoth/working-vocoder` must remain untouched. Do not push to that remote.
Do not modify the separate `daisy-pod-sine-flash` project.

Preserve the vocoder DSP and four MIDI voices unless the user explicitly requests
changes. The existing Knob 2 reassignment fixes base wet level at 0.55 and uses
Knob 2 / Button 2 for granular amount / enable. Keep exact bypass behavior.

Build with `/opt/homebrew/bin` on PATH. Only flash a successfully built image
when DFU detection identifies the Seed. Distinguish successful transfer from
DFU-exit errors and hardware runtime verification. Keep README controls and
validation notes accurate; do not call an untested firmware hardware-verified.
