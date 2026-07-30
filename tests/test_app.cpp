#include "communicake/app.hpp"

#include "synthetic.hpp"
#include "test_harness.hpp"

#include <opencv2/imgcodecs.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

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

TEST(a_confidence_floor_suppresses_gestures) {
    const fs::path directory = buildSequence("communicake_it_confidence", 10, 3);

    Options options;
    options.input = directory.string();
    options.headless = true;

    AppConfig config = syntheticConfig();
    config.vision.minConfidence = 0.99; // the synthetic hand cannot reach this
    App app(options, config);
    RunSummary summary;
    CHECK_EQ(app.run(&summary), 0);
    CHECK_EQ(summary.frames, std::size_t(10));
    CHECK_EQ(summary.triggered, std::size_t(0));
    fs::remove_all(directory);
}

TEST(a_benchmark_stops_after_the_requested_frames) {
    const fs::path directory = buildSequence("communicake_it_benchmark", 20, 3);

    Options options;
    options.input = directory.string();
    options.benchmark = 12;

    App app(options, syntheticConfig());
    RunSummary summary;
    CHECK_EQ(app.run(&summary), 0);
    CHECK_EQ(summary.frames, std::size_t(12));
    CHECK(summary.frame.meanMs > 0.0);
    CHECK(summary.segment.meanMs > 0.0);
    CHECK(summary.analyse.meanMs > 0.0);
    // The frame total is the sum of its stages, and the tail is never shorter
    // than the median.
    CHECK_NEAR(summary.frame.meanMs, summary.segment.meanMs + summary.analyse.meanMs, 1e-6);
    CHECK(summary.frame.p50Ms <= summary.frame.p95Ms);
    CHECK(summary.frame.p95Ms <= summary.frame.maxMs);
    fs::remove_all(directory);
}

TEST(a_run_can_log_every_frame) {
    const fs::path directory = buildSequence("communicake_it_log", 8, 3);
    const fs::path logPath = fs::temp_directory_path() / "communicake_it_log.csv";
    fs::remove(logPath);

    Options options;
    options.input = directory.string();
    options.headless = true;
    options.logPath = logPath.string();

    App app(options, syntheticConfig());
    RunSummary summary;
    CHECK_EQ(app.run(&summary), 0);
    CHECK_EQ(summary.frames, std::size_t(8));

    {
        std::ifstream file(logPath);
        std::string line;
        std::size_t rows = 0;
        bool sawThree = false;
        while (std::getline(file, line)) {
            ++rows;
            if (line.find("three") != std::string::npos && line.find("echo three") != std::string::npos) {
                sawThree = true;
            }
        }
        CHECK_EQ(rows, std::size_t(9)); // header plus one row per frame
        CHECK(sawThree);
    }

    fs::remove(logPath);
    fs::remove_all(directory);
}

TEST(image_sequence_frames_are_ordered_numerically) {
    const fs::path directory = fs::temp_directory_path() / "communicake_it_numeric";
    fs::remove_all(directory);
    fs::create_directories(directory);

    // First frame is the background; the rest show hands with 1, 2, 3 fingers,
    // but named without zero-padding so lexicographic order would be wrong.
    cv::imwrite((directory / "frame_0.png").string(), synthetic::blank());
    cv::imwrite((directory / "frame_10.png").string(), synthetic::makeHand(3));
    cv::imwrite((directory / "frame_2.png").string(), synthetic::makeHand(2));
    cv::imwrite((directory / "frame_1.png").string(), synthetic::makeHand(1));

    const fs::path logPath = fs::temp_directory_path() / "communicake_it_numeric.csv";
    fs::remove(logPath);

    Options options;
    options.input = directory.string();
    options.headless = true;
    options.logPath = logPath.string();

    App app(options, syntheticConfig());
    RunSummary summary;
    CHECK_EQ(app.run(&summary), 0);
    CHECK_EQ(summary.frames, std::size_t(3));

    {
        std::ifstream file(logPath);
        std::string line;
        std::getline(file, line); // skip header
        std::vector<int> counts;
        while (std::getline(file, line)) {
            // CSV: frame,hand,fingers,confidence,area,gesture,command
            const std::size_t first = line.find(',');
            const std::size_t second = line.find(',', first + 1);
            const std::size_t third = line.find(',', second + 1);
            const int fingers = std::stoi(line.substr(second + 1, third - second - 1));
            counts.push_back(fingers);
        }
        // Numeric order: frame_1 (1 finger), frame_2 (2 fingers), frame_10 (3 fingers)
        CHECK_EQ(counts.size(), std::size_t(3));
        CHECK_EQ(counts[0], 1);
        CHECK_EQ(counts[1], 2);
        CHECK_EQ(counts[2], 3);
    }

    fs::remove(logPath);
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
