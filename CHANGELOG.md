## v1.18.0 — Brand spelling and small-layout visual fixes (November 2026)
- Corrects title to **SKELL PAD**, with the double-L surname in ivory and PAD in green, tightly spaced.
- Suppresses redundant full-width pan scrollbar until zoom requires navigation.
- Reduces the visual weight of bend-diamond handles without changing hit targets or drag behavior.
- Hides playback cursor until audio is received.
- Makes STOP's actual muting behavior explicit in compact text and tooltip.
- Keeps the input-capture / lock behavior and all audio processing code unchanged.

## v1.17.0 — Audio-feedback meter, branding alignment and stress coverage (November 2026)
- Adds a real processed-output 48-segment audio activity strip in Skell Key's style without enlarging the 441×263 interface.
- Uses a realtime-safe atomic peak hold and UI-only decibel ballistics, retaining exact dry passthrough and zero reported latency.
- Fixes the visual gap between SKEL and green PAD with glyph-metric-based placement.
- Expands JUCE-free and real-JUCE integration/stress coverage across extreme speed, transient and automation scenarios.
- Preserves plugin identity, all six automation IDs, 36 presets, and existing DSP routing.

## v1.16.0 — Bone house UI and SKEL PAD branding (November 2026)
- Matches visual palette and corner-bracket cards of skell-key-vst and skell-spacer-vst, without changing the DSP or plugin identity.
- Adds the exact vector skull motif and two-color SKEL PAD footer brand.
- Replaces the oversized neon frame and segmented platter with a warm flat UI, slimmer arc dial, and simple bordered Stop/Loop Lock controls.
- Preserves click targets, editable envelope, all 36 factory presets, all existing automation parameter IDs and same resizing constraints.

# ScratchPad changelog

## v1.14.1 — Warning-free Release regression and CI cleanup (November 2026)
- Replaces all C++ test `assert` macros with unconditional `REQUIRE`, removing the root cause of MSVC D9025 NDEBUG override warnings while still evaluating checks in optimized builds.
- Upgrades GitHub checkout and artifact actions to Node.js 24-compatible major versions to address runtime/dependency deprecation warnings.
- Adds a reproducible static geometry check for five editor controls at four supported UI sizes.
- Strengthens loop-seam regression with a quantitative jump-reduction assertion at ±2× and ±4×. Audio plugin/DSP output stays identical to v1.14.0.


## v1.14.0 — Seam safety, bounded high-speed playback, accessible controls (November 2026)
- Conditions the readout of the immutable captured loop in a 1.5 ms region around the recording seam to avoid hard step discontinuities during forward/reverse scratches. This is applied at read time; stored capture samples are untouched.
- Constrains the 32-tap sinc resampler to the maximum source-tap magnitude to prevent reconstruction overshoot, and sanitizes non-finite input taps. Generalized position wrap uses a slow-path modulo only when needed.
- Adds visible keyboard-focus states to Stop and Loop Lock, preserving the compact interface and all 36 presets.
- Adds a JUCE-free CMake build switch for offline regression testing, a seventh seam regression suite and an optional 4x stereo performance benchmark.
- Preserves plugin IDs and the existing six automatable parameter IDs, including Loop Lock; the GitHub Windows workflow continues to build VST3 and Standalone.


## v1.13.0 — True locked-loop capture and aggressive scratch fidelity (November 2026)
- Appends an automatable `loopLock` boolean parameter without changing the prior five parameter IDs or VST3 identity.
- Captures the **next full input loop** into a dedicated preallocated mono/stereo buffer, never overwriting captured audio while locked. A reset/unlock crossfade safely returns to live input, and loop-length menu changes re-arm capture.
- Routes high-speed forward and reverse playback through phase- and speed-interpolated 32-tap windowed-sinc input low-pass coefficients to reduce aliasing compared with unconstrained cubic resampling.
- Retains bounded cubic interpolation for <=1x and blends gradually into band-limited processing for >1x, with no runtime coefficient allocation.
- Preserves the current waveform/UI layout, adds a compact Loop Lock button and capture progress, and retains DAW Stop automation and all 36 presets.
- Adds a 6th offline test suite covering immutable loop capture and anti-alias/passband measurements.
- Remaining release gates: successful Windows JUCE VST3 build, FL Studio automation listening tests, and profiling at 192 kHz / 4x on lower-end computers.


## v1.12.0 — Frozen-signal cleanup and preset polish (November 2026)
- Adds a 12 Hz wet-only single-pole DC blocker to prevent sustained DC when the speed envelope freezes at 0×. Preserves exact dry audio at 0% Scratch Amount.
- Automatically reinitializes the filter on engine preparation and after complete dry transitions, with no allocations or locks inside the sample callback.
- Corrects footer preset index after factory preset restoration, rename and undo/redo; clicking preset arrows now synchronizes first.
- Adds clear bend-handle hover feedback without adding controls or changing the 441×263 design.
- Expands DSP tests to measure DC removal and mid-band preservation at 44.1/48/96/192 kHz.
- Keeps existing plugin code, five parameter identifiers, 36 presets and existing CMake/GitHub Actions packaging.


## v1.11.0 — Capture safety and UI polish (November 2026)
- Keeps dry audio audible until the requested loop has been fully recorded; starts a two-millisecond wet fade only after capture is complete.
- Resets the read head to verified recorded history on initial capture readiness; safely handles loop-length increases that need more history.
- Avoids unnecessary interpolation when output is entirely dry or capture is incomplete.
- Improves envelope editor graphics: selection crosshairs, clearer bend grips, a playhead flag, and explicit snap guidance.
- Tessellates only visible curve segments at high zoom for better editor responsiveness.
- Fine-tunes low-contrast outlines and the knob hub while keeping the existing Skelly-green branding.
- Adds capture-geometry regression coverage; maintains existing five automatable parameter identifiers, 36 presets and v1.10 transport behavior.


## v1.10.0 — Timing, editing and speed refinement (November 2026)
- Aligns tempo-synced envelope phase to the host PPQ on transport starts and seeks, falling back to the first node when PPQ is absent.
- Matches curve snapping and arrow-key increments to the selected loop length (1/16 beat; finer nudge with Shift).
- Adds 0.4 ms sample-rate-independent speed slewing to reduce rapid-edit zipper artifacts.
- Restores preset names alongside curve nodes during Undo and Redo.
- Removes per-sample modulo from the recording write-head advance.
- Adds a fifth regression suite for PPQ/grid behavior and DSP speed-slew tests across sample rates.
- Preserves the five parameter IDs, plugin identity, 36 presets and Skelly neon visual styling.


## v1.9.0 — Skelly Neon Pro refinement (November 2026)

### DSP and playback
- Scratch Amount now transitions from exact dry input at 0% to processed scratching at 100%; Stop automation mutes both paths.
- A short capture fade prevents abruptly switching to potentially empty recording history at startup.
- Hermite interpolation with bounded output and non-finite input rejection is now in tested, JUCE-independent code.
- Monotonic cursor avoids repeated binary searches for segment evaluation on the audio thread.
- Ring read-head advances use fast bounded wrapping in normal sample-by-sample operation.
- Preserved 2 ms dual-head transitions, clip/tempo synchronization, and overwrite safeguards.
- Reclaims retired curve snapshots on the non-real-time editor timer.
- Added symmetric mono and stereo channel-layout support and more defensive plugin-state parsing, and synchronized preset-name access across UI/state threads.

### UI
- Displays actual direction and effective playback speed (FWD/REV/HOLD), STOP state, capture progress, and selected-point details.
- Fixed stale curve display when a host restores plugin state without moving the playhead.
- Musical grid reflects loop-length choice and zoom level.
- Scratch knob supports wheel, arrow keys, fine movement with Shift, and Home/End.
- Animated Scratch dial with continuous six-wedge fill and a fractional marker, plus polished pointer behavior and native font styling; retained the 441x263 fixed-aspect design and Skelly neon-green theme.

### Build and tests
- Four offline C++ suites (curve math, head safety, all 36 presets, and DSP regression).
- GitHub Release tests explicitly undefine NDEBUG so assert-based checks genuinely run.
- The Actions artifact contains a VST3 directory and Standalone .exe; no nested ZIP.
- Removes the externally embedded font dependency; original vector UI remains.

**Local validation:** GCC 14.2 C++17 tests passed in Release with assertions on; Clang AddressSanitizer/UndefinedBehaviorSanitizer tests passed. JUCE/Windows/FL Studio compilation and audio listening tests are not verified locally.

## v1.15.0 — Processor-level test coverage and UI interaction (November 2026)
- Adds a real JUCE integration executable, built from the actual processor/editor code, to run `processBlock()` with known stereo input and verify exact 0% dry passthrough, the immutable locked loop, Stop automation, 4x playback safety, non-finite input sanitization, and reported latency.
- Renders the actual JUCE editor to PNG at 441×263, 882×526 and 1323×789 on the Windows CI runner, and checks that the images contain the expected neon-green UI. Preview PNGs are packaged with the VST3 and Standalone artifact.
- Adds an instant envelope fit-to-window shortcut (`F` / Ctrl+0 / double-click empty graph space) and `Esc` to deselect a point.
- In locked mode the playback readout displays current forward/reverse speed or Hold, while the Loop Lock button continues to show capture/held status.
- Keeps all 36 presets, plugin identity, and existing parameter IDs unchanged. Real listening, FL Studio runtime, and end-user interaction validation remain separate release gates.

## v1.15.1 — Render-reviewed default-curve polish (November 2026)
- Replaces only the fresh-instance default speed envelope with a balanced eight-point Skelly Starter pattern, leaving all factory presets and saved session curves intact.
- Changes the new-instance display from "CAPTURE 0%" to "AWAIT AUDIO" until a real audio block reaches the processor; once audio arrives, capture percentage works as before.
- Verified against actual JUCE editor renders and the processor-level smoke suite on Windows.
