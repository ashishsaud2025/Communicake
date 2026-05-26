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

TEST(a_smooth_blob_is_not_a_hand) {
    HandPipeline pipeline(testParams());
    pipeline.setBackground(synthetic::blank());
    const communicake::HandObservation observation = pipeline.process(synthetic::makeBlob());
    CHECK(observation.valid);
    CHECK_EQ(observation.fingerCount, 0);
}

TEST(speckle_is_ignored) {
    HandPipeline pipeline(testParams());
    pipeline.setBackground(synthetic::blank());
    const communicake::HandObservation observation = pipeline.process(synthetic::makeSpeckle());
    CHECK(!observation.valid);
}

TEST(an_empty_frame_is_invalid) {
    HandPipeline pipeline(testParams());
    const communicake::HandObservation observation = pipeline.process(cv::Mat());
    CHECK(!observation.valid);
    CHECK_EQ(observation.fingerCount, 0);
}

TEST(without_a_background_model_binary_mode_finds_nothing) {
    HandPipeline pipeline(testParams());
    const communicake::HandObservation observation = pipeline.process(synthetic::makeHand(3));
    CHECK(!observation.valid);
}

TEST(confidence_stays_in_range) {
    for (int fingers = 0; fingers <= 5; ++fingers) {
        HandPipeline pipeline(testParams());
        pipeline.setBackground(synthetic::blank());
        const communicake::HandObservation observation =
            pipeline.process(synthetic::makeHand(fingers));
        CHECK(observation.confidence >= 0.0);
        CHECK(observation.confidence <= 1.0);
    }
}

TEST(reusing_the_frame_buffers_does_not_leak_state) {
    HandPipeline pipeline(testParams());
    pipeline.setBackground(synthetic::blank());
    for (int round = 0; round < 3; ++round) {
        for (int fingers = 0; fingers <= 5; ++fingers) {
            const communicake::HandObservation observation =
                pipeline.process(synthetic::makeHand(fingers));
            CHECK_EQ(observation.fingerCount, fingers);
            // The mask alias must still describe the frame just processed.
            CHECK_EQ(pipeline.lastMask().size(), synthetic::blank().size());
        }
    }
}

TEST_MAIN
