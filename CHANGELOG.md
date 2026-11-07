# ScratchPad changelog

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
