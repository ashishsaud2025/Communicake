#include "communicake/profile.hpp"

#include "test_harness.hpp"

#include <filesystem>

using communicake::AppConfig;
using communicake::Gesture;
using communicake::Profile;

TEST(a_profile_carries_the_tuning_and_nothing_else) {
    AppConfig config = AppConfig::defaults();
    config.vision.binaryThreshold = 44;
    config.stabilizer.cooldownMs = 1000;

    const Profile profile = Profile::fromConfig(config);
    const auto document = profile.toJson();
    CHECK(document.has("vision"));
    CHECK(document.has("stabilizer"));
    CHECK(!document.has("actions"));
    CHECK(!document.has("dryRun"));
}

TEST(applying_a_profile_keeps_the_actions) {
    AppConfig config = AppConfig::defaults();
    const std::size_t actionCount = config.actions.size();

    Profile profile;
    profile.vision.binaryThreshold = 12;
    profile.stabilizer.requiredFrames = 9;

    const AppConfig updated = profile.applyTo(config);
    CHECK_EQ(updated.vision.binaryThreshold, 12);
    CHECK_EQ(updated.stabilizer.requiredFrames, 9);
    CHECK_EQ(updated.actions.size(), actionCount);
}

TEST(a_profile_round_trips_through_a_file) {
    Profile profile;
    profile.vision.roi = cv::Rect(3, 4, 150, 160);
    profile.vision.mode = communicake::SegmentationMode::Hsv;
    profile.stabilizer.requiredFrames = 6;

    const auto path =
        std::filesystem::temp_directory_path() / "communicake_profile_roundtrip.json";
    std::string error;
    CHECK(profile.save(path.string(), &error));
    CHECK(error.empty());

    const auto loaded = Profile::load(path.string(), &error);
    CHECK(loaded.has_value());
    CHECK_EQ(loaded->vision.roi, cv::Rect(3, 4, 150, 160));
    CHECK(loaded->vision.mode == communicake::SegmentationMode::Hsv);
    CHECK_EQ(loaded->stabilizer.requiredFrames, 6);

    std::filesystem::remove(path);
}

TEST(an_invalid_profile_is_rejected) {
    std::string error;
    const auto profile = Profile::fromJson(communicake::json::parse(R"({"vision": {"roi": [1, 2]}})"),
                                           &error);
    CHECK(!profile.has_value());
    CHECK(error.find("vision.roi") != std::string::npos);
}

TEST_MAIN
