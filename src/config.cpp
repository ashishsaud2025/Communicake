#include "communicake/config.hpp"

#include <fstream>
#include <sstream>

namespace communicake {
namespace {

bool readNumber(const json::Value& object, const std::string& key, const std::string& path,
                double minimum, double maximum, double& target, std::string* error) {
    if (!object.has(key)) {
        return true;
    }
    const json::Value& value = object[key];
    if (!value.isNumber()) {
        if (error != nullptr) {
            *error = path + "." + key + ": expected a number";
        }
        return false;
    }
    const double number = value.asNumber();
    if (number < minimum || number > maximum) {
        if (error != nullptr) {
            *error = path + "." + key + ": must be between " + std::to_string(minimum) + " and " +
                     std::to_string(maximum);
        }
        return false;
    }
    target = number;
    return true;
}

bool readInt(const json::Value& object, const std::string& key, const std::string& path,
             int minimum, int maximum, int& target, std::string* error) {
    double number = static_cast<double>(target);
    if (!readNumber(object, key, path, minimum, maximum, number, error)) {
        return false;
    }
    target = static_cast<int>(number + 0.5);
    return true;
}

bool readColor(const json::Value& object, const std::string& key, const std::string& path,
               cv::Scalar& target, std::string* error) {
    if (!object.has(key)) {
        return true;
    }
    const json::Value& value = object[key];
    if (!value.isArray() || value.size() != 3) {
        if (error != nullptr) {
            *error = path + "." + key + ": expected an array of three numbers";
        }
        return false;
    }
    for (std::size_t i = 0; i < 3; ++i) {
        if (!value[i].isNumber() || value[i].asNumber() < 0 || value[i].asNumber() > 255) {
            if (error != nullptr) {
                *error = path + "." + key + ": channels must be between 0 and 255";
            }
            return false;
        }
        target[static_cast<int>(i)] = value[i].asNumber();
    }
    return true;
}

bool readRoi(const json::Value& object, const std::string& key, const std::string& path,
             cv::Rect& target, std::string* error) {
    if (!object.has(key)) {
        return true;
    }
    const json::Value& value = object[key];
    if (!value.isArray() || value.size() != 4) {
        if (error != nullptr) {
            *error = path + "." + key + ": expected [x, y, width, height]";
        }
        return false;
    }
    int numbers[4] = {0, 0, 0, 0};
    for (std::size_t i = 0; i < 4; ++i) {
        if (!value[i].isNumber()) {
            if (error != nullptr) {
                *error = path + "." + key + ": expected [x, y, width, height]";
            }
            return false;
        }
        numbers[static_cast<int>(i)] = value[i].asInt();
    }
    if (numbers[2] < 1 || numbers[3] < 1) {
        if (error != nullptr) {
            *error = path + "." + key + ": width and height must be positive";
        }
        return false;
    }
    target = cv::Rect(numbers[0], numbers[1], numbers[2], numbers[3]);
    return true;
}

bool readSegmentationMode(const json::Value& object, const std::string& key, const std::string& path,
                          SegmentationMode& target, std::string* error) {
    if (!object.has(key)) {
        return true;
    }
    const std::string name = object[key].asString();
    if (name == "binary") {
        target = SegmentationMode::Binary;
        return true;
    }
    if (name == "hsv") {
        target = SegmentationMode::Hsv;
        return true;
    }
    if (error != nullptr) {
        *error = path + "." + key + ": expected \"binary\" or \"hsv\"";
    }
    return false;
}

bool readBinding(const std::string& gestureName, const json::Value& value, ActionBinding& target,
                 std::string* error) {
    if (value.isString()) {
        target.command = value.asString();
        return true;
    }
    if (!value.isObject()) {
        if (error != nullptr) {
            *error = "actions." + gestureName + ": expected a string or an object";
        }
        return false;
    }
    if (value.has("command")) {
        if (!value["command"].isString()) {
            if (error != nullptr) {
                *error = "actions." + gestureName + ".command: expected a string";
            }
            return false;
        }
        target.command = value["command"].asString();
    }
    for (const char* platform : {"windows", "linux", "macos"}) {
        if (!value.has(platform)) {
            continue;
        }
        if (!value[platform].isString()) {
            if (error != nullptr) {
                *error = "actions." + gestureName + "." + platform + ": expected a string";
            }
            return false;
        }
        target.platforms[platform] = value[platform].asString();
    }
    return true;
}

json::Value bindingToJson(const ActionBinding& binding) {
    json::Value object;
    if (!binding.command.empty()) {
        object.set("command", json::Value(binding.command));
    }
    for (const auto& entry : binding.platforms) {
        object.set(entry.first, json::Value(entry.second));
    }
    return object;
}

} // namespace

std::string ActionBinding::forPlatform(const std::string& platform) const {
    const auto it = platforms.find(platform);
    if (it != platforms.end()) {
        return it->second;
    }
    return command;
}

bool ActionBinding::empty() const {
    return command.empty() && platforms.empty();
}

std::string currentPlatform() {
#if defined(_WIN32)
    return "windows";
#elif defined(__APPLE__)
    return "macos";
#elif defined(__linux__)
    return "linux";
#else
    return "unknown";
#endif
}

AppConfig AppConfig::defaults() {
    AppConfig config;
    config.actions[Gesture::One] = ActionBinding{"code", {}};
    config.actions[Gesture::Two] = ActionBinding{"notepad", {{"linux", "gedit"}}};
    config.actions[Gesture::Three] =
        ActionBinding{"xdg-open https://github.com", {{"windows", "start \"\" https://github.com"}}};
    config.actions[Gesture::Four] =
        ActionBinding{"xterm", {{"windows", "calc"}, {"macos", "open -a Calculator"}}};
    config.actions[Gesture::Five] = ActionBinding{"xdg-open .", {{"windows", "explorer ."}}};
    return config;
}

std::optional<AppConfig> AppConfig::fromJson(const json::Value& root, std::string* error) {
    if (!root.isObject() && !root.isNull()) {
        if (error != nullptr) {
            *error = "root: expected an object";
        }
        return std::nullopt;
    }

    AppConfig config = defaults();

    if (root.has("vision")) {
        const json::Value& vision = root["vision"];
        if (!vision.isObject()) {
            if (error != nullptr) {
                *error = "vision: expected an object";
            }
            return std::nullopt;
        }
        if (!readSegmentationMode(vision, "mode", "vision", config.vision.mode, error) ||
            !readInt(vision, "binaryThreshold", "vision", 0, 255, config.vision.binaryThreshold,
                     error) ||
            !readNumber(vision, "minContourArea", "vision", 1.0, 1e8, config.vision.minContourArea,
                        error) ||
            !readInt(vision, "fingertipDistance", "vision", 1, 5000, config.vision.fingertipDistance,
                     error) ||
            !readInt(vision, "minFingerGap", "vision", 1, 5000, config.vision.minFingerGap, error) ||
            !readInt(vision, "defectDepth", "vision", 0, 1000, config.vision.defectDepth, error) ||
            !readInt(vision, "blurKernel", "vision", 3, 31, config.vision.blurKernel, error) ||
            !readNumber(vision, "minConfidence", "vision", 0.0, 1.0, config.vision.minConfidence,
                        error) ||
            !readRoi(vision, "roi", "vision", config.vision.roi, error) ||
            !readColor(vision, "hsvMin", "vision", config.vision.hsvMin, error) ||
            !readColor(vision, "hsvMax", "vision", config.vision.hsvMax, error)) {
            return std::nullopt;
        }
    }

    if (root.has("stabilizer")) {
        const json::Value& stabilizer = root["stabilizer"];
        if (!stabilizer.isObject()) {
            if (error != nullptr) {
                *error = "stabilizer: expected an object";
            }
            return std::nullopt;
        }
        if (!readInt(stabilizer, "requiredFrames", "stabilizer", 1, 1000,
                     config.stabilizer.requiredFrames, error) ||
            !readInt(stabilizer, "cooldownMs", "stabilizer", 0, 600000, config.stabilizer.cooldownMs,
                     error)) {
            return std::nullopt;
        }
    }

    if (root.has("actions")) {
        const json::Value& actions = root["actions"];
        if (!actions.isObject()) {
            if (error != nullptr) {
                *error = "actions: expected an object";
            }
            return std::nullopt;
        }
        for (const auto& entry : actions.members()) {
            const auto gesture = gestureFromName(entry.first);
            if (!gesture.has_value()) {
                if (error != nullptr) {
                    *error = "actions: unknown gesture \"" + entry.first + "\"";
                }
                return std::nullopt;
            }
            ActionBinding binding;
            if (!readBinding(entry.first, entry.second, binding, error)) {
                return std::nullopt;
            }
            config.actions[*gesture] = binding;
        }
    }

    if (root.has("dryRun")) {
        if (!root["dryRun"].isBool()) {
            if (error != nullptr) {
                *error = "dryRun: expected true or false";
            }
            return std::nullopt;
        }
        config.dryRun = root["dryRun"].asBool();
    }

    return config;
}

json::Value AppConfig::toJson() const {
    json::Value root;

    json::Value visionValue;
    visionValue.set("mode", json::Value(vision.mode == SegmentationMode::Hsv ? "hsv" : "binary"));
    visionValue.set("binaryThreshold", json::Value(vision.binaryThreshold));
    visionValue.set("minContourArea", json::Value(vision.minContourArea));
    visionValue.set("fingertipDistance", json::Value(vision.fingertipDistance));
    visionValue.set("minFingerGap", json::Value(vision.minFingerGap));
    visionValue.set("defectDepth", json::Value(vision.defectDepth));
    visionValue.set("blurKernel", json::Value(vision.blurKernel));
    visionValue.set("minConfidence", json::Value(vision.minConfidence));
    json::Value roi;
    roi.push(json::Value(vision.roi.x));
    roi.push(json::Value(vision.roi.y));
    roi.push(json::Value(vision.roi.width));
    roi.push(json::Value(vision.roi.height));
    visionValue.set("roi", roi);
    json::Value hsvMin;
    json::Value hsvMax;
    for (int i = 0; i < 3; ++i) {
        hsvMin.push(json::Value(static_cast<int>(vision.hsvMin[i])));
        hsvMax.push(json::Value(static_cast<int>(vision.hsvMax[i])));
    }
    visionValue.set("hsvMin", hsvMin);
    visionValue.set("hsvMax", hsvMax);
    root.set("vision", visionValue);

    json::Value stabilizerValue;
    stabilizerValue.set("requiredFrames", json::Value(stabilizer.requiredFrames));
    stabilizerValue.set("cooldownMs", json::Value(stabilizer.cooldownMs));
    root.set("stabilizer", stabilizerValue);

    json::Value actionValues;
    for (const auto& entry : actions) {
        actionValues.set(gestureName(entry.first), bindingToJson(entry.second));
    }
    root.set("actions", actionValues);

    root.set("dryRun", json::Value(dryRun));
    return root;
}

std::optional<AppConfig> AppConfig::load(const std::string& path, std::string* error) {
    std::ifstream file(path);
    if (!file) {
        if (error != nullptr) {
            *error = "cannot open " + path;
        }
        return std::nullopt;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();

    std::string parseError;
    const json::Value root = json::parse(buffer.str(), &parseError);
    if (!parseError.empty()) {
        if (error != nullptr) {
            *error = path + ": " + parseError;
        }
        return std::nullopt;
    }
    auto config = fromJson(root, error);
    if (!config.has_value() && error != nullptr) {
        *error = path + ": " + *error;
    }
    return config;
}

bool AppConfig::save(const std::string& path, std::string* error) const {
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

} // namespace communicake
