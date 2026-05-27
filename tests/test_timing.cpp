#include "communicake/timing.hpp"

#include "test_harness.hpp"

#include <chrono>

using communicake::RollingStats;
using communicake::Stopwatch;

namespace {

std::chrono::steady_clock::time_point at(int milliseconds) {
    return std::chrono::steady_clock::time_point{} + std::chrono::milliseconds(milliseconds);
}

} // namespace

TEST(an_empty_window_reports_zero) {
    const RollingStats stats(4);
    CHECK(stats.empty());
    CHECK_EQ(stats.count(), std::size_t(0));
    CHECK_NEAR(stats.mean(), 0.0, 1e-9);
    CHECK_NEAR(stats.percentile(0.5), 0.0, 1e-9);
    CHECK_NEAR(stats.last(), 0.0, 1e-9);
}

TEST(the_window_keeps_only_the_most_recent_samples) {
    RollingStats stats(3);
    for (int value = 1; value <= 5; ++value) {
        stats.add(value);
    }
    CHECK_EQ(stats.count(), std::size_t(3));
    CHECK_NEAR(stats.mean(), 4.0, 1e-9); // 3, 4, 5
    CHECK_NEAR(stats.last(), 5.0, 1e-9);
    CHECK_NEAR(stats.maximum(), 5.0, 1e-9);
}

TEST(percentiles_interpolate_between_neighbours) {
    RollingStats stats(10);
    for (double value = 1.0; value <= 5.0; value += 1.0) {
        stats.add(value);
    }
    CHECK_NEAR(stats.percentile(0.0), 1.0, 1e-9);
    CHECK_NEAR(stats.percentile(1.0), 5.0, 1e-9);
    CHECK_NEAR(stats.percentile(0.5), 3.0, 1e-9);
    CHECK_NEAR(stats.percentile(0.25), 2.0, 1e-9);
    CHECK_NEAR(stats.percentile(-4.0), 1.0, 1e-9); // clamped
    CHECK_NEAR(stats.percentile(9.0), 5.0, 1e-9);  // clamped
}

TEST(clearing_the_window_forgets_everything) {
    RollingStats stats(4);
    stats.add(10.0);
    stats.clear();
    CHECK(stats.empty());
    CHECK_NEAR(stats.mean(), 0.0, 1e-9);
    stats.add(2.0);
    stats.add(4.0);
    CHECK_NEAR(stats.mean(), 3.0, 1e-9);
}

TEST(the_stopwatch_measures_laps_with_an_injected_clock) {
    int nowMs = 1000;
    Stopwatch watch([&nowMs] { return at(nowMs); });

    nowMs += 25;
    CHECK_NEAR(watch.lapMs(), 25.0, 1e-6);

    nowMs += 10;
    CHECK_NEAR(watch.elapsedMs(), 10.0, 1e-6); // since the last lap, without restarting
    CHECK_NEAR(watch.lapMs(), 10.0, 1e-6);

    nowMs += 5;
    CHECK_NEAR(watch.lapMs(), 5.0, 1e-6);
}

TEST_MAIN
