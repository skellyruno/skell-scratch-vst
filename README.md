# ScratchPad v1.19 — Skell Pad Bone Edition

JUCE 8/C++17 Windows VST3 scratch-speed effect, with an original vector UI, live circular recording buffer, two-head crossfades, and a freely editable speed envelope.

## Windows VST3 preview status

The release workflow produces **Windows x64 VST3 only**; it does not build a Standalone executable. The private Actions preview is held as `PREVIEW_ONLY` and includes the flat VST3 bundle, actual JUCE editor preview captures, strict module metadata verification, source/build provenance, and SHA-256 checksums. The workflow runs the real processor/editor CTest suite and pluginval strictness level 5 before uploading.

This preview is not a commercial release. FL Studio scan/load and session recall, real-audio A/B, target-hardware CPU/latency, support contacts, licensing/attribution approval, and signed installer/update/rollback verification remain outstanding.

## v1.19 — Short SkellCore guided tutorial