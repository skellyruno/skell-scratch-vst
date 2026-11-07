# ScratchPad changelog

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
