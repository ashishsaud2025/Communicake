# Changelog

All notable changes to Communike are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses
[semantic versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

Planned as 5.1.0. Everything below is on `main` and passes the test suite.

### Added

- `--benchmark FRAMES` reports mean, p50, p95 and maximum milliseconds for
  segmentation, analysis and the whole frame, after three warm-up frames. It
  implies `--headless` and `--dry-run`.
- `--log PATH` writes a CSV row per frame (frame, hand, fingers, confidence,
  area, gesture, command) for tuning against real recordings.
- `--realtime` paces file playback to the file's frame rate instead of reading
  as fast as the disk allows.
- `--backend NAME` selects the camera backend (`any`, `dshow`, `msmf`, `v4l2`).
- `vision.morphKernel` makes the morphology kernel size configurable, range
  checked to 3–31.
- `vision.minConfidence` drops weak observations before the gesture stabiliser.
- Stage timings and the confidence/area of the current hand are drawn in the
  interactive overlay.
- A deterministic fingertip order (clockwise around the palm), so the reported
  points no longer depend on contour traversal.
- A golden-coverage test pins the segmentation output; property tests cover
  `clampRect`, and the JSON parser is exercised on every truncated prefix.
- CI jobs for Linux, Windows (vcpkg, MSVC) and Linux under AddressSanitizer and
  UndefinedBehaviorSanitizer.
- `docs/TROUBLESHOOTING.md`, covering camera backends, segmentation, gesture
  timing, shell quoting and OpenCV linkage.

### Changed

- Image-sequence frames are ordered by the numeric parts of their names, so
  `frame_2.png` precedes `frame_10.png`.
- Segmentation reuses its buffers instead of allocating a mask per frame.
- The version string is generated from the CMake `project()` declaration
  instead of being written out by hand.

### Fixed

- A quiet action failure is now reported with its exit code and counted in the
  run summary.
- A hand clipped by the ROI border is rejected instead of being reported with a
  meaningless fingertip count.
- The JSON parser rejects duplicate object keys and unescaped control
  characters in strings, both of which previously led to silently different
  configuration than other JSON readers would produce.

## [5.0.0]

The rewrite that replaced the original single-file demo:

- A vision core independent of the camera, driven by synthetic frames in tests.
- Gesture-to-action bindings read from `config/actions.json`, with per-platform
  commands and a dry-run mode, instead of hardcoded `system()` calls.
- Gesture consensus, hold latching and a cooldown, replacing the per-frame
  trigger.
- Headless mode, mask dumping, profiles, a CLI, and a test suite that runs
  without a camera.
