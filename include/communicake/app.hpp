#pragma once

#include "communicake/cli.hpp"
#include "communicake/config.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace communicake {

/// What a run did. Returned so the tests can assert on a headless run without
/// scraping stdout.
struct RunSummary {
    std::size_t frames = 0;
    std::size_t triggered = 0;
    std::vector<std::string> actions;
};

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
