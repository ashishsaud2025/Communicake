#pragma once

#include <opencv2/core.hpp>

#include <vector>

namespace communicake {

/// How the hand is separated from the background.
enum class SegmentationMode { Binary = 0, Hsv = 1 };

/// Everything the rest of the program needs to know about one processed frame.
struct HandObservation {
    bool valid = false;       ///< a hand-sized contour was found
    int fingerCount = 0;      ///< number of detected fingertip points
    double confidence = 0.0;  ///< 0..1, how strongly the contour looks like a hand
    cv::Point centroid{0, 0}; ///< contour centroid, in ROI-local coordinates
    std::vector<cv::Point> fingertips;
    std::vector<cv::Point> hull;
    double area = 0.0;        ///< contour area in pixels
};

/// Wall-clock cost of the stages of one process() call.
struct FrameTimings {
    double segmentMs = 0.0; ///< colour conversion, subtraction and morphology
    double contourMs = 0.0; ///< contour search, hull, fingertips and confidence
    double totalMs = 0.0;   ///< segmentMs + contourMs
};

} // namespace communicake
