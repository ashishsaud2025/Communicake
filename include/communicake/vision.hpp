#pragma once

#include "communicake/types.hpp"

#include <opencv2/core.hpp>

#include <vector>

namespace communicake {

/// Every tunable value the vision pipeline uses. The defaults are the ones the
/// trackbar UI starts from; a config file or a profile can override them.
struct VisionParams {
    SegmentationMode mode = SegmentationMode::Binary;
    int binaryThreshold = 20;
    double minContourArea = 1200.0;
    int fingertipDistance = 55;
    int minFingerGap = 34;
    int defectDepth = 18;
    int blurKernel = 5;
    cv::Rect roi{10, 10, 300, 300};
    cv::Scalar hsvMin{0, 60, 60};
    cv::Scalar hsvMax{25, 255, 255};
};

/// Separates a hand from the background. Holds no reference to a camera, a
/// window or the operating system.
class Segmenter {
  public:
    explicit Segmenter(const VisionParams& params = VisionParams());

    void setParams(const VisionParams& params);
    const VisionParams& params() const;

    /// Capture the background model from a full frame. Returns false for an
    /// empty frame. The frame is cropped to `params().roi` first.
    bool setBackground(const cv::Mat& frame);
    bool hasBackground() const;

    /// Produce an 8-bit single-channel mask for a ROI-sized BGR image.
    cv::Mat segment(const cv::Mat& roiBgr) const;

  private:
    VisionParams params_;
    cv::Mat background_; // grayscale, ROI sized
    bool hasBackground_ = false;
};

/// Crops the ROI, segments it, finds the largest contour and turns it into a
/// HandObservation. Pure computation: safe to call from tests and CI.
class HandPipeline {
  public:
    explicit HandPipeline(const VisionParams& params = VisionParams());

    HandObservation process(const cv::Mat& frame);

    void setBackground(const cv::Mat& frame);
    void setParams(const VisionParams& params);

    const VisionParams& params() const;
    const cv::Mat& lastMask() const;
    const cv::Mat& lastRoiView() const;
    cv::Rect lastRoi() const;

  private:
    VisionParams params_;
    Segmenter segmenter_;
    cv::Mat mask_;
    cv::Mat roiView_;
    cv::Rect lastRoi_;
};

namespace vision {

/// Fingertip points for a hand contour, ordered by distance from the centroid.
/// At most five points are returned.
std::vector<cv::Point> findFingertips(const std::vector<cv::Point>& contour,
                                      const VisionParams& params);

/// 0..1, how strongly the observation looks like a hand.
double confidenceFor(const HandObservation& observation, const VisionParams& params);

} // namespace vision
} // namespace communicake
