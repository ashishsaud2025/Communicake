#include "communicake/gesture.hpp"

#include "test_harness.hpp"

using communicake::Gesture;
using communicake::GestureStabilizer;
using communicake::StabilizerConfig;
using communicake::gestureFromFingerCount;
using communicake::gestureFromName;
using communicake::gestureName;

namespace {

StabilizerConfig config(int frames = 5, int cooldown = 750) {
    StabilizerConfig c;
    c.requiredFrames = frames;
    c.cooldownMs = cooldown;
    return c;
}

} // namespace

TEST(finger_counts_map_to_gestures) {
    CHECK(gestureFromFingerCount(0) == Gesture::Fist);
    CHECK(gestureFromFingerCount(1) == Gesture::One);
    CHECK(gestureFromFingerCount(5) == Gesture::Five);
    CHECK(!gestureFromFingerCount(6).has_value());
    CHECK(!gestureFromFingerCount(-1).has_value());
}

TEST(gesture_names_round_trip) {
    for (Gesture gesture : {Gesture::Fist, Gesture::One, Gesture::Three, Gesture::Five}) {
        const auto parsed = gestureFromName(gestureName(gesture));
        CHECK(parsed.has_value());
        CHECK(*parsed == gesture);
    }
    CHECK(!gestureFromName("banana").has_value());
}

TEST(a_flicker_never_fires) {
    long long now = 0;
    GestureStabilizer stabilizer(config(5), [&now] { return now; });
    for (int i = 0; i < 4; ++i) {
        now += 16;
        CHECK(!stabilizer.update(Gesture::Two).has_value());
    }
    now += 16;
    CHECK(!stabilizer.update(Gesture::One).has_value());
}

TEST(a_sustained_gesture_fires_once) {
    long long now = 0;
    GestureStabilizer stabilizer(config(5), [&now] { return now; });

    std::optional<Gesture> fired;
    for (int i = 0; i < 5; ++i) {
        now += 16;
        const auto event = stabilizer.update(Gesture::Three);
        if (i < 4) {
            CHECK(!event.has_value());
        } else {
            fired = event;
        }
    }
    CHECK(fired.has_value());
    CHECK(*fired == Gesture::Three);
}

TEST(holding_a_gesture_does_not_refire) {
    long long now = 0;
    GestureStabilizer stabilizer(config(3), [&now] { return now; });
    int events = 0;
    for (int i = 0; i < 200; ++i) {
        now += 16;
        if (stabilizer.update(Gesture::Two).has_value()) {
            ++events;
        }
    }
    CHECK_EQ(events, 1);
}

TEST(a_new_gesture_fires_after_the_cooldown) {
    long long now = 0;
    GestureStabilizer stabilizer(config(3, 750), [&now] { return now; });
    for (int i = 0; i < 3; ++i) {
        now += 16;
        stabilizer.update(Gesture::Two);
    }

    // Change hands too soon: consensus is reached, but the cooldown blocks it.
    for (int i = 0; i < 3; ++i) {
        now += 16;
        CHECK(!stabilizer.update(Gesture::One).has_value());
    }
    now = 900; // cooldown elapsed
    bool fired = false;
    for (int i = 0; i < 3; ++i) {
        now += 16;
        const auto event = stabilizer.update(Gesture::One);
        if (event.has_value()) {
            fired = true;
            CHECK(*event == Gesture::One);
        }
    }
    CHECK(fired);
}

TEST(none_resets_the_streak) {
    long long now = 0;
    GestureStabilizer stabilizer(config(3), [&now] { return now; });
    now += 16;
    stabilizer.update(Gesture::Two);
    now += 16;
    stabilizer.update(Gesture::Two);
    now += 16;
    stabilizer.update(Gesture::None);
    CHECK_EQ(stabilizer.streak(), 1);

    now += 16;
    CHECK(!stabilizer.update(Gesture::Two).has_value());
}

TEST(reset_clears_everything) {
    long long now = 0;
    GestureStabilizer stabilizer(config(2), [&now] { return now; });
    for (int i = 0; i < 2; ++i) {
        now += 16;
        stabilizer.update(Gesture::Five);
    }
    stabilizer.reset();
    CHECK_EQ(stabilizer.streak(), 0);
    CHECK(stabilizer.current() == Gesture::None);
    now += 16;
    CHECK(!stabilizer.update(Gesture::Three).has_value());
}

TEST_MAIN
