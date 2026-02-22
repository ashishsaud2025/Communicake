#include "communicake/vision.hpp"

#include "communicake/geometry.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace communicake {
namespace {

int oddAtLeastThree(int value) {
    int v = std::max(3, value);
    if (v % 2 == 0) {
        ++v;
    }
    return v;
}

} // namespace

Segmenter::Segmenter(const VisionParams& params) : params_(params) {}

void Segmenter::setParams(const VisionParams& params) {
    params_ = params;
}

const VisionParams& Segmenter::params() const {
    return params_;
}

bool Segmenter::setBackground(const cv::Mat& frame) {
    if (frame.empty()) {
        return false;
    }
    const cv::Rect roi = geometry::clampRect(params_.roi, frame.size());
    cv::Mat gray;
    cv::cvtColor(frame(roi), gray, cv::COLOR_BGR2GRAY);
    const int k = oddAtLeastThree(params_.blurKernel);
    cv::GaussianBlur(gray, gray, cv::Size(k, k), 0);
    background_ = gray.clone();
    hasBackground_ = true;
    return true;
}

bool Segmenter::hasBackground() const {
    return hasBackground_;
}

cv::Mat Segmenter::segment(const cv::Mat& roiBgr) const {
    cv::Mat mask;
    if (roiBgr.empty()) {
        return mask;
    }

    if (params_.mode == SegmentationMode::Hsv) {
        cv::Mat hsv;
        cv::cvtColor(roiBgr, hsv, cv::COLOR_BGR2HSV);
        cv::inRange(hsv, params_.hsvMin, params_.hsvMax, mask);
    } else {
        // Without a background model there is nothing to subtract; return an
        // empty mask rather than pretending the whole frame is a hand.
        if (!hasBackground_) {
            return cv::Mat::zeros(roiBgr.size(), CV_8UC1);
        }
        cv::Mat gray;
        cv::cvtColor(roiBgr, gray, cv::COLOR_BGR2GRAY);
        const int k = oddAtLeastThree(params_.blurKernel);
        cv::GaussianBlur(gray, gray, cv::Size(k, k), 0);

        cv::Mat background;
        if (background_.size() == gray.size()) {
            background = background_;
        } else {
            // The ROI moved or was resized since the background was captured.
            cv::resize(background_, background, gray.size(), 0, 0, cv::INTER_LINEAR);
        }
        cv::Mat difference;
        cv::absdiff(gray, background, difference);
        cv::threshold(difference, mask, params_.binaryThreshold, 255, cv::THRESH_BINARY);
    }

    const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);  // drop speckle
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel); // fill pinholes
    return mask;
}

HandPipeline::HandPipeline(const VisionParams& params) : params_(params), segmenter_(params) {}

void HandPipeline::setBackground(const cv::Mat& frame) {
    segmenter_.setParams(params_);
    segmenter_.setBackground(frame);
}

void HandPipeline::setParams(const VisionParams& params) {
    params_ = params;
    segmenter_.setParams(params);
}

const VisionParams& HandPipeline::params() const {
    return params_;
}

const cv::Mat& HandPipeline::lastMask() const {
    return mask_;
}

const cv::Mat& HandPipeline::lastRoiView() const {
    return roiView_;
}

cv::Rect HandPipeline::lastRoi() const {
    return lastRoi_;
}

HandObservation HandPipeline::process(const cv::Mat& frame) {
    HandObservation observation;
    if (frame.empty()) {
        return observation;
    }

    lastRoi_ = geometry::clampRect(params_.roi, frame.size());
    roiView_ = frame(lastRoi_).clone();
    mask_ = segmenter_.segment(roiView_);
    if (mask_.empty()) {
        return observation;
    }

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask_, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    const int index = geometry::largestContourIndex(contours, params_.minContourArea);
    if (index < 0) {
        return observation;
    }

    const std::vector<cv::Point>& contour = contours[static_cast<std::size_t>(index)];
    const auto centre = geometry::centroid(contour);
    if (!centre.has_value()) {
        return observation;
    }

    observation.valid = true;
    observation.centroid = *centre;
    observation.area = geometry::polygonArea(contour);
    cv::convexHull(contour, observation.hull);
    observation.fingertips = vision::findFingertips(contour, params_);
    observation.fingerCount = static_cast<int>(observation.fingertips.size());
    observation.confidence = vision::confidenceFor(observation, params_);
    return observation;
}

namespace vision {

std::vector<cv::Point> findFingertips(const std::vector<cv::Point>& contour,
                                      const VisionParams& params) {
    std::vector<cv::Point> fingertips;
    if (contour.size() < 5) {
        return fingertips;
    }
    const auto centre = geometry::centroid(contour);
    if (!centre.has_value()) {
        return fingertips;
    }

    std::vector<int> hullIndices;
    cv::convexHull(contour, hullIndices, false, false); // indices into `contour`
    if (hullIndices.size() < 4) {
        return fingertips;
    }

    std::vector<cv::Vec4i> defects;
    cv::convexityDefects(contour, hullIndices, defects);

    // A fingertip is a hull vertex that borders a valley deep enough to be the
    // gap between two fingers. The valley depth is what separates a hand from a
    // smooth blob such as a fist.
    std::vector<double> valleyDepth(contour.size(), 0.0);
    for (const cv::Vec4i& defect : defects) {
        const double depth = defect[3] / 256.0;
        if (depth < params.defectDepth) {
            continue;
        }
        valleyDepth[static_cast<std::size_t>(defect[0])] =
            std::max(valleyDepth[static_cast<std::size_t>(defect[0])], depth);
        valleyDepth[static_cast<std::size_t>(defect[1])] =
            std::max(valleyDepth[static_cast<std::size_t>(defect[1])], depth);
    }

    struct Candidate {
        cv::Point point;
        double distance = 0.0;
    };
    std::vector<Candidate> candidates;
    for (int index : hullIndices) {
        const std::size_t i = static_cast<std::size_t>(index);
        if (valleyDepth[i] <= 0.0) {
            continue;
        }
        const cv::Point& point = contour[i];
        if (point.y >= centre->y) {
            continue; // below the centroid: palm or wrist, never a fingertip
        }
        const double distance = geometry::distance(point, *centre);
        if (distance < params.fingertipDistance) {
            continue;
        }
        candidates.push_back({point, distance});
    }

    if (candidates.empty()) {
        // A single extended finger can leave only a shallow valley. Fall back to
        // a protrusion test: one hull point much farther out than the rest.
        std::vector<double> hullDistances;
        hullDistances.reserve(hullIndices.size());
        for (int index : hullIndices) {
            hullDistances.push_back(
                geometry::distance(contour[static_cast<std::size_t>(index)], *centre));
        }
        const auto middle = hullDistances.begin() + hullDistances.size() / 2;
        std::nth_element(hullDistances.begin(), middle, hullDistances.end());
        const double median = *middle;
        const auto farthest = std::max_element(hullDistances.begin(), hullDistances.end());
        if (median > 0.0 && *farthest > params.fingertipDistance &&
            *farthest > 1.55 * median) {
            const std::size_t index = static_cast<std::size_t>(
                std::distance(hullDistances.begin(), farthest));
            candidates.push_back({contour[static_cast<std::size_t>(hullIndices[index])],
                                  *farthest});
        }
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b) { return a.distance > b.distance; });

    for (const Candidate& candidate : candidates) {
        if (fingertips.size() >= 5) {
            break;
        }
        bool tooClose = false;
        for (const cv::Point& accepted : fingertips) {
            if (geometry::distance(candidate.point, accepted) < params.minFingerGap) {
                tooClose = true;
                break;
            }
        }
        if (!tooClose) {
            fingertips.push_back(candidate.point);
        }
    }
    return fingertips;
}

double confidenceFor(const HandObservation& observation, const VisionParams& params) {
    if (!observation.valid || observation.hull.empty()) {
        return 0.0;
    }
    const double hullArea = geometry::polygonArea(observation.hull);
    const double solidity =
        hullArea > 0.0 ? std::clamp(observation.area / hullArea, 0.0, 1.0) : 0.0;

    double prominence = 0.0;
    if (!observation.fingertips.empty()) {
        double total = 0.0;
        for (const cv::Point& tip : observation.fingertips) {
            total += geometry::distance(tip, observation.centroid);
        }
        const double average = total / static_cast<double>(observation.fingertips.size());
        prominence = std::clamp(
            (average - params.fingertipDistance) / (params.fingertipDistance * 1.2), 0.0, 1.0);
    }
    return std::clamp(0.5 * solidity + 0.5 * prominence, 0.0, 1.0);
}

} // namespace vision
} // namespace communicake
