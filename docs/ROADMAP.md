# Roadmap

## Next

- **A calibration wizard.** Press `k`, hold an empty scene and then a hand, and
  derive the threshold, ROI and HSV bounds instead of tuning six trackbars by
  hand. The pieces exist: `Segmenter::setBackground` and `Profile`.
- **Gesture sequences.** A swipe or a pinch-and-hold is a sequence of
  observations, not a single frame count. `GestureStabilizer` is the place to
  grow a small state machine.
- **Confidence-driven actions.** `HandObservation::confidence` is computed and
  displayed but not used. Ignoring low-confidence gestures would cut false
  triggers in poor light.
- **A headless benchmark.** Run a recorded clip through the pipeline and report
  frames per second and per-stage timings, so optimisation has a number to move.

## Later

- **Two hands.** The pipeline currently keeps only the largest contour. Tracking
  two would mean keying actions by hand identity.
- **A configuration GUI.** The trackbars are serviceable but a small window with
  labelled fields and a live mask preview would be friendlier.
- **Packaging.** A Windows installer and a macOS app bundle, so the project does
  not require a compiler to try.

## Not planned

- **Neural hand pose estimation.** MediaPipe, and similar, would be more accurate
  but would replace the classical-CV pipeline that makes this project
  interesting, and add a large dependency. If it happens, it belongs behind a
  separate `FrameAnalyser` implementation so the two can be compared.
- **Remote control over a network.** Out of scope, and a security surface the
  project does not need.
