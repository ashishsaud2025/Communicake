#include "communicake/geometry.hpp"

#include "test_harness.hpp"

#include <opencv2/imgproc.hpp>

#include <cstdint>

using communicake::geometry::clampRect;
using communicake::geometry::centroid;
using communicake::geometry::largestContourIndex;
using communicake::geometry::polygonArea;

TEST(polygon_area_of_a_triangle) {
    const std::vector<cv::Point> triangle{{0, 0}, {10, 0}, {0, 10}};
    CHECK_NEAR(polygonArea(triangle), 50.0, 0.001);
}

TEST(polygon_area_of_a_degenerate_polygon_is_zero) {
    CHECK_NEAR(polygonArea({}), 0.0, 0.001);
    CHECK_NEAR(polygonArea({{0, 0}, {1, 1}}), 0.0, 0.001);
    CHECK_NEAR(polygonArea({{0, 0}, {10, 10}, {20, 20}}), 0.0, 0.001);
}

TEST(centroid_of_a_square_is_its_centre) {
    const std::vector<cv::Point> square{{0, 0}, {100, 0}, {100, 100}, {0, 100}};
    const auto centre = centroid(square);
    CHECK(centre.has_value());
    CHECK_EQ(centre->x, 50);
    CHECK_EQ(centre->y, 50);
}

TEST(centroid_rejects_degenerate_contours) {
    CHECK(!centroid({}).has_value());
    CHECK(!centroid({{5, 5}}).has_value());
    CHECK(!centroid({{0, 0}, {10, 10}}).has_value());
    CHECK(!centroid({{0, 0}, {10, 10}, {20, 20}}).has_value());
}

TEST(largest_contour_index_picks_the_biggest) {
    const std::vector<std::vector<cv::Point>> contours{
        {{0, 0}, {10, 0}, {10, 10}, {0, 10}},
        {{0, 0}, {100, 0}, {100, 100}, {0, 100}},
        {{0, 0}, {4, 0}, {4, 4}, {0, 4}},
    };
    CHECK_EQ(largestContourIndex(contours), 1);
    CHECK_EQ(largestContourIndex({}), -1);
}

TEST(largest_contour_index_respects_the_minimum_area) {
    const std::vector<std::vector<cv::Point>> contours{
        {{0, 0}, {10, 0}, {10, 10}, {0, 10}},
    };
    CHECK_EQ(largestContourIndex(contours, 50.0), 0);
    CHECK_EQ(largestContourIndex(contours, 500.0), -1);
}

TEST(clamp_rect_keeps_a_valid_rectangle_untouched) {
    const cv::Rect clamped = clampRect(cv::Rect(10, 10, 100, 50), cv::Size(640, 480));
    CHECK_EQ(clamped, cv::Rect(10, 10, 100, 50));
}

TEST(clamp_rect_trims_a_negative_origin) {
    const cv::Rect clamped = clampRect(cv::Rect(-5, -5, 100, 100), cv::Size(640, 480));
    CHECK_EQ(clamped.x, 0);
    CHECK_EQ(clamped.y, 0);
    CHECK_EQ(clamped.width, 95);
    CHECK_EQ(clamped.height, 95);
}

TEST(clamp_rect_trims_an_oversized_rectangle) {
    const cv::Rect clamped = clampRect(cv::Rect(600, 400, 1000, 1000), cv::Size(640, 480));
    CHECK_EQ(clamped.x, 600);
    CHECK_EQ(clamped.y, 400);
    CHECK_EQ(clamped.width, 40);
    CHECK_EQ(clamped.height, 80);
}

TEST(clamp_rect_never_returns_an_empty_rectangle) {
    const cv::Rect clamped = clampRect(cv::Rect(9999, 9999, 10, 10), cv::Size(640, 480));
    CHECK_EQ(clamped.width, 1);
    CHECK_EQ(clamped.height, 1);
    CHECK_EQ(clamped.x, 639);
    CHECK_EQ(clamped.y, 479);
}

TEST(clamp_rect_on_an_empty_frame_is_empty) {
    CHECK_EQ(clampRect(cv::Rect(0, 0, 10, 10), cv::Size(0, 0)), cv::Rect());
}

namespace {

/// Deterministic linear congruential generator. The C++ <random> engines are
/// implementation defined, and a property test that fails on one platform and
/// passes on another is worse than the hand-picked cases above.
class Lcg {
  public:
    explicit Lcg(std::uint32_t seed) : state_(seed) {}

    std::uint32_t next() {
        state_ = state_ * 1664525u + 1013904223u;
        return state_;
    }

    /// A value in [low, high], inclusive.
    int between(int low, int high) {
        const std::uint32_t span = static_cast<std::uint32_t>(high - low + 1);
        return low + static_cast<int>(next() % span);
    }

  private:
    std::uint32_t state_;
};

bool contains(const cv::Rect& outer, const cv::Rect& inner) {
    return inner.x >= outer.x && inner.y >= outer.y &&
           inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}

bool inside(const cv::Rect& rect, const cv::Size& frame) {
    return rect.x >= 0 && rect.y >= 0 && rect.width >= 1 && rect.height >= 1 &&
           rect.x + rect.width <= frame.width && rect.y + rect.height <= frame.height;
}

} // namespace

TEST(clamp_rect_holds_its_invariants_for_random_input) {
    Lcg random(20260101u);
    for (int iteration = 0; iteration < 2000; ++iteration) {
        const cv::Size frame(random.between(1, 800), random.between(1, 800));
        const cv::Rect rect(random.between(-500, 1200), random.between(-500, 1200),
                            random.between(-200, 1500), random.between(-200, 1500));
        const cv::Rect clamped = clampRect(rect, frame);

        CHECK(inside(clamped, frame));

        // Clamping may grow a degenerate rectangle to one pixel, but it must
        // never grow a rectangle that actually intersects the frame.
        const cv::Rect intersection = rect & cv::Rect(0, 0, frame.width, frame.height);
        if (!intersection.empty()) {
            CHECK(contains(rect, clamped));
            CHECK(contains(clamped, intersection));
            CHECK_EQ(clamped, intersection);
        }

        // Clamping an already clamped rectangle must not move it.
        CHECK_EQ(clampRect(clamped, frame), clamped);
    }
}

TEST_MAIN
