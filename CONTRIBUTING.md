# Contributing

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

On Windows with MSYS2, use the MinGW preset:

```bash
cmake --preset mingw
cmake --build --preset mingw
```

OpenCV 4 is the only dependency. On MSYS2 the `mingw-w64-ucrt-x86_64-opencv`
package pulls in `qt6-base` transitively for the highgui windows; if `ctest`
reports `0xc0000135`, install `mingw-w64-ucrt-x86_64-qt6-base`.

## Tests

```bash
ctest --test-dir build --output-on-failure
```

A new behaviour needs a test in `tests/`. The vision tests use generated frames
(`tests/synthetic.hpp`) rather than a webcam, so they run on a build server with
no camera and no display.

## Style

- C++17, four spaces, 100 columns. `.clang-format` is authoritative:
  `clang-format -i src/*.cpp include/communicake/*.hpp tests/*.cpp`.
- No dependency beyond OpenCV and the standard library. A feature that needs one
  needs a discussion first.
- The core library must stay free of windows, cameras and shell calls. If it
  needs one, it belongs in `src/app.cpp` or `src/ui.cpp`.

## Commits

Conventional Commits: `feat:`, `fix:`, `test:`, `refactor:`, `build:`, `ci:`,
`docs:`, `chore:`, `perf:`, `style:`. Imperative mood, under 72 characters.
One coherent change per commit, and the tree builds at every commit.

## Pull requests

- Describe what changed and why, not just what files moved.
- Include the new test that would fail without the change.
- Update `docs/CONFIG.md` when a configuration key is added or its range changes.
