# Configuration

Communicake reads a JSON file. `--config PATH` names it explicitly; otherwise
`config/actions.json` is used when it exists. Anything the file leaves out keeps
the built-in default.

```bash
communicake --config config/actions.json
communicake --dry-run           # print actions instead of launching them
```

## `vision`

| Key | Type | Range | Default | Meaning |
| --- | --- | --- | --- | --- |
| `mode` | string | `"binary"`, `"hsv"` | `"binary"` | How the hand is separated from the background |
| `binaryThreshold` | number | 0–255 | 20 | Grey-level difference that counts as the hand in binary mode |
| `minContourArea` | number | 1–1e8 | 1200 | Contours smaller than this are ignored |
| `fingertipDistance` | number | 1–5000 | 55 | Minimum distance from the centroid, in pixels |
| `minFingerGap` | number | 1–5000 | 34 | Two fingertips closer than this are the same finger |
| `defectDepth` | number | 0–1000 | 18 | Valley depth that separates two fingers |
| `blurKernel` | number | 3–31, odd | 5 | Gaussian kernel used before subtraction |
| `roi` | array | `[x, y, w, h]` | `[10, 10, 300, 300]` | Region analysed each frame |
| `hsvMin` | array | three 0–255 | `[0, 60, 60]` | Lower bound for HSV mode |
| `hsvMax` | array | three 0–255 | `[25, 255, 255]` | Upper bound for HSV mode |

The ROI is clamped to the frame, so a size larger than the camera cannot crash
the pipeline.

## `stabilizer`

| Key | Type | Range | Default | Meaning |
| --- | --- | --- | --- | --- |
| `requiredFrames` | number | 1–1000 | 5 | Consecutive frames a gesture must hold before it fires |
| `cooldownMs` | number | 0–600000 | 750 | Minimum gap between two different fired gestures |

A held gesture fires once, not once per frame.

## `actions`

Map a gesture name to a command. `fist` is zero fingers; `one` to `five` are the
finger counts. `none` is accepted but never fires.

Two forms are supported:

```json
{
  "actions": {
    "two": "notepad",
    "three": {
      "command": "xdg-open https://github.com",
      "windows": "start \"\" https://github.com",
      "macos": "open https://github.com"
    }
  }
}
```

The string form is shorthand for `{"command": "..."}`. In the object form,
`command` is the fallback and `windows`, `linux`, `macos` override it on that
platform. A gesture with an empty command does nothing.

`{platform}` in a command is replaced with `windows`, `linux`, `macos` or
`unknown`.

Commands are executed with `std::system`, so they run through the platform shell.
Keep that in mind before binding a destructive command to a gesture.

## `dryRun`

`true` prints each resolved command instead of launching it. `--dry-run` on the
command line does the same for one run.

## Profiles

Tuning done with the trackbars is not written back to the config automatically.
Press `s` in the interactive view to save the current vision parameters and
stabiliser thresholds to `--profile PATH`, or to `profiles/communicake.json` when
no path was given. Load one with:

```bash
communicake --profile profiles/communicake.json
```

A profile contains only `vision` and `stabilizer`; action bindings stay in the
config file.

## Errors

A malformed file reports a line and column:

```
config/actions.json: line 4, column 12: expected ':' after key "roi"
```

An out-of-range value reports the key:

```
config/actions.json: vision.binaryThreshold: must be between 0 and 255
```

Both exit with status 2.
