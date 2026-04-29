#pragma once

#include "communicake/config.hpp"

#include <optional>
#include <string>

namespace communicake {

/// The tuning half of a configuration: vision parameters plus the stabiliser
/// thresholds. Written by the UI's save key so a session's settings survive a
/// restart, without duplicating the action bindings.
struct Profile {
    VisionParams vision;
    StabilizerConfig stabilizer;

    static Profile fromConfig(const AppConfig& config);
    AppConfig applyTo(AppConfig config) const;

    json::Value toJson() const;
    static std::optional<Profile> fromJson(const json::Value& root,
                                           std::string* error = nullptr);

    bool save(const std::string& path, std::string* error = nullptr) const;
    static std::optional<Profile> load(const std::string& path, std::string* error = nullptr);
};

} // namespace communicake
