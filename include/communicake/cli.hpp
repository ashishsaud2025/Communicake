#pragma once

#include <optional>
#include <string>
#include <vector>

namespace communicake {

/// Everything the command line can change.
struct Options {
    int camera = 0;
    std::string input;      ///< video file or image directory; empty means camera
    std::string configPath; ///< empty means "config/actions.json when it exists"
    std::string profilePath;
    std::string dumpMaskDir;
    std::string logPath; ///< CSV of per-frame observations; empty means no log
    std::string cameraBackend = "any"; ///< any, dshow, msmf or v4l2
    int width = 0;
    int height = 0;
    int benchmark = 0; ///< process this many frames, then report timings and exit
    bool headless = false;
    bool dryRun = false;
    bool realtime = false; ///< pace file playback to the source frame rate
    bool mirror = true; ///< flip webcam frames left-to-right, like a mirror
    bool verbose = false;
    bool help = false;
    bool version = false;
};

/// Parse arguments without the program name. Returns nullopt and fills `error`
/// for an unknown option, a missing value or a value that is not a number.
std::optional<Options> parseArgs(const std::vector<std::string>& arguments,
                                 std::string* error = nullptr);

std::optional<Options> parseArgs(int argc, char** argv, std::string* error = nullptr);

std::string usage();
std::string versionString();

} // namespace communicake
