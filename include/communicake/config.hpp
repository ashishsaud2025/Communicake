#pragma once

#include "communicake/gesture.hpp"
#include "communicake/json.hpp"
#include "communicake/vision.hpp"

#include <map>
#include <optional>
#include <string>

namespace communicake {

/// One gesture's command. `command` is the fallback; `platforms` holds
/// per-operating-system overrides keyed by "windows", "linux" or "macos".
struct ActionBinding {
    std::string command;
    std::map<std::string, std::string> platforms;

    std::string forPlatform(const std::string& platform) const;
    bool empty() const;
};

/// Everything the application needs that is not compiled in.
struct AppConfig {
    VisionParams vision;
    StabilizerConfig stabilizer;
    std::map<Gesture, ActionBinding> actions;
    bool dryRun = false;

    /// A working configuration when no file is supplied.
    static AppConfig defaults();

    /// Overlay a parsed document on top of the defaults. Unknown keys are
    /// ignored; unknown gesture names and out-of-range values are errors.
    static std::optional<AppConfig> fromJson(const json::Value& root, std::string* error = nullptr);

    json::Value toJson() const;

    static std::optional<AppConfig> load(const std::string& path, std::string* error = nullptr);
    bool save(const std::string& path, std::string* error = nullptr) const;
};

/// "windows", "linux", "macos" or "unknown".
std::string currentPlatform();

} // namespace communicake
