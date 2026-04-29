#include "communicake/profile.hpp"

#include <fstream>

namespace communicake {

Profile Profile::fromConfig(const AppConfig& config) {
    Profile profile;
    profile.vision = config.vision;
    profile.stabilizer = config.stabilizer;
    return profile;
}

AppConfig Profile::applyTo(AppConfig config) const {
    config.vision = vision;
    config.stabilizer = stabilizer;
    return config;
}

json::Value Profile::toJson() const {
    AppConfig temporary;
    temporary.vision = vision;
    temporary.stabilizer = stabilizer;
    const json::Value full = temporary.toJson();
    json::Value root;
    root.set("vision", full["vision"]);
    root.set("stabilizer", full["stabilizer"]);
    return root;
}

std::optional<Profile> Profile::fromJson(const json::Value& root, std::string* error) {
    const auto parsed = AppConfig::fromJson(root, error);
    if (!parsed.has_value()) {
        return std::nullopt;
    }
    return fromConfig(*parsed);
}

bool Profile::save(const std::string& path, std::string* error) const {
    std::ofstream file(path);
    if (!file) {
        if (error != nullptr) {
            *error = "cannot write " + path;
        }
        return false;
    }
    file << json::dump(toJson()) << "\n";
    if (!file) {
        if (error != nullptr) {
            *error = "failed while writing " + path;
        }
        return false;
    }
    return true;
}

std::optional<Profile> Profile::load(const std::string& path, std::string* error) {
    const auto config = AppConfig::load(path, error);
    if (!config.has_value()) {
        return std::nullopt;
    }
    return fromConfig(*config);
}

} // namespace communicake
