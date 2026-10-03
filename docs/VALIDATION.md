# Validation status — 0.1.0

## Verified locally

- The portable engine compiles with GCC 13.3 using C++17 and compiler warnings enabled.
- All eight behavioral test groups pass in the optimized build.
- The same test groups pass with AddressSanitizer and UndefinedBehaviorSanitizer. Leak detection is disabled for this local run because the preparation environment blocks LeakSanitizer's process inspection; the CI sanitizer command retains its defaults.
- The audio callback and pattern changes allocate no memory in the allocation probe.
- The A/B renderer produces a valid mono 44.1 kHz PCM WAV containing four dry bars and four flipped bars.

## Requires macOS validation

The AU wrapper, custom editor, and universal macOS bundles have **not** been compiled or run in this preparation environment. The included GitHub Actions macOS job is configured to build them and run `auval`; that workflow has not been executed yet.

After a successful AU build, check in Logic:

1. Insert on mono and stereo drum tracks. The initial insert should leave the audio unchanged.
2. Play a drum loop, wait for capture, and press FLIP. A new pattern should start on the next grid cell.
3. Loop a one-bar region for several repetitions. The effect should stay active across loop boundaries.
4. Turn Amount or Mix down to zero and compare with the original track. Allow the mix ramp/grid boundary to settle.
5. Stop, seek, and change tempo. The plugin should capture fresh input and resume after an aligned bar.
6. Save, close, and reopen the project. The controls and pattern seed should return to their saved values.
7. Automate Pattern Seed and Glitch Enabled, then bounce the track. Confirm that the result matches playback.

Development artifacts have no Developer ID signature, notarization, or installer. Treat 0.1.0 as a working DSP prototype with host integration still to validate.
