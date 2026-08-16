# Troubleshooting

Common failures and what to check first.

## The camera will not open

```
error: cannot open camera 0
```

- Another application is already using the camera. Close video-call and
  recording tools, then retry.
- The index is wrong. Try `--camera 1` or `--camera 2`.
- OpenCV picked a backend your camera does not support. Force one:

  ```bash
  communicake --backend dshow     # Windows: DirectShow
  communicake --backend msmf      # Windows: Media Foundation
  communicake --backend v4l2      # Linux
  ```

- On Linux, make sure your user may read `/dev/video*` (`video` group).

## The mask is black in binary mode

Binary segmentation subtracts a background model. The first frame of a run is
captured as that model, so:

- Start with an empty scene; a hand already in frame becomes part of the
  background.
- Press `b` to capture a new background when the scene changes.
- If the whole mask is white, the hand is darker or lighter than the
  background by more than `vision.binaryThreshold` everywhere; raise the
  threshold.

In HSV mode the mask is an `inRange` window, so tune `hsvMin`/`hsvMax`
instead.

## A gesture never fires

- `requiredFrames` (default 5) consecutive frames must agree. Lower it for
  high-latency cameras.
- The confidence floor rejects weak contours. Check with `--verbose`:

  ```
  [   42] rejected  confidence=0.31 < 0.35
  ```

  Lower `vision.minConfidence` or improve the lighting.
- The hand touches the ROI border. A clipped hand is discarded on purpose;
  move the ROI or your hand so the whole hand is inside it.
- The gesture is not bound in the config. Every fired gesture prints its
  resolved command.

## A gesture fires twice

`cooldownMs` (default 750) is the minimum gap between two fired gestures. Holding
a gesture fires once; lowering the cooldown below the time it takes to change
hands lets the same gesture fire again.

## Actions do nothing on Windows

Commands run through the platform shell (`std::system`). Quoting rules differ
from `cmd.exe`:

- `start "" https://example.com` needs the empty title argument.
- Prefer absolute paths for programs that are not on `PATH`.
- Use `--dry-run` to see exactly what would be launched:

  ```bash
  communicake --dry-run
  ```

The exit code of a failed action is reported on `stderr` and counted in the
final summary.

## `libopencv_highgui` fails to load (Windows)

The MSYS2 UCRT64 build of OpenCV's highgui depends on Qt6. Install it into the
same prefix:

```bash
pacman -S mingw-w64-ucrt-x86_64-qt6-base
```

If a test executable exits with `0xc0000135` (`STATUS_DLL_NOT_FOUND`), put the
prefix's `bin` directory on `PATH` before running it.

## CMake cannot find OpenCV

```
Could not find a package configuration file provided by "OpenCV"
```

- Linux: `sudo apt install libopencv-dev`
- MSYS2: `pacman -S mingw-w64-ucrt-x86_64-opencv` and configure with the
  `MinGW Makefiles` generator from an MSYS2 shell.
- Windows with vcpkg: pass
  `-DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake`.

An MSVC-built OpenCV cannot be linked by MinGW (and vice versa); use the same
toolchain for both.

## An image sequence plays out of order

Frames are ordered by the numeric parts of their names, so `frame_2.png` comes
before `frame_10.png`. If files are still in the wrong order, their names do not
share a numeric pattern; rename them to `frame_0001.png`, `frame_0002.png`, …
