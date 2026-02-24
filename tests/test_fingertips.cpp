#include "communicake/vision.hpp"

#include "synthetic.hpp"
#include "test_harness.hpp"

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

int countFor(const cv::Mat& hand) {
    HandPipeline pipeline(testParams());
    pipeline.setBackground(synthetic::blank());
    const communicake::HandObservation observation = pipeline.process(hand);
    return observation.valid ? observation.fingerCount : -1;
}

} // namespace

TEST(zero_fingers_on_a_fist) {
    CHECK_EQ(countFor(synthetic::makeHand(0)), 0);
}

TEST(one_finger) {
    CHECK_EQ(countFor(synthetic::makeHand(1)), 1);
}

TEST(two_fingers) {
    CHECK_EQ(countFor(synthetic::makeHand(2)), 2);
}

TEST(three_fingers) {
    CHECK_EQ(countFor(synthetic::makeHand(3)), 3);
}

TEST(four_fingers) {
    CHECK_EQ(countFor(synthetic::makeHand(4)), 4);
}

TEST(five_fingers) {
    CHECK_EQ(countFor(synthetic::makeHand(5)), 5);
}

TEST(rotated_hands_keep_their_count) {
    CHECK_EQ(countFor(synthetic::makeHand(3, synthetic::kDefaultCanvas, 12.0)), 3);
    CHECK_EQ(countFor(synthetic::makeHand(5, synthetic::kDefaultCanvas, -12.0)), 5);
}

TEST_MAIN
