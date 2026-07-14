#include "communicake/app.hpp"

#include "communicake/action.hpp"
#include "communicake/frame_source.hpp"
#include "communicake/gesture.hpp"
#include "communicake/observation_log.hpp"
#include "communicake/profile.hpp"
#include "communicake/timing.hpp"
#include "communicake/ui.hpp"
#include "communicake/vision.hpp"

#include <opencv2/imgcodecs.hpp>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>

namespace communicake {
namespace {

StageStats summarise(const RollingStats& samples) {
    StageStats stats;
    stats.meanMs = samples.mean();
    stats.p50Ms = samples.percentile(0.5);
    stats.p95Ms = samples.percentile(0.95);
    stats.maxMs = samples.maximum();
    return stats;
}

} // namespace

App::App(Options options, AppConfig config) : options_(std::move(options)), config_(std::move(config)) {}

std::string App::commandFor(Gesture gesture) const {
    const auto binding = config_.actions.find(gesture);
    if (binding == config_.actions.end()) {
        return std::string();
    }
    return binding->second.forPlatform(currentPlatform());
}

int App::run(RunSummary* summary) {
    // A benchmark is a measurement, not a session: no windows, no actions.
    if (options_.benchmark > 0) {
        options_.headless = true;
        options_.dryRun = true;
    }

    std::unique_ptr<FrameSource> source;
    std::string requested;
    if (!options_.input.empty()) {
        requested = options_.input;
        source = isDirectory(options_.input) ? openImageSequence(options_.input)
                                             : openVideoFile(options_.input);
    } else {
        requested = "camera " + std::to_string(options_.camera);
        source = openWebcam(options_.camera, options_.width, options_.height);
    }
    if (!source) {
        std::fprintf(stderr, "error: cannot open %s\n", requested.c_str());
        if (options_.input.empty()) {
            std::fprintf(stderr, "hint: check the camera index with --camera, or pass --input\n");
        }
        return 1;
    }

    std::printf("%s\n", versionString().c_str());
    std::printf("source: %s\n", source->describe().c_str());

    VisionParams params = config_.vision;
    HandPipeline pipeline(params);
    GestureStabilizer stabilizer(config_.stabilizer);
    ActionRunner runner(config_.dryRun || options_.dryRun);
    if (runner.dryRun()) {
        std::printf("dry run: actions will be printed, not launched\n");
    }

    if (!options_.profilePath.empty()) {
        std::string profileError;
        const auto profile = Profile::load(options_.profilePath, &profileError);
        if (!profile.has_value()) {
            std::fprintf(stderr, "error: %s\n", profileError.c_str());
            return 2;
        }
        config_ = profile->applyTo(config_);
        params = config_.vision;
        pipeline.setParams(params);
        std::printf("profile: %s\n", options_.profilePath.c_str());
    }

    cv::Mat frame;
    std::size_t frameIndex = 0;
    if (!source->next(frame)) {
        std::fprintf(stderr, "error: %s produced no frames\n", source->describe().c_str());
        return 1;
    }
    pipeline.setBackground(frame);
    ++frameIndex;

    std::unique_ptr<Ui> ui;
    if (!options_.headless) {
        ui = std::make_unique<Ui>(&params);
        ui->create();
    }

    ObservationLog log;
    if (!options_.logPath.empty()) {
        std::string logError;
        if (!log.open(options_.logPath, &logError)) {
            std::fprintf(stderr, "error: %s\n", logError.c_str());
            return 2;
        }
        std::printf("log: %s\n", options_.logPath.c_str());
    }

    if (!options_.dumpMaskDir.empty()) {
        std::error_code error;
        std::filesystem::create_directories(options_.dumpMaskDir, error);
        if (error) {
            std::fprintf(stderr, "warning: cannot create %s: %s\n", options_.dumpMaskDir.c_str(),
                         error.message().c_str());
        }
    }

    std::size_t processed = 0;
    std::size_t triggered = 0;
    double fps = source->fps();
    auto lastFpsUpdate = std::chrono::steady_clock::now();
    int framesSinceUpdate = 0;
    RollingStats segmentTimes(240);
    RollingStats analyseTimes(240);
    RollingStats frameTimes(240);

    // The first frames of a capture are dominated by allocator and cache
    // warm-up; including them would make short benchmarks pessimistic.
    constexpr std::size_t kWarmupFrames = 3;

    const std::size_t frameLimit =
        options_.benchmark > 0 ? static_cast<std::size_t>(options_.benchmark) : 0;

    while ((frameLimit == 0 || processed < frameLimit) && source->next(frame)) {
        ++frameIndex;
        if (ui) {
            ui->readIntoParams();
            pipeline.setParams(params);
            ui->updateModeWindow();
        }

        const HandObservation observation = pipeline.process(frame);
        // A contour that barely looks like a hand must not reach the
        // stabiliser: a stray sleeve or a shadow would otherwise count up to a
        // gesture of its own.
        const bool accepted = observation.valid && observation.confidence >= params.minConfidence;
        const std::optional<Gesture> observed =
            accepted ? gestureFromFingerCount(observation.fingerCount)
                     : std::optional<Gesture>(Gesture::None);
        const std::optional<Gesture> event = stabilizer.update(observed.value_or(Gesture::None));

        if (!accepted && options_.verbose) {
            std::printf("[%5zu] rejected  confidence=%.2f < %.2f\n", frameIndex,
                        observation.confidence, params.minConfidence);
        }

        std::string fired;
        if (event.has_value()) {
            const std::string command = commandFor(*event);
            if (!command.empty()) {
                const std::string expanded = ActionRunner::expand(command, currentPlatform());
                runner.run(expanded);
                fired = expanded;
                ++triggered;
                if (summary != nullptr) {
                    summary->actions.push_back(expanded);
                }
                if (options_.headless || options_.verbose) {
                    std::printf("[%5zu] %-5s fingers=%d confidence=%.2f  ->  %s\n", frameIndex,
                                gestureName(*event), observation.fingerCount,
                                observation.confidence, expanded.c_str());
                }
            }
        } else if (options_.verbose && options_.headless) {
            std::printf("[%5zu] %-5s fingers=%d confidence=%.2f area=%.0f\n", frameIndex,
                        gestureName(observed.value_or(Gesture::None)), observation.fingerCount,
                        observation.confidence, observation.area);
        }

        if (log.isOpen()) {
            ObservationRecord record;
            record.frame = frameIndex;
            record.handVisible = observation.valid;
            record.fingerCount = observation.fingerCount;
            record.confidence = observation.confidence;
            record.area = observation.area;
            record.gesture = gestureName(observed.value_or(Gesture::None));
            record.command = fired;
            log.write(record);
        }

        if (!options_.dumpMaskDir.empty() && !pipeline.lastMask().empty()) {
            char name[64];
            std::snprintf(name, sizeof(name), "mask_%05zu.png", frameIndex);
            const std::string path = (std::filesystem::path(options_.dumpMaskDir) / name).string();
            cv::imwrite(path, pipeline.lastMask());
        }

        ++processed;
        ++framesSinceUpdate;
        if (processed > kWarmupFrames) {
            segmentTimes.add(pipeline.lastTimings().segmentMs);
            analyseTimes.add(pipeline.lastTimings().contourMs);
            frameTimes.add(pipeline.lastTimings().totalMs);
        }
        const auto now = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(now - lastFpsUpdate).count();
        if (elapsed >= 0.5) {
            fps = framesSinceUpdate / elapsed;
            lastFpsUpdate = now;
            framesSinceUpdate = 0;
        }

        if (ui) {
            ui->show(frame, pipeline.lastMask(), pipeline.lastRoiView(), observation,
                     stabilizer.current(), fps, pipeline.lastTimings());
        }

        const int key = ui ? ui->waitKey(1) : -1;
        if (key == 27) { // ESC
            break;
        }
        if (key == 'b' || key == 'B') {
            pipeline.setBackground(frame);
        }
        if (key == 'm' || key == 'M') {
            params.mode = params.mode == SegmentationMode::Hsv ? SegmentationMode::Binary
                                                              : SegmentationMode::Hsv;
            if (ui) {
                ui->syncFromParams();
                ui->updateModeWindow();
            }
        }
        if (key == 's' || key == 'S') {
            const std::string path =
                options_.profilePath.empty() ? "profiles/communicake.json" : options_.profilePath;
            const std::filesystem::path parent = std::filesystem::path(path).parent_path();
            if (!parent.empty()) {
                std::error_code error;
                std::filesystem::create_directories(parent, error);
            }
            Profile profile = Profile::fromConfig(config_);
            profile.vision = params;
            std::string saveError;
            if (profile.save(path, &saveError)) {
                std::printf("profile saved to %s\n", path.c_str());
            } else {
                std::fprintf(stderr, "warning: %s\n", saveError.c_str());
            }
        }
    }

    if (summary != nullptr) {
        summary->frames = processed;
        summary->triggered = triggered;
        summary->segment = summarise(segmentTimes);
        summary->analyse = summarise(analyseTimes);
        summary->frame = summarise(frameTimes);
    }
    if (options_.benchmark > 0) {
        const StageStats total = summarise(frameTimes);
        std::printf("benchmark: %zu frames (%zu after warm-up)\n", processed, frameTimes.count());
        const auto printStage = [](const char* name, const StageStats& stats) {
            std::printf("  %-7s mean %6.2f  p50 %6.2f  p95 %6.2f  max %6.2f ms\n", name,
                        stats.meanMs, stats.p50Ms, stats.p95Ms, stats.maxMs);
        };
        printStage("segment", summarise(segmentTimes));
        printStage("analyse", summarise(analyseTimes));
        printStage("total", total);
        if (total.meanMs > 0.0) {
            std::printf("  ceiling %.1f fps (mean total per frame)\n", 1000.0 / total.meanMs);
        }
    } else if (options_.headless) {
        std::printf("processed %zu frames, triggered %zu actions\n", processed, triggered);
    }
    return 0;
}

} // namespace communicake