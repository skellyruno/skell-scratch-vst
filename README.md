# ScratchPad — JUCE VST3 / Standalone source

Original vector scratch-speed envelope plugin. Based on the uploaded JUCE/GitHub workflow template.

## Build

- Install CMake >= 3.22, C++17 compiler, and optionally JUCE 7+.
- `cmake -S . -B build -DJUCE_PATH=/path/to/JUCE` (otherwise CMake fetches JUCE 8.0.12).
- `cmake --build build --config Release --parallel 2`
- GitHub Actions -> **Build ScratchPad Windows** -> Run workflow to download a zipped VST3 artifact. Standalone is built alongside it.

## Features

Drag red nodes; click the graph to add; double-click to remove; drag near a segment midpoint to modify its bend; right-click graph for editing commands and presets. Alt bypasses 1/64-time and 1/8-speed snapping. Wheel scrolls, Ctrl+wheel zooms. Drag knob vertically or double-click to reset. Stop is a momentary gesture. Save / Load presets use binary JUCE state. All five parameters are in APVTS; curve nodes and preset name are persisted alongside state.

## DSP notes / limits

- Continuous live input ring buffer; each loop is an output envelope cycle, not a one-time capture. Speed 0 holds the current sample, +/- speed scrubs backward/forward with Hermite interpolation.
- Loop timing follows host BPM when sync is enabled; without a reported tempo, uses 120 BPM.
- Zero reported algorithmic latency; note that looped playback necessarily reproduces previously recorded audio and cannot predict future input.
- Boundary fade is a simple output-blend anti-click strategy, **not** dual-head phase-consistent overlap-add. Extreme scrubbing and repeated loop resets may still produce artifacts. The first pass is functional source, not a sonically validated production release.
- This environment does not ship JUCE's SDK, so compile/run verification should be done with the included Windows GitHub workflow.
