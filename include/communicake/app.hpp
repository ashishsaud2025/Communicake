#pragma once

#include "communicake/cli.hpp"
#include "communicake/config.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace communicake {

/// Mean, median, tail and worst case for one series of timings.
struct StageStats {
    double meanMs = 0.0;
    double p50Ms = 0.0;
    double p95Ms = 0.0;
    double maxMs = 0.0;
};

/// What a run did. Returned so the tests can assert on a headless run without
/// scraping stdout.
struct RunSummary {
    std::size_t frames = 0;
    std::size_t triggered = 0;
    std::size_t failed = 0;
    StageStats segment;
    StageStats analyse;
    StageStats frame;
    std::vector<std::string> actions;
};

/// One line describing a finished run, e.g. "processed 120 frames, triggered 2
/// actions, 1 failed". Kept out of App::run so the wording is covered by tests
/// instead of by scraping stdout.
std::string formatRunSummary(const RunSummary& summary);

/// Wires a frame source, the pipeline, the stabiliser and the action runner
/// together. Interactive and headless runs share the same loop.
class App {
  public:
    App(Options options, AppConfig config);

    /// Returns 0 on success, 1 when the frame source cannot be opened, 2 for a
    /// configuration problem.
    int run(RunSummary* summary = nullptr);

  private:
    std::string commandFor(Gesture gesture) const;

    Options options_;
    AppConfig config_;
};

} // namespace communicake
