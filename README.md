# Communicake

Communicake is a gesture-controlled desktop application written in C++ with
OpenCV. It turns any webcam into a hands-free remote control: track your hand
in real time, count your raised fingers, and launch applications with nothing
more than a gesture.

## Introduction

Communicake lets you control your computer the way people actually
communicate: with your hands. Whether you are presenting, multitasking, or
just experimenting, the project makes computing feel more direct, and a lot
more fun.

## Features

What makes Communicake stand out:

- **Real-time hand tracking**: implemented entirely with classical computer
  vision (convex hull and centroid analysis). No neural networks, no GPUs,
  just OpenCV.
- **Fingertip detection**: `findFingertips()` uses a distance-from-centroid
  heuristic to identify the tips of each raised finger, even in less-than-
  ideal lighting.
- **Two tracking modes**: toggle between binary mode (background subtraction
  with Gaussian blur) and HSV mode (skin-color segmentation with adjustable
  H/S/V trackbars) using the `m` key. No need to settle for one.
- **Adjustable ROI**: fine-tune exactly which part of the frame is analyzed
  with the X, Y, WIDTH, and HEIGHT trackbars in the "ROI Controls" window.
- **Gesture-based app launching**: count your fingers to launch applications,
  a bridge between vision and action.
- **Adaptive background model**: press `b` to re-capture the background and
  adapt to a changing environment.
- **Live visual feedback**: the detected finger count is drawn on the frame,
  so you always know what the tracker sees.

## How it works

The pipeline is simpler than it looks:

1. Each frame is captured from the webcam and flipped for a natural mirror
   view.
2. Processing happens inside a region of interest (ROI) whose size and
   position you control live, via trackbars.
3. The hand is segmented from the background using either background
   subtraction (binary mode) or skin-color filtering (HSV mode).
4. The hand contour is extracted and analyzed with a convex hull to derive
   the centroid and the hand's shape.
5. Hull points that sit far enough from the centroid (and high enough in the
   frame) are classified as fingertips.
6. The number of raised fingers drives actions, such as launching
   applications, turning your hand into a remote control.

Everything runs in real time with only OpenCV as a dependency, which also
makes the project a good starting point if you are learning computer vision.

## Getting started

### Prerequisites

- A computer running Linux, Windows, or macOS
- A webcam
- OpenCV (4.x recommended) with C++ development headers
- A C++11 (or newer) compiler

### Build

From the repository root:

```bash
g++ -std=c++11 HandTracker/main.cpp HandTracker/HandTracker.cpp \
    HandTracker/Utilities.cpp -o communicake \
    `pkg-config --cflags --libs opencv4`
```

### Run

```bash
./communicake
```

Point your hand at the camera and start counting.

## Controls

| Key | Action |
|-----|--------|
| `m` | Toggle between binary and HSV modes |
| `b` | Re-capture the background model |
| `ESC` | Quit |

## Project structure

```
HandTracker/
  HandTracker.hpp    HandTracker class header (ROI, modes, fingertip detection)
  HandTracker.cpp    Implementation of the tracking pipeline
  Utilities.cpp      Drawing, gesture, and application-launch helpers
  Utilities.hpp      Utility declarations
  main.cpp           Webcam capture, trackbars, main loop
```

## Roadmap

- A full gesture vocabulary (pinch, swipe, point, and more)
- Multi-hand support
- A cross-platform installer
- A configuration panel instead of raw trackbars

## Conclusion

Communicake shows that you do not need a neural network to build something
useful. With classical computer vision and a handful of OpenCV primitives,
your hand becomes a real, working input device.

If you find a bug or have an idea for an improvement, open an issue or send a
pull request.