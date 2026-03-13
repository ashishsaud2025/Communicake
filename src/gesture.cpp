#include "communicake/gesture.hpp"

#include <algorithm>
#include <array>
#include <chrono>

namespace communicake {
namespace {

constexpr std::array<const char*, 7> kNames{"none", "fist", "one", "two", "three", "four", "five"};

long long steadyClockMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

} // namespace

const char* gestureName(Gesture gesture) {
    const auto index = static_cast<std::size_t>(gesture);
    return index < kNames.size() ? kNames[index] : "none";
}

std::optional<Gesture> gestureFromName(const std::string& name) {
    for (std::size_t i = 0; i < kNames.size(); ++i) {
        if (name == kNames[i]) {
            return static_cast<Gesture>(i);
        }
    }
    return std::nullopt;
}

std::optional<Gesture> gestureFromFingerCount(int fingers) {
    if (fingers < 0 || fingers > 5) {
        return std::nullopt;
    }
    return static_cast<Gesture>(fingers + 1); // Fist = 1
}

GestureStabilizer::GestureStabilizer(StabilizerConfig config, Clock clockMs)
    : config_(config), clock_(std::move(clockMs)) {}

std::optional<Gesture> GestureStabilizer::update(Gesture observed) {
    if (observed != candidate_) {
        candidate_ = observed;
        streak_ = 1;
        firedForHold_ = false;
    } else if (streak_ < 1000000) {
        ++streak_;
    }
    current_ = candidate_;

    if (candidate_ == Gesture::None) {
        return std::nullopt;
    }
    if (streak_ < std::max(1, config_.requiredFrames)) {
        return std::nullopt;
    }
    if (firedForHold_) {
        return std::nullopt;
    }

    const long long now = clock_ ? clock_() : steadyClockMs();
    if (hasFired_ && config_.cooldownMs > 0 && now - lastFireMs_ < config_.cooldownMs) {
        return std::nullopt;
    }

    firedForHold_ = true;
    hasFired_ = true;
    lastFireMs_ = now;
    return candidate_;
}

void GestureStabilizer::reset() {
    candidate_ = Gesture::None;
    current_ = Gesture::None;
    streak_ = 0;
    firedForHold_ = false;
    hasFired_ = false;
    lastFireMs_ = 0;
}

Gesture GestureStabilizer::current() const {
    return current_;
}

int GestureStabilizer::streak() const {
    return streak_;
}

} // namespace communicake
