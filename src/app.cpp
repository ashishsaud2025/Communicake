#include "communicake/app.hpp"

#include "communicake/action.hpp"
#include "communicake/frame_source.hpp"
#include "communicake/gesture.hpp"
#include "communicake/profile.hpp"
#include "communicake/ui.hpp"
#include "communicake/vision.hpp"

#include <opencv2/imgcodecs.hpp>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>

namespace communicake {

App::App(Options options, AppConfig config) : options_(std::move(options)), config_(std::move(config)) {}

std::string App::commandFor(Gesture gesture) const {
    const auto binding = config_.actions.find(gesture);
    if (binding == config_.actions.end()) {
        return std::string();
    }
    return binding->second.forPlatform(currentPlatform());
}

int App::run(RunSummary* summary) {
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

    while (source->next(frame)) {
        ++frameIndex;
        if (ui) {
            ui->readIntoParams();
            pipeline.setParams(params);
            ui->updateModeWindow();
        }

        const HandObservation observation = pipeline.process(frame);
        const std::optional<Gesture> observed =
            observation.valid ? gestureFromFingerCount(observation.fingerCount)
                              : std::optional<Gesture>(Gesture::None);
        const std::optional<Gesture> event = stabilizer.update(observed.value_or(Gesture::None));

        if (event.has_value()) {
            const std::string command = commandFor(*event);
            if (!command.empty()) {
                const std::string expanded = ActionRunner::expand(command, currentPlatform());
                runner.run(expanded);
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

        if (!options_.dumpMaskDir.empty() && !pipeline.lastMask().empty()) {
            char name[64];
            std::snprintf(name, sizeof(name), "mask_%05zu.png", frameIndex);
            const std::string path = (std::filesystem::path(options_.dumpMaskDir) / name).string();
            cv::imwrite(path, pipeline.lastMask());
        }

        ++processed;
        ++framesSinceUpdate;
        const auto now = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(now - lastFpsUpdate).count();
        if (elapsed >= 0.5) {
            fps = framesSinceUpdate / elapsed;
            lastFpsUpdate = now;
            framesSinceUpdate = 0;
        }

        if (ui) {
            ui->show(frame, pipeline.lastMask(), pipeline.lastRoiView(), observation,
                     stabilizer.current(), fps);
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
    }
    if (options_.headless) {
        std::printf("processed %zu frames, triggered %zu actions\n", processed, triggered);
    }
    return 0;
}

} // namespace communicake