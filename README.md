# Communicake

Communicake turns a webcam into a hands-free remote control. It tracks your hand
with classical computer vision, counts the fingers you raise, and launches
whatever you bind to that gesture.

```
camera ──▶ ROI crop ──▶ segmentation ──▶ contours ──▶ convex hull + defects
                                                              │
                          application ◀── action ◀── stabiliser ◀── fingertips
```

No neural networks, no GPU: OpenCV's segmentation, contour, hull and convexity
primitives do the work, which also makes the whole pipeline testable on generated
frames.

## Requirements

- A webcam (or a video file / a directory of frames)
- OpenCV 4.x with C++ headers
- CMake 3.16+ and a C++17 compiler

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Windows with MSYS2:

```bash
cmake --preset mingw
cmake --build --preset mingw
```

The highgui windows need Qt6 on MSYS2: `pacman -S mingw-w64-ucrt-x86_64-qt6-base`.

## Run

```bash
./build/communicake
```

Stand in front of the camera, raise zero to five fingers, and the configured
application launches. Holding a gesture fires it once, not on every frame.

Try it without launching anything first:

```bash
./build/communicake --dry-run
```

## Options

| Option | Meaning |
| --- | --- |
| `--camera INDEX` | Camera to open (default `0`) |
| `--backend NAME` | Capture backend: `any`, `dshow`, `msmf` or `v4l2` |
| `--input PATH` | Video file, or a directory of image frames |
| `--config PATH` | JSON configuration (default: `config/actions.json` when present) |
| `--profile PATH` | Tuned vision settings saved with `s` |
| `--log PATH` | Write a CSV row per frame: frame, hand, fingers, confidence, area, gesture, command |
| `--dump-mask DIR` | Write the segmentation mask of every frame |
| `--width`, `--height` | Requested capture size |
| `--benchmark FRAMES` | Process FRAMES frames, report per-stage timings, exit |
| `--headless` | No windows; prints gestures as they fire |
| `--dry-run` | Print the action instead of launching it |
| `--realtime` | Pace file playback to the file's frame rate instead of running flat out |
| `--no-mirror` | Do not flip webcam frames left-to-right |
| `-v`, `--verbose` | Print an observation for every frame |
| `--version`, `-h` | Version, usage |

Headless over a recording is handy for checking a configuration without waving
at a camera:

```bash
./build/communicake --headless --input clip.mp4 --verbose
```

Two options exist for measuring rather than watching. `--benchmark` reports
mean, p50, p95 and maximum milliseconds for segmentation, analysis and the whole
frame after discarding three warm-up frames, and implies `--headless`:

```bash
./build/communicake --benchmark 300 --input clip.mp4
```

`--log` records one row per frame for later analysis, which is how thresholds
in `config/actions.json` are tuned against real recordings:

```bash
./build/communicake --headless --log observations.csv --input clip.mp4
```

## Interactive controls

| Key | Action |
| --- | --- |
| `m` | Toggle binary (background subtraction) and HSV (skin colour) modes |
| `b` | Re-capture the background model |
| `s` | Save the current tuning to a profile |
| `ESC` | Quit |

The trackbars set the segmentation threshold, the fingertip distance and the
region of interest. In HSV mode a second window exposes the six HSV bounds.

## Configuration

Gestures map to commands in JSON:

```json
{
  "actions": {
    "two": "notepad",
    "three": {
      "command": "xdg-open https://github.com",
      "windows": "start \"\" https://github.com"
    }
  }
}
```

Every field, its range and its default is documented in
[docs/CONFIG.md](docs/CONFIG.md).

## How it works

1. The frame is cropped to a region of interest that is clamped to the image.
2. The hand is segmented — background subtraction against a captured model, or
   an HSV skin-colour range.
3. Morphology removes speckle and fills pinholes; the largest contour that
   passes a minimum area becomes the hand.
4. Convexity defects find the valleys between fingers; hull vertices next to a
   deep valley, above the centroid, and far enough out are fingertip candidates.
   At most five survive.
5. A stabiliser requires the same finger count for several frames, fires once per
   hold, and enforces a cooldown.
6. The gesture's command runs, or is printed in dry-run mode.

More detail, including why convexity defects rather than distance alone, is in
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Project layout

```
include/communicake/   public headers: vision, gesture, config, json, action, cli
src/                   implementation; app.cpp and ui.cpp hold the only UI code
tests/                 synthetic hand fixtures and unit/integration tests
config/actions.json    default gesture map
docs/                  architecture, configuration reference, roadmap
```

## Tests

```bash
ctest --test-dir build --output-on-failure
```

The suite generates its own frames: a palm with zero to five fingers, a smooth
blob and speckle. No camera, no display, so CI runs the same tests as your
laptop. The integration test feeds a generated image sequence through the whole
application and asserts exactly one action fires.

## What changed in v5

- CMake build, CI and tests; C++17.
- The vision pipeline is a library with no camera, window or shell dependency.
- Fingertips are found from convexity defects instead of a distance heuristic.
- Gestures are debounced and rate-limited.
- Actions come from a JSON config with per-platform commands, not hardcoded
  `system()` calls.
- Headless mode, `--dump-mask`, profiles, `--dry-run`, and a proper CLI.
- Fixed: the ROI could exceed the frame, a degenerate contour divided by zero,
  more than five fingertips deleted only one point, and the HSV window could be
  read after it was destroyed.

## Roadmap

See [docs/ROADMAP.md](docs/ROADMAP.md). Contributions are welcome;
[CONTRIBUTING.md](CONTRIBUTING.md) covers the build, the tests and the style.
When something does not work, [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md)
starts with the usual causes. [CHANGELOG.md](CHANGELOG.md) lists what changed
in each release, and [docs/RELEASING.md](docs/RELEASING.md) is the checklist
used to cut one.

## License

MIT, see [LICENSE](LICENSE).
