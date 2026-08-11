#include "communicake/cli.hpp"

#include <cstdlib>
#include <string>

namespace communicake {
namespace {

bool parseInt(const std::string& text, int& value) {
    if (text.empty()) {
        return false;
    }
    char* end = nullptr;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0') {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

} // namespace

std::string versionString() {
    return "Communicake 5.0.0";
}

std::string usage() {
    return versionString() + R"( - communication made a piece of cake

Usage:
  communicake [options]

Options:
  --camera INDEX     Camera to open (default 0)
  --input PATH       Video file, or a directory of image frames
  --config PATH      JSON configuration (default: config/actions.json)
  --profile PATH     JSON profile with tuned vision settings
  --headless         Run without windows; print gestures as they fire
  --dry-run          Print the action instead of launching it
  --realtime         Pace file playback to the source frame rate
  --dump-mask DIR    Write the segmentation mask of every frame to DIR
  --log PATH         Write one CSV row per processed frame to PATH
  --benchmark FRAMES Process FRAMES frames without windows or actions and
                     report the frame timings
  --width PIXELS     Requested capture width
  --height PIXELS    Requested capture height
  -v, --verbose      Print an observation for every frame
  -h, --help         Show this help
  --version          Print the version
)";
}

std::optional<Options> parseArgs(const std::vector<std::string>& arguments, std::string* error) {
    Options options;

    auto fail = [&](const std::string& message) -> std::optional<Options> {
        if (error != nullptr) {
            *error = message;
        }
        return std::nullopt;
    };

    for (std::size_t i = 0; i < arguments.size(); ++i) {
        std::string argument = arguments[i];
        std::string inlineValue;
        bool hasInlineValue = false;

        const std::string::size_type equals = argument.find('=');
        if (argument.rfind("--", 0) == 0 && equals != std::string::npos) {
            hasInlineValue = true;
            inlineValue = argument.substr(equals + 1);
            argument = argument.substr(0, equals);
        }

        auto takeValue = [&](std::string& target) -> bool {
            if (hasInlineValue) {
                target = inlineValue;
                return true;
            }
            if (i + 1 >= arguments.size()) {
                return false;
            }
            target = arguments[++i];
            return true;
        };

        if (argument == "-h" || argument == "--help") {
            options.help = true;
        } else if (argument == "--version") {
            options.version = true;
        } else if (argument == "--headless") {
            options.headless = true;
        } else if (argument == "--dry-run") {
            options.dryRun = true;
        } else if (argument == "--realtime") {
            options.realtime = true;
        } else if (argument == "-v" || argument == "--verbose") {
            options.verbose = true;
        } else if (argument == "--camera" || argument == "--width" || argument == "--height" ||
                   argument == "--benchmark") {
            std::string value;
            if (!takeValue(value)) {
                return fail(argument + ": missing value");
            }
            int number = 0;
            if (!parseInt(value, number) || number < 0) {
                return fail(argument + ": expected a non-negative whole number, got \"" + value +
                            "\"");
            }
            if (argument == "--benchmark" && number == 0) {
                return fail("--benchmark: expected at least one frame");
            }
            if (argument == "--camera") {
                options.camera = number;
            } else if (argument == "--width") {
                options.width = number;
            } else if (argument == "--height") {
                options.height = number;
            } else {
                options.benchmark = number;
            }
        } else if (argument == "--input" || argument == "--config" || argument == "--profile" ||
                   argument == "--dump-mask" || argument == "--log") {
            std::string value;
            if (!takeValue(value)) {
                return fail(argument + ": missing value");
            }
            if (argument == "--input") {
                options.input = value;
            } else if (argument == "--config") {
                options.configPath = value;
            } else if (argument == "--profile") {
                options.profilePath = value;
            } else if (argument == "--log") {
                options.logPath = value;
            } else {
                options.dumpMaskDir = value;
            }
        } else {
            return fail("unknown option \"" + argument + "\"");
        }
    }
    return options;
}

std::optional<Options> parseArgs(int argc, char** argv, std::string* error) {
    std::vector<std::string> arguments;
    for (int i = 1; i < argc; ++i) {
        arguments.emplace_back(argv[i]);
    }
    return parseArgs(arguments, error);
}

} // namespace communicake
