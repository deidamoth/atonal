# Project boundaries

This is `deidamoth/atonal`, based on working-vocoder-plus-gran-plus-sample at 7db4bb0.
Do not modify or push to working-vocoder, working-vocoder-plus-gran,
working-vocoder-plus-gran-plus-sample, daisy-pod-midi-vocoder, or daisy-pod-sine-flash.

Do not edit VocoderDsp.h or GranularFx.h. Preserve four saw voices, speech gate,
controls, exact granular bypass, and the separate embedded one-shot's routing
and note-off behavior. Base wet level stays 0.55. Both saws and sample use
24-TET anchored at MIDI 65 via QuarterTonePitch.h.

Build with /opt/homebrew/bin on PATH. Use USB DFU, not ST-Link. Flash only a
successfully built image after dfu-util -l identifies 0483:df11 Internal Flash.
Report successful transfer separately from DFU-exit errors and hardware testing.
