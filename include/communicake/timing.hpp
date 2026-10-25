#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <vector>

namespace communicake {

/// A fixed-capacity ring of timings. Keeping the window bounded means the
/// overlay and the benchmark report the recent behaviour of the machine rather
/// than an average over the whole run, and memory use stays flat.
class RollingStats {
  public:
    explicit RollingStats(std::size_t capacity = 120);

    void add(double value);
    void clear();

    bool empty() const;
    std::size_t count() const;
    std::size_t capacity() const;

    double last() const;
    double mean() const;
    double maximum() const;

    /// Linear-interpolated percentile. `fraction` is clamped to [0, 1]; an
    /// empty window yields 0.
    double percentile(double fraction) const;

  private:
    std::vector<double> samples_;
    std::size_t capacity_ = 1;
    std::size_t next_ = 0;
    std::size_t filled_ = 0;
};

/// Milliseconds between two points in time. The clock is injectable so the
/// tests never have to sleep.
class Stopwatch {
  public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Now = std::function<TimePoint()>;

    Stopwatch();
    explicit Stopwatch(Now now);

    /// Milliseconds since construction or the previous lap, and restart.
    double lapMs();

    /// Milliseconds since construction or the previous lap, without restarting.
    double elapsedMs() const;

  private:
    Now now_;
    TimePoint start_;
};

/// The frame period to pace at, or 0 when the rate is not credible. Rates
/// outside 1–1000 fps come from broken container metadata (a "0.001 fps" clip
/// would otherwise sleep for minutes per frame), and are ignored.
double pacingPeriodMs(double fps);

/// Paces frame production to a target frame rate, so a video plays back at
/// realtime speed instead of as fast as the machine can process it.
class FramePacer {
  public:
    using SleepFor = std::function<void(std::chrono::milliseconds)>;

    explicit FramePacer(double targetFps);
    FramePacer(double targetFps, Stopwatch::Now now, SleepFor sleepFor);

    /// Sleep, if needed, so that frames are spaced by the target period.
    /// Returns true if the pacer is active (the rate was credible).
    bool sleepUntilNextFrame();

    bool active() const;

  private:
    double periodMs_ = 0.0;
    Stopwatch watch_;
    SleepFor sleepFor_;
    bool active_ = false;
    bool firstFrame_ = true;
};

} // namespace communicake
