#include "communicake/config.hpp"

#include "test_harness.hpp"

#include <filesystem>
#include <fstream>

using communicake::AppConfig;
using communicake::Gesture;
using communicake::json::parse;

namespace {

std::filesystem::path temporaryPath(const std::string& name) {
    return std::filesystem::temp_directory_path() / name;
}

} // namespace

TEST(defaults_bind_the_documented_gestures) {
    const AppConfig config = AppConfig::defaults();
    CHECK(!config.actions.empty());
    CHECK(!config.actions.at(Gesture::Two).forPlatform("windows").empty());
    CHECK(!config.dryRun);
}

TEST(a_partial_config_keeps_the_defaults) {
    const auto root = parse(R"({"vision": {"binaryThreshold": 40}})");
    std::string error;
    const auto config = AppConfig::fromJson(root, &error);
    CHECK(config.has_value());
    CHECK(error.empty());
    CHECK_EQ(config->vision.binaryThreshold, 40);
    CHECK_EQ(config->vision.fingertipDistance, communicake::VisionParams().fingertipDistance);
}

TEST(out_of_range_values_are_rejected_with_the_key_name) {
    std::string error;
    const auto config = AppConfig::fromJson(parse(R"({"vision": {"binaryThreshold": 900}})"), &error);
    CHECK(!config.has_value());
    CHECK(error.find("vision.binaryThreshold") != std::string::npos);

    error.clear();
    const auto frames = AppConfig::fromJson(parse(R"({"stabilizer": {"requiredFrames": 0}})"), &error);
    CHECK(!frames.has_value());
    CHECK(error.find("stabilizer.requiredFrames") != std::string::npos);
}

TEST(a_bad_mode_is_rejected) {
    std::string error;
    const auto config = AppConfig::fromJson(parse(R"({"vision": {"mode": "magic"}})"), &error);
    CHECK(!config.has_value());
    CHECK(error.find("vision.mode") != std::string::npos);
}

TEST(an_unknown_gesture_is_rejected) {
    std::string error;
    const auto config = AppConfig::fromJson(parse(R"({"actions": {"banana": "open"}})"), &error);
    CHECK(!config.has_value());
    CHECK(error.find("banana") != std::string::npos);
}

TEST(actions_accept_the_string_shorthand) {
    std::string error;
    const auto config = AppConfig::fromJson(parse(R"({"actions": {"two": "gedit"}})"), &error);
    CHECK(config.has_value());
    CHECK_EQ(config->actions.at(Gesture::Two).forPlatform("linux"), std::string("gedit"));
}

TEST(a_platform_override_beats_the_fallback) {
    std::string error;
    const auto config = AppConfig::fromJson(
        parse(R"({"actions": {"three": {"command": "generic", "windows": "start chrome"}}})"),
        &error);
    CHECK(config.has_value());
    CHECK_EQ(config->actions.at(Gesture::Three).forPlatform("windows"), std::string("start chrome"));
    CHECK_EQ(config->actions.at(Gesture::Three).forPlatform("linux"), std::string("generic"));
}

TEST(the_dry_run_flag_is_read) {
    const auto config = AppConfig::fromJson(parse(R"({"dryRun": true})"));
    CHECK(config.has_value());
    CHECK(config->dryRun);
}

TEST(a_config_round_trips_through_a_file) {
    AppConfig original = AppConfig::defaults();
    original.vision.binaryThreshold = 33;
    original.vision.minConfidence = 0.4;
    original.vision.morphKernel = 7;
    original.vision.roi = cv::Rect(4, 5, 200, 210);
    original.stabilizer.requiredFrames = 7;
    original.dryRun = true;

    const auto path = temporaryPath("communicake_config_roundtrip.json");
    std::string error;
    CHECK(original.save(path.string(), &error));
    CHECK(error.empty());

    const auto loaded = AppConfig::load(path.string(), &error);
    CHECK(loaded.has_value());
    CHECK_EQ(loaded->vision.binaryThreshold, 33);
    CHECK_NEAR(loaded->vision.minConfidence, 0.4, 1e-9);
    CHECK_EQ(loaded->vision.morphKernel, 7);
    CHECK_EQ(loaded->vision.roi, cv::Rect(4, 5, 200, 210));
    CHECK_EQ(loaded->stabilizer.requiredFrames, 7);
    CHECK(loaded->dryRun);

    std::filesystem::remove(path);
}

TEST(the_confidence_floor_is_range_checked) {
    std::string error;
    const auto parsed = AppConfig::fromJson(parse(R"({"vision": {"minConfidence": 1.5}})"), &error);
    CHECK(!parsed.has_value());
    CHECK(error.find("minConfidence") != std::string::npos);
}

TEST(a_config_without_a_confidence_floor_keeps_the_default) {
    const auto parsed = AppConfig::fromJson(parse(R"({"vision": {"binaryThreshold": 12}})"));
    CHECK(parsed.has_value());
    CHECK_NEAR(parsed->vision.minConfidence, 0.0, 1e-9);
    CHECK_EQ(parsed->vision.binaryThreshold, 12);
}

TEST(the_morphology_kernel_is_range_checked) {
    std::string error;
    const auto tooSmall = AppConfig::fromJson(parse(R"({"vision": {"morphKernel": 1}})"), &error);
    CHECK(!tooSmall.has_value());
    CHECK(error.find("morphKernel") != std::string::npos);

    const auto tooLarge = AppConfig::fromJson(parse(R"({"vision": {"morphKernel": 64}})"), &error);
    CHECK(!tooLarge.has_value());
}

TEST(the_morphology_kernel_defaults_for_an_old_config) {
    const auto parsed = AppConfig::fromJson(parse(R"({"vision": {"blurKernel": 7}})"));
    CHECK(parsed.has_value());
    CHECK_EQ(parsed->vision.morphKernel, 5);
}

TEST(a_malformed_file_names_the_path) {
    const auto path = temporaryPath("communicake_config_broken.json");
    {
        std::ofstream file(path);
        file << "{\"vision\": }";
    }
    std::string error;
    const auto loaded = AppConfig::load(path.string(), &error);
    CHECK(!loaded.has_value());
    CHECK(error.find("communicake_config_broken.json") != std::string::npos);

    std::filesystem::remove(path);
}

TEST(a_duplicate_key_is_reported_with_its_name) {
    const auto path = temporaryPath("communicake_config_duplicate.json");
    {
        std::ofstream file(path);
        file << R"({"vision": {"blurKernel": 3}, "vision": {"blurKernel": 7}})";
    }
    std::string error;
    const auto loaded = AppConfig::load(path.string(), &error);
    CHECK(!loaded.has_value());
    CHECK(error.find("communicake_config_duplicate.json") != std::string::npos);
    CHECK(error.find("duplicate key \"vision\"") != std::string::npos);

    std::filesystem::remove(path);
}

TEST(a_missing_file_is_reported) {
    std::string error;
    const auto loaded = AppConfig::load("definitely-not-here.json", &error);
    CHECK(!loaded.has_value());
    CHECK(error.find("definitely-not-here.json") != std::string::npos);
}

TEST_MAIN
