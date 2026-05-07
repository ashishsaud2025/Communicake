#include "communicake/app.hpp"

#include "synthetic.hpp"
#include "test_harness.hpp"

#include <opencv2/imgcodecs.hpp>

#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

using communicake::ActionBinding;
using communicake::App;
using communicake::AppConfig;
using communicake::Gesture;
using communicake::Options;
using communicake::RunSummary;

namespace {

fs::path buildSequence(const std::string& name, int handFrames, int fingers) {
    const fs::path directory = fs::temp_directory_path() / name;
    fs::remove_all(directory);
    fs::create_directories(directory);

    // The first frame is the empty background the pipeline captures its model
    // from; the rest show the hand.
    cv::imwrite((directory / "frame_000.png").string(), synthetic::blank());
    for (int i = 1; i <= handFrames; ++i) {
        char filename[32];
        std::snprintf(filename, sizeof(filename), "frame_%03d.png", i);
        cv::imwrite((directory / filename).string(), synthetic::makeHand(fingers));
    }
    return directory;
}

AppConfig syntheticConfig() {
    AppConfig config = AppConfig::defaults();
    config.vision.roi = cv::Rect(0, 0, synthetic::kDefaultCanvas, synthetic::kDefaultCanvas);
    config.vision.minContourArea = 800.0;
    config.stabilizer.requiredFrames = 3;
    config.dryRun = true;
    config.actions[Gesture::Three] = ActionBinding{"echo three", {}};
    config.actions[Gesture::Fist] = ActionBinding{"echo fist", {}};
    return config;
}

} // namespace

TEST(a_held_three_finger_gesture_triggers_once) {
    const fs::path directory = buildSequence("communicake_it_three", 10, 3);

    Options options;
    options.input = directory.string();
    options.headless = true;

    App app(options, syntheticConfig());
    RunSummary summary;
    CHECK_EQ(app.run(&summary), 0);
    CHECK_EQ(summary.frames, std::size_t(10));
    CHECK_EQ(summary.triggered, std::size_t(1));
    CHECK_EQ(summary.actions.size(), std::size_t(1));
    if (!summary.actions.empty()) {
        CHECK_EQ(summary.actions.front(), std::string("echo three"));
    }
    fs::remove_all(directory);
}

TEST(a_fist_triggers_the_fist_action) {
    const fs::path directory = buildSequence("communicake_it_fist", 6, 0);

    Options options;
    options.input = directory.string();
    options.headless = true;

    App app(options, syntheticConfig());
    RunSummary summary;
    CHECK_EQ(app.run(&summary), 0);
    CHECK_EQ(summary.triggered, std::size_t(1));
    if (!summary.actions.empty()) {
        CHECK_EQ(summary.actions.front(), std::string("echo fist"));
    }
    fs::remove_all(directory);
}

TEST(an_empty_scene_triggers_nothing) {
    const fs::path directory = fs::temp_directory_path() / "communicake_it_empty";
    fs::remove_all(directory);
    fs::create_directories(directory);
    for (int i = 0; i < 6; ++i) {
        char filename[32];
        std::snprintf(filename, sizeof(filename), "frame_%03d.png", i);
        cv::imwrite((directory / filename).string(), synthetic::blank());
    }

    Options options;
    options.input = directory.string();
    options.headless = true;

    App app(options, syntheticConfig());
    RunSummary summary;
    CHECK_EQ(app.run(&summary), 0);
    CHECK_EQ(summary.frames, std::size_t(5));
    CHECK_EQ(summary.triggered, std::size_t(0));
    fs::remove_all(directory);
}

TEST(a_missing_input_fails_cleanly) {
    Options options;
    options.input = "definitely-not-a-video.mp4";
    options.headless = true;

    App app(options, syntheticConfig());
    RunSummary summary;
    CHECK_EQ(app.run(&summary), 1);
    CHECK_EQ(summary.frames, std::size_t(0));
}

TEST_MAIN
