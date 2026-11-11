# ScratchPad v1.18 — Skell Pad Bone Edition

JUCE 8/C++17 scratch-speed VST3 and Standalone effect, with an original vector UI, live circular recording buffer, two-head crossfades, and a freely editable speed envelope.

## v1.18 — SKELL PAD brand correction and precise UI cleanup

- Footer brand now displays **SKELL** (two Ls) in ivory immediately followed by green **PAD**, with glyph-based spacing preserved at every supported size.
- Removed the redundant pan scrollbar at full-envelope zoom. It reappears only when the graph is zoomed, and is interactive only then.
- Subdued the default bend diamonds so the drawn envelope stays visually legible; hovering a bend handle still highlights its editing target.
- Hid the playhead and trailing dots until actual audio reaches the processor.
- Clarified STOP's subtitle to **MUTE OUTPUT** and **MUTED**, reflecting what its audio gain control actually does. Expanded Loop Lock's tooltip and updated About instructions.
- DSP, 36 presets, existing six automation IDs and core timing remain unchanged.

## v1.17 — Signal feedback, title spacing and DSP stress

- **48 Skell Key-style dotted audio ticks** at the bottom of the interface, driven by the actual peak of processed output audio (not a looping animation). A lock-free max hold captures transients between UI redraws, and the display falls gently at 30 Hz.
- **Tight SKEL PAD title kerning**: PAD now starts immediately after the measured width of SKEL, in lime green as before. The vector skull remains at the left.
- **Zero-latency feedback**: metering is a sidechain-only visualization of the existing output and adds no signal processing delay or host PDC.
- **Extreme stress regression**: high/low-speed sinc, stereo/mono tap safety, circular-buffer seams, sampled-output limits, invalid/silent input, peak-hold concurrency, denormal-range inputs, and huge virtual read positions.
- **Windows JUCE regression**: real processor audio metering and updated tight two-color branding are checked in the editor render.

## v1.16 — Visual unification with Skell Key + Skell Spacer

- The UI is now the **Bone house theme** used by Skell Key and Skell Spacer: warm near-black #0b0a09, panels #131210, bone-white #e9e1cc, and restrained lime #b6f56a.
- A matching vector **skull at bottom left** sits beside **SKEL** (bone) **PAD** (green). The actual plugin identity, preset data and automation IDs remain ScratchPad-compatible.
- Replaced the heavily glowing double-neon shell and segmented platter art with flat corner-bracket cards, a quieter graph, simple buttons, and a slimmer ring dial. No control coordinates have moved.
- Keep the product-name word in green on other Skell titles when those repositories are updated; this project changes ScratchPad only.

## v1.15 — Integrated DSP and rendered UI verification

- Adds an **actual JUCE processor/editor smoke executable** used by Windows CI. It runs audio through the real `processBlock()` and checks dry passthrough, locked-loop capture playback after input goes silent, Stop automation, fast playback, non-finite input handling, and zero reported PDC latency.
- Generates **actual JUCE editor snapshots** at the baseline 441×263 and two larger window sizes; the CI artifact now includes the PNGs alongside the VST3 and Standalone build. This is a real component render, not a mock UI.
- Curve editor: press **F** or **Ctrl+0**, or double-click empty graph space, to reset zoom; press **Escape** to deselect a point.
- Locked playback now shows actual scratch speed/direction in the graph header instead of a permanent `LOOP LOCKED` message; capture status remains visible on the Loop Lock button.
- The existing JUCE-free seven C++ regression suites remain available without downloading JUCE.
- New plugin instances start with a more balanced **Skelly Starter** envelope instead of a crowded left-edge scribble; saved session curves and factory presets are unchanged. Before the first audio block, the UI reads **AWAIT AUDIO** rather than showing misleading 0% capture progress.

## v1.14.1 — CI and regression cleanup

- Replaced Release-disabled `assert` with a test-only `REQUIRE` macro, so all checks run unconditionally with no MSVC `/DNDEBUG`/`/UNDEBUG` override warning.
- Updated Windows Actions to official Node.js 24-compatible `checkout@v6` and `upload-artifact@v6`, addressing runner deprecation warnings.
- Added a static editor-geometry check at 441×263, 661×394, 882×526, and 1323×789. The check verifies components stay inside the window and do not overlap; it does **not** substitute for an actual JUCE UI render.
- Expanded the seam regression to measure adjacent-output jumps at ±2× and ±4× against an unconditioned discontinuous loop.

## What's new in v1.14

- **Smooth locked-loop seams:** at the capture boundary, the first and last ~1.5 ms of playback are gently eased toward a shared midpoint. This prevents a hard jump from the last recorded sample to the first at high or reverse speeds, without altering the stored loop. Constant/periodic loops are not significantly affected.
- **Band-limited peak safety:** high-speed sinc interpolation is constrained by the peak of the original source taps, preventing reconstruction overshoot while respecting DAW floating-point headroom. NaN taps are rejected.
- **UI:** clearer keyboard focus rings on Stop and Loop Lock, without changing the compact 441×263 Skelly layout.
- **Tests:** seven JUCE-independent regression suites, including aggressive forward/reverse seam traversal and full-scale impulse bounds. Release and sanitizer builds can run without downloading JUCE:

```sh
cmake -S . -B build-core -DSCRATCHPAD_CORE_TESTS_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --parallel
ctest --test-dir build-core -C Release --output-on-failure
```

- **DSP profiling:** `g++ -O3 -std=c++17 tests/dsp_benchmark.cpp -o bench && ./bench` profiles the stereo 4x filtered read path at 44.1–192 kHz. CPU estimates depend on the machine and exclude the full JUCE host callback.

## What's new in v1.13

### True Loop Lock (new automatable control)

- **LOOP LOCK** below STOP starts recording the **next complete input loop** into a dedicated, preallocated stereo/mono capture buffer. The button shows capture progress and **HELD** when ready.
- Once ready, the captured buffer is **immutable**: subsequent live input can never overwrite it, even with a 0x hold, long reverse play, or extended scratching. The live recording ring keeps running independently.
- Lock/unlock uses an 8 ms equal-gain morph, and loop-boundary/host-seek playback jumps get a 2 ms secondary read-head fade. Changing loop-length choice while locked re-arms a fresh capture after fading to live.
- Both the new **loopLock** boolean and the original **stopTrigger** boolean can be automated in FL Studio. Previously saved sessions retain all five original parameter IDs; loopLock defaults off in older sessions.
- The lock-mode parameter is saved with the project, **but not the captured audio**. Reopening a project with lock enabled captures fresh incoming audio rather than loading stale audio from project state. Don't rely on the lock buffer surviving an offline project reload.

### High-speed anti-alias interpolation

- Forward and reverse speeds **above 1x** interpolate via a precomputed **32-tap, 64-phase Blackman-windowed sinc** with multiple speed-specific low-pass banks (1x/1.5x/2x/3x/4x). Smooth crossfades prevent tonal switches around 1x.
- Low-speed motion continues using bounded cubic Hermite interpolation to reduce CPU usage. High-speed processing is allocation-free in the callback; sinc tables are prepared once in `prepareToPlay`.
- New JUCE-free regression tests measure **9 kHz alias attenuation at 4x**, **1 kHz passband preservation**, fractional interpolation stability, and immutable loop storage across repeated input writes.

### Compact UI

- Preserves the **441×263** Skelly neon-green design with a compact Loop Lock button below Stop, a capture-progress indicator, and a **LOOP LOCKED** status in the envelope display.
- Existing 36 presets, draggable curve dots and bend handles, and reset/undo/redo workflow remain available.

## What's new in v1.12

- **Frozen-audio DC protection**: a lightweight 12 Hz wet-only DC blocker removes the constant offset that a held read-head sample can produce at 0× speed. Dry input at 0% Scratch Amount remains completely unfiltered.
- **Preset navigation**: the footer now synchronizes its factory index when a saved preset name returns after undo/redo or state recall. Clicking an arrow navigates relative to the currently displayed factory preset.
- **More discoverable shape editing**: hovering a diamond handle shows its segment bend amount; selected node details retain priority.
- **Offline regression**: tests verify DC settles during freeze and 440 Hz midrange energy remains within ±2% across 44.1, 48, 96 and 192 kHz.

## What's new in v1.11

- **Reliable startup capture**: the wet signal is muted until an entire loop has been recorded, keeping the dry input audible rather than switching to partially empty history. The wet path fades in over 2 ms when capture completes.
- **Optimized DSP**: at 0% Scratch Amount and during capture, the engine skips unnecessary Hermite reads while continuing to record input and track playback safely.
- **UI polish**: visually clearer node-selection guides, brighter bend grips, a crisp playhead marker and explicit 1/16-beat snapping reminder.
- **Rendering performance**: only the visible portion of a zoomed curve is tessellated, saving editor CPU in dense 512-point envelopes.
- **Regression**: added startup capture-gating and read-head geometry checks. JUCE/Windows compilation and DAW listening tests still determine release readiness.

## What's new in v1.10

- **Host PPQ alignment:** synced transport restart and timeline seeks resume at the correct musical phase when the host supplies PPQ; unsynced mode resets to the first point.
- **Beat-aware editing:** snapped nodes and arrow nudging track the selected loop length. The grid is 1/16 beat; hold Alt to bypass snapping.
- **Smooth scratches:** a 0.4 ms sample-rate-independent speed slew reduces zipper noise from live node edits and automation.
- **Predictable undo:** restores saved preset names alongside node positions.
- **CPU:** branch-based write-head wrapping instead of per-sample modulo.
- **Tests:** five JUCE-free suites now include PPQ timing and speed-slew checks.

## What shipped in v1.9

- **0% Scratch Amount** now gives clean, zero-latency input rather than freezing playback. Intermediate values blend dry and scratched audio, and the wet path smoothly fades in after capture begins.
- **Audio engine:** bounded Hermite interpolation is now independently regression-tested, non-finite input samples are sanitized before recording, and audio-thread work is reduced using a monotonic curve cursor plus branch-based ring wrapping.
- **Interface:** real-time direction/speed (`FWD`, `REV`, `HOLD`), capture progress, a loop-length-aware beat grid, and immediate UI updates on recalled curves. The scratch knob has animated continuous segment lighting and supports keyboard arrows/Home/End and mouse wheel.
- **Reliability:** old curve snapshots are reclaimed outside the audio callback, mono and stereo bus layouts are supported, preset state is validated before restoring, and Release regression tests now keep assertions enabled.
- **Packaging:** GitHub Actions uploads the VST3 and Standalone app in a single-extract artifact (no nested ZIP).

This update retains the 36 factory presets, automation IDs, editable curve points, neon-green design, 441×263 baseline and fixed aspect ratio. Windows/FL Studio compilation and listening tests are **not yet verified**.

## Build

Requirements: CMake 3.22+, Windows MSVC 2022/2026 or a suitable C++17 toolchain, and JUCE 7+ (by default CMake fetches JUCE 8.0.12).

```sh
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

On non-Windows generators, omit `-A x64`. Supply `-DJUCE_PATH=/path/to/JUCE` if JUCE is already checked out. GitHub Actions > **Build ScratchPad Windows** builds both the VST3 and Standalone app and stages them in one artifact.

## Curve controls

| Action | Behavior |
|---|---|
| Click empty graph space | Add a node exactly where you click (subject to snap) |
| Drag circle | Change a node's time and speed; endpoint times stay fixed at 0 and 1 |
| Drag diamond between circles | Bend that curve segment's shape (tension) |
| Hold Alt while editing | Bypass beat-aware 1/16-beat and 1/8-speed snapping |
| Double-click node | Delete an interior node; endpoints cannot be deleted |
| Right-click graph | Reset, invert, reverse, stretch, undo/redo, factory presets; right-click interior node to delete |
| Ctrl+wheel | Zoom horizontally toward cursor position (up to 8x) |
| Wheel | Scroll horizontally when zoomed |
| Drag bottom bar | Scroll time axis |
| Click a node | Select it (the white node and outline persist after mouse-up) |
| Delete / Backspace | Delete the selected interior node (or the one under the cursor) |
| Arrow keys | Nudge the selected node in time or speed; Shift+arrows fine-adjust |
| Ctrl+Z, Ctrl+Y | Undo/redo editing, including factory preset changes from the footer (click graph first for keyboard focus) |

The speed axis responds to the selected ±1x, ±2x or ±4x range, and the selected point shows its time and speed numerically. The static grid cache rasterizes at up to 3× logical resolution for smoother scaling. Branding uses a built-in system font, not a separately distributed font file.

The curve is saved in JUCE plugin state, including x/y/tension. Graph edits change the name to **Custom**. The editor renders the same curve interpolation formula used by the processor.

## Controls and factory presets

- **SCRATCH**: audio-automatable depth 0–100%; 0% is dry input and 100% is full scratch. Mouse wheel or arrow keys adjust it (Shift+arrows for fine adjustments; Home/End to jump to 0/100). Double-click resets to 100%.
- **STOP**: host-automatable latched boolean. 1 = mute/hold speed at 0; 0 = resume. The UI reflects host automation and a click emits an automation gesture.
- **MENU**: 36 factory presets, loop length (1/4–4 bars), speed range (±1/±2/±4x), tempo sync, save/load/init/about. Choices sent with host parameter gestures.
- **RENAME**: in-place preset name editor. Save/Load use `.scratchpad` files, containing both parameters and curve state.
- **Arrows**: cycle through all 36 factory presets.

## Engineering notes

- Audio uses a ring buffer of input history; it **does not capture a permanently immutable loop**. Near-frozen playback can be retargeted before the input write head overwrites it. The retarget transition uses a 2 ms two-head smoothstep crossfade.
- Hermite interpolation is bounded to its nearby sample extrema to avoid polynomial overshoot. Scratch amount and playback speed range are smoothed.
- Curve snapshots are exchanged atomically; reclamation is deferred from the audio callback. Non-audio curve publications are serialized by a mutex; the audio callback never takes that mutex.
- The plugin reports zero algorithmic latency, not zero age of the looped audio.
- CPU usage, audible click behavior, automation in FL Studio and Windows VST3 build success **have not been verified here**; validate before distributing commercially. UI typography uses system fonts; no separately distributed font is required.

## Offline regression tests

The JUCE-free test files can also be built directly:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Werror tests/curve_math_test.cpp -o curve_test && ./curve_test
g++ -std=c++17 -O2 -Wall -Wextra -Werror tests/head_safety_test.cpp -o head_test && ./head_test
g++ -std=c++17 -O2 -Wall -Wextra -Werror tests/preset_bank_test.cpp -o preset_test && ./preset_test
g++ -std=c++17 -O2 -Wall -Wextra -Werror tests/dsp_regression_test.cpp -o dsp_test && ./dsp_test
```

Tests cover curve normalization, endpoints, insertion-collision cleanup, finite data, interpolation, all 36 preset envelopes, head wrapping and overwrite geometry. They do not replace DAW audition tests.


## v1.7.1 Windows compiler compatibility

Fixed MSVC C3861 (`setTooltip` not found) in `Source/ui/ScratchKnob.cpp`.
`ScratchKnob` now derives from `juce::SettableTooltipClient` in addition to
`juce::Component`, and the editor owns a `juce::TooltipWindow` so the knob
and stop tooltips display inside the plugin editor. This patch does not
change plugin IDs, parameter IDs, preset formats, or DSP behavior.

## v1.8 — UI and curve-editor refinement

- Cleaner dark-neon palette, lighter grid density, less glow, readable system UI fonts while preserving the original Skelly logotype.
- Rebuilt footer for clearer Menu, Rename, preset arrows and name placement, with hover feedback.
- Stop remains fully automatable but displays an explicit **ENGAGED / CLICK TO HALT** sub-label instead of ambiguous state text.
- Persistent node selection, keyboard delete and arrows, single-gesture undo on click-add-drag, and preset changes captured in undo history.
- Accurate speed-axis tick labels when changing the speed-range parameter.
- Curves and playback markers remain vector paths; high-density cached static grid reduces pixelated text at enlarged window sizes.
- Reduced idle footer repaints. No changes to plugin IDs or parameter identifiers; preset and host state formats are unchanged.

This release was checked with standalone GCC C++17 offline tests. A full JUCE/MSVC or DAW build still needs verification.
