#pragma once

#include <functional>
#include <optional>
#include <string>

namespace communicake {

/// The gestures Communicake understands. `Fist` is a deliberate gesture (zero
/// fingers), while `None` means "no hand in view".
enum class Gesture { None = 0, Fist, One, Two, Three, Four, Five };

const char* gestureName(Gesture gesture);
std::optional<Gesture> gestureFromName(const std::string& name);

/// 0 fingers is a fist, 1..5 map to One..Five, anything else has no gesture.
std::optional<Gesture> gestureFromFingerCount(int fingers);

struct StabilizerConfig {
    int requiredFrames = 5; ///< consecutive frames needed before a gesture fires
    int cooldownMs = 750;   ///< minimum time between two different fired gestures
};

/// Turns a noisy per-frame gesture stream into at most one event per hold.
class GestureStabilizer {
  public:
    using Clock = std::function<long long()>;

    explicit GestureStabilizer(StabilizerConfig config = StabilizerConfig(),
                               Clock clockMs = nullptr);

    /// Feed one frame's gesture. Returns the gesture when it should trigger an
    /// action, and nullopt otherwise.
    std::optional<Gesture> update(Gesture observed);

    void reset();

    Gesture current() const;
    int streak() const;

  private:
    StabilizerConfig config_;
    Clock clock_;
    Gesture candidate_ = Gesture::None;
    Gesture current_ = Gesture::None;
    int streak_ = 0;
    bool firedForHold_ = false;
    bool hasFired_ = false;
    long long lastFireMs_ = 0;
};

} // namespace communicake
