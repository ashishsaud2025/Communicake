#include "communicake/timing.hpp"

#include <algorithm>
#include <cmath>
#include <thread>

namespace communicake {

RollingStats::RollingStats(std::size_t capacity) : capacity_(std::max<std::size_t>(1, capacity)) {
    samples_.reserve(capacity_);
}

void RollingStats::add(double value) {
    if (samples_.size() < capacity_) {
        samples_.push_back(value);
    } else {
        samples_[next_] = value;
    }
    next_ = (next_ + 1) % capacity_;
    filled_ = std::min(filled_ + 1, capacity_);
}

void RollingStats::clear() {
    samples_.clear();
    next_ = 0;
    filled_ = 0;
}

bool RollingStats::empty() const {
    return filled_ == 0;
}

std::size_t RollingStats::count() const {
    return filled_;
}

std::size_t RollingStats::capacity() const {
    return capacity_;
}

double RollingStats::last() const {
    if (filled_ == 0) {
        return 0.0;
    }
    const std::size_t index = (next_ + capacity_ - 1) % capacity_;
    return samples_[index];
}

double RollingStats::mean() const {
    if (filled_ == 0) {
        return 0.0;
    }
    double total = 0.0;
    for (std::size_t i = 0; i < filled_; ++i) {
        total += samples_[i];
    }
    return total / static_cast<double>(filled_);
}

double RollingStats::maximum() const {
    if (filled_ == 0) {
        return 0.0;
    }
    double best = samples_[0];
    for (std::size_t i = 1; i < filled_; ++i) {
        best = std::max(best, samples_[i]);
    }
    return best;
}

double RollingStats::percentile(double fraction) const {
    if (filled_ == 0) {
        return 0.0;
    }
    std::vector<double> sorted(samples_.begin(),
                               samples_.begin() + static_cast<std::ptrdiff_t>(filled_));
    std::sort(sorted.begin(), sorted.end());
    const double clamped = std::clamp(fraction, 0.0, 1.0);
    const double position = clamped * static_cast<double>(sorted.size() - 1);
    const std::size_t lower = static_cast<std::size_t>(std::floor(position));
    const std::size_t upper = static_cast<std::size_t>(std::ceil(position));
    const double weight = position - static_cast<double>(lower);
    return sorted[lower] * (1.0 - weight) + sorted[upper] * weight;
}

Stopwatch::Stopwatch() : now_([] { return Clock::now(); }), start_(now_()) {}

Stopwatch::Stopwatch(Now now) : now_(std::move(now)), start_(now_()) {}

double Stopwatch::lapMs() {
    const TimePoint now = now_();
    const double elapsed = std::chrono::duration<double, std::milli>(now - start_).count();
    start_ = now;
    return elapsed;
}

double Stopwatch::elapsedMs() const {
    return std::chrono::duration<double, std::milli>(now_() - start_).count();
}

FramePacer::FramePacer(double targetFps)
    : sleepFor_([](std::chrono::milliseconds duration) {
          std::this_thread::sleep_for(duration);
      }) {
    if (targetFps > 0.0) {
        periodMs_ = 1000.0 / targetFps;
        active_ = true;
    }
}

FramePacer::FramePacer(double targetFps, Stopwatch::Now now, SleepFor sleepFor)
    : watch_(std::move(now)), sleepFor_(std::move(sleepFor)) {
    if (targetFps > 0.0) {
        periodMs_ = 1000.0 / targetFps;
        active_ = true;
    }
}

bool FramePacer::sleepUntilNextFrame() {
    if (!active_) {
        return false;
    }
    // The first frame establishes the baseline; nothing to pace against yet.
    if (firstFrame_) {
        firstFrame_ = false;
        watch_.lapMs();
        return true;
    }
    const double elapsed = watch_.lapMs();
    if (elapsed < periodMs_) {
        const std::chrono::milliseconds sleepFor =
            std::chrono::milliseconds(static_cast<long long>(periodMs_ - elapsed));
        sleepFor_(sleepFor);
    }
    return true;
}

bool FramePacer::active() const {
    return active_;
}

} // namespace communicake
