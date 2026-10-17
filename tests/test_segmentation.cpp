#include "communicake/vision.hpp"

#include "synthetic.hpp"
#include "test_harness.hpp"

#include <opencv2/imgproc.hpp>

using communicake::HandPipeline;
using communicake::VisionParams;

namespace {

VisionParams testParams() {
    VisionParams params;
    params.mode = communicake::SegmentationMode::Binary;
    params.roi = cv::Rect(0, 0, synthetic::kDefaultCanvas, synthetic::kDefaultCanvas);
    params.minContourArea = 800.0;
    return params;
}

double coverage(int fingers) {
    HandPipeline pipeline(testParams());
    pipeline.setBackground(synthetic::blank());
    pipeline.process(synthetic::makeHand(fingers));
    const cv::Mat& mask = pipeline.lastMask();
    const double total = static_cast<double>(mask.rows) * static_cast<double>(mask.cols);
    return total > 0.0 ? cv::countNonZero(mask) / total : 0.0;
}

} // namespace

// The expected values were measured once and pinned. A change to the
// segmentation (thresholding, morphology, the background model) that shifts
// them by more than a percentage point must be an intentional, reviewed change,
// not an accident.
TEST(segmentation_coverage_matches_the_golden_ratios) {
    const double expected[] = {0.0809, 0.1013, 0.1207, 0.1402, 0.1585, 0.1772};
    for (int fingers = 0; fingers <= 5; ++fingers) {
        CHECK_NEAR(coverage(fingers), expected[fingers], 0.01);
    }
}

TEST(every_extra_finger_covers_more_of_the_mask) {
    double previous = 0.0;
    for (int fingers = 0; fingers <= 5; ++fingers) {
        const double current = coverage(fingers);
        CHECK(current > previous);
        previous = current;
    }
}

TEST(the_mask_is_binary_and_roi_sized) {
    HandPipeline pipeline(testParams());
    pipeline.setBackground(synthetic::blank());
    pipeline.process(synthetic::makeHand(3));

    const cv::Mat& mask = pipeline.lastMask();
    CHECK_EQ(mask.size(), cv::Size(synthetic::kDefaultCanvas, synthetic::kDefaultCanvas));
    CHECK_EQ(mask.type(), CV_8UC1);

    double minimum = 0.0;
    double maximum = 0.0;
    cv::minMaxLoc(mask, &minimum, &maximum);
    CHECK_NEAR(minimum, 0.0, 1e-9);
    CHECK_NEAR(maximum, 255.0, 1e-9);

    // Every set pixel is exactly 255, never a grey value left by the blur.
    cv::Mat saturated;
    cv::compare(mask, 255, saturated, cv::CMP_EQ);
    CHECK_EQ(cv::countNonZero(mask), cv::countNonZero(saturated));
}

TEST(a_blank_frame_produces_an_empty_mask) {
    HandPipeline pipeline(testParams());
    pipeline.setBackground(synthetic::blank());
    pipeline.process(synthetic::blank());
    CHECK_EQ(cv::countNonZero(pipeline.lastMask()), 0);
}

TEST_MAIN
