# Architecture

Communicake is split into a core library and a thin application shell. The core
knows nothing about cameras, windows or the operating system; that is what makes
it testable in CI and what keeps the demo from turning into a pile of
`cv::imshow` calls tangled with `system()`.

## Layers

```
communicake_core            (static library, no UI, no OS calls)
  types.hpp                 HandObservation, SegmentationMode
  geometry.hpp              centroid, contour selection, rectangle clamping
  vision.hpp                Segmenter, HandPipeline, fingertip detection
  gesture.hpp               Gesture, GestureStabilizer
  json.hpp                  small JSON reader and writer
  config.hpp                AppConfig: vision, stabilizer and action bindings
  action.hpp                ActionRunner: resolve and launch a command
  frame_source.hpp          webcam / video file / image sequence
  app.hpp                   wiring, interactive and headless modes

communicake                 (executable)
  main.cpp                  argument handling and start-up
  ui.cpp/hpp                trackbars, windows and overlays
```

## Frame pipeline

```
FrameSource::next(frame)
        │
        ▼
clampRect(params.roi, frame.size())      ROI is never allowed outside the frame
        │
        ▼
Segmenter::segment(roiView)              binary: |gray - background| > threshold
        │                                hsv:    inRange(h, s, v)
        ▼
morphology: open (drop speckle) → close (fill pinholes)
        │
        ▼
findContours → largest contour above minContourArea
        │
        ▼
convexHull + convexityDefects
        │
        ▼
HandObservation { valid, fingerCount, confidence, centroid, fingertips, hull, area }
        │
        ▼
GestureStabilizer::update(gestureFromFingerCount(obs.fingerCount))
        │  fires at most once per hold, not more often than the cooldown
        ▼
ActionRunner::run(config.resolveAction(gesture, platform))
```

## Why convexity defects

`findFingertips` does not guess from distance alone. A fist is a smooth blob, and
its topmost point is "far from the centroid" too, so distance by itself produces
false fingers. `cv::convexityDefects` gives the valleys between fingers: a hull
vertex that borders a valley deeper than `defectDepth` is a fingertip candidate.
Distance from the centroid and a minimum gap between accepted points then filter
the candidates, and at most five survive.

A single extended finger can leave only a shallow valley, so the detector falls
back to a prominence test: if one hull point is at least 1.55× farther from the
centroid than the median hull point, it counts as a fingertip. A round blob fails
that ratio, which keeps a fist at zero.

## Why the stabiliser exists

Raw per-frame counting flickers. One dropped frame in the middle of "three
fingers" would otherwise read as a different gesture, and holding a hand up would
launch the same application on every frame. `GestureStabilizer` requires the same
gesture for `requiredFrames` consecutive frames, fires once per hold, and refuses
to fire again until the hand has changed and `cooldownMs` has elapsed.

The clock is injected (`std::function<long long()>`), so the tests exercise
timing without sleeping.

## Why the config is JSON

Actions used to be five hardcoded `system()` calls. They are now a mapping in
`config/actions.json`, with per-platform overrides:

```json
{
  "three": { "command": "code", "windows": "start chrome", "macos": "open -a Safari" }
}
```

The project deliberately avoids a JSON dependency, so `json.cpp` implements the
subset the config needs: objects, arrays, strings with escapes, numbers, booleans
and null, with line and column numbers in error messages.

## Coordinates

`HandObservation` points are ROI-local: (0, 0) is the top-left corner of
`params.roi`, not of the camera frame. The UI adds `params.roi.tl()` when it
draws. This keeps the core independent of where the ROI happens to sit.

## Testing

The tests build synthetic frames (`tests/synthetic.hpp`): a palm with zero to
five fingers, a smooth blob, and speckle. Because the core takes `cv::Mat` and
returns values, the whole pipeline — segmentation included — runs in CI without a
webcam. `test_fingertips` asserts the finger count for every hand shape;
`test_geometry` pins the contour and rectangle helpers.
