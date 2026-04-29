#include "communicake/ui.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <string>

namespace communicake {
namespace {

const char* kControls = "Controls";
const char* kRoiControls = "ROI Controls";
const char* kHsvControls = "HSV VALUES";
const char* kOriginal = "Original";
const char* kMaskWindow = "Mask";
const char* kRoiWindow = "ROI";

} // namespace

Ui::Ui(VisionParams* params) : params_(params) {}

Ui::~Ui() {
    destroy();
}

void Ui::create() {
    if (created_) {
        return;
    }
    cv::namedWindow(kOriginal, cv::WINDOW_NORMAL);
    cv::resizeWindow(kOriginal, 640, 480);
    cv::moveWindow(kOriginal, 800, 60);
    cv::namedWindow(kMaskWindow, cv::WINDOW_NORMAL);
    cv::resizeWindow(kMaskWindow, 320, 240);
    cv::moveWindow(kMaskWindow, 400, 400);
    cv::namedWindow(kRoiWindow, cv::WINDOW_NORMAL);
    cv::resizeWindow(kRoiWindow, 320, 240);
    cv::moveWindow(kRoiWindow, 60, 400);

    cv::namedWindow(kControls, cv::WINDOW_NORMAL);
    cv::resizeWindow(kControls, 320, 90);
    cv::moveWindow(kControls, 400, 60);
    cv::createTrackbar("Mode", kControls, nullptr, 1);
    cv::createTrackbar("Sens", kControls, nullptr, 255);
    cv::createTrackbar("Finger D", kControls, nullptr, 200);

    cv::namedWindow(kRoiControls, cv::WINDOW_NORMAL);
    cv::resizeWindow(kRoiControls, 320, 110);
    cv::moveWindow(kRoiControls, 60, 60);
    cv::createTrackbar("X", kRoiControls, nullptr, 4000);
    cv::createTrackbar("Y", kRoiControls, nullptr, 4000);
    cv::createTrackbar("WIDTH", kRoiControls, nullptr, 4000);
    cv::createTrackbar("HEIGHT", kRoiControls, nullptr, 4000);

    created_ = true;
    syncFromParams();
    updateModeWindow();
}

void Ui::syncFromParams() {
    if (!created_ || params_ == nullptr) {
        return;
    }
    cv::setTrackbarPos("Mode", kControls, params_->mode == SegmentationMode::Hsv ? 1 : 0);
    cv::setTrackbarPos("Sens", kControls, std::clamp(params_->binaryThreshold, 0, 255));
    cv::setTrackbarPos("Finger D", kControls, std::clamp(params_->fingertipDistance, 0, 200));
    cv::setTrackbarPos("X", kRoiControls, params_->roi.x);
    cv::setTrackbarPos("Y", kRoiControls, params_->roi.y);
    cv::setTrackbarPos("WIDTH", kRoiControls, params_->roi.width);
    cv::setTrackbarPos("HEIGHT", kRoiControls, params_->roi.height);
    if (hsvWindowOpen_) {
        cv::setTrackbarPos("HMIN", kHsvControls, static_cast<int>(params_->hsvMin[0]));
        cv::setTrackbarPos("SMIN", kHsvControls, static_cast<int>(params_->hsvMin[1]));
        cv::setTrackbarPos("VMIN", kHsvControls, static_cast<int>(params_->hsvMin[2]));
        cv::setTrackbarPos("HMAX", kHsvControls, static_cast<int>(params_->hsvMax[0]));
        cv::setTrackbarPos("SMAX", kHsvControls, static_cast<int>(params_->hsvMax[1]));
        cv::setTrackbarPos("VMAX", kHsvControls, static_cast<int>(params_->hsvMax[2]));
    }
}

void Ui::readIntoParams() {
    if (!created_ || params_ == nullptr) {
        return;
    }
    const int mode = cv::getTrackbarPos("Mode", kControls);
    params_->mode = mode == 1 ? SegmentationMode::Hsv : SegmentationMode::Binary;
    params_->binaryThreshold = cv::getTrackbarPos("Sens", kControls);
    params_->fingertipDistance = cv::getTrackbarPos("Finger D", kControls);
    params_->roi.x = cv::getTrackbarPos("X", kRoiControls);
    params_->roi.y = cv::getTrackbarPos("Y", kRoiControls);
    params_->roi.width = cv::getTrackbarPos("WIDTH", kRoiControls);
    params_->roi.height = cv::getTrackbarPos("HEIGHT", kRoiControls);
    if (hsvWindowOpen_) {
        params_->hsvMin = cv::Scalar(cv::getTrackbarPos("HMIN", kHsvControls),
                                     cv::getTrackbarPos("SMIN", kHsvControls),
                                     cv::getTrackbarPos("VMIN", kHsvControls));
        params_->hsvMax = cv::Scalar(cv::getTrackbarPos("HMAX", kHsvControls),
                                     cv::getTrackbarPos("SMAX", kHsvControls),
                                     cv::getTrackbarPos("VMAX", kHsvControls));
    }
}

void Ui::updateModeWindow() {
    if (!created_ || params_ == nullptr) {
        return;
    }
    const bool wantHsv = params_->mode == SegmentationMode::Hsv;
    if (wantHsv && !hsvWindowOpen_) {
        cv::namedWindow(kHsvControls, cv::WINDOW_NORMAL);
        cv::resizeWindow(kHsvControls, 320, 160);
        cv::moveWindow(kHsvControls, 740, 560);
        cv::createTrackbar("HMIN", kHsvControls, nullptr, 255);
        cv::createTrackbar("SMIN", kHsvControls, nullptr, 255);
        cv::createTrackbar("VMIN", kHsvControls, nullptr, 255);
        cv::createTrackbar("HMAX", kHsvControls, nullptr, 255);
        cv::createTrackbar("SMAX", kHsvControls, nullptr, 255);
        cv::createTrackbar("VMAX", kHsvControls, nullptr, 255);
        hsvWindowOpen_ = true;
        syncFromParams();
    } else if (!wantHsv && hsvWindowOpen_) {
        cv::destroyWindow(kHsvControls);
        hsvWindowOpen_ = false;
    }
}

void Ui::show(const cv::Mat& frame, const cv::Mat& mask, const cv::Mat& roiView,
              const HandObservation& observation, Gesture gesture, double fps) {
    if (!created_) {
        return;
    }
    cv::Mat canvas = frame.clone();
    const cv::Rect roi = params_ != nullptr ? params_->roi : cv::Rect();
    cv::rectangle(canvas, roi, cv::Scalar(0, 255, 0), 2);

    const cv::Point offset = roi.tl();
    for (const cv::Point& tip : observation.fingertips) {
        cv::circle(canvas, tip + offset, 6, cv::Scalar(0, 0, 255), -1);
        cv::line(canvas, observation.centroid + offset, tip + offset, cv::Scalar(0, 200, 255), 2);
    }
    if (observation.valid) {
        cv::circle(canvas, observation.centroid + offset, 4, cv::Scalar(255, 128, 0), -1);
    }

    const std::string label = std::string("gesture: ") + gestureName(gesture);
    cv::putText(canvas, label, cv::Point(12, 32), cv::FONT_HERSHEY_SIMPLEX, 0.8,
                cv::Scalar(0, 0, 255), 2);
    const std::string counters =
        "fingers " + std::to_string(observation.fingerCount) + "  " +
        (fps > 0.0 ? std::to_string(static_cast<int>(fps + 0.5)) + " fps" : std::string("-- fps"));
    cv::putText(canvas, counters, cv::Point(12, 62), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                cv::Scalar(30, 220, 30), 1);

    cv::imshow(kOriginal, canvas);
    if (!mask.empty()) {
        cv::imshow(kMaskWindow, mask);
    }
    if (!roiView.empty()) {
        cv::imshow(kRoiWindow, roiView);
    }
}

int Ui::waitKey(int milliseconds) {
    return created_ ? cv::waitKey(milliseconds) : -1;
}

void Ui::destroy() {
    if (!created_) {
        return;
    }
    cv::destroyAllWindows();
    created_ = false;
    hsvWindowOpen_ = false;
}

} // namespace communicake
