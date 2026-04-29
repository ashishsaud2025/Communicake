#pragma once

#include "communicake/gesture.hpp"
#include "communicake/types.hpp"
#include "communicake/vision.hpp"

#include <opencv2/core.hpp>

namespace communicake {

/// All highgui use lives here. Nothing else in the program opens a window.
class Ui {
  public:
    explicit Ui(VisionParams* params);
    ~Ui();

    Ui(const Ui&) = delete;
    Ui& operator=(const Ui&) = delete;

    void create();
    void readIntoParams();
    void syncFromParams();

    /// Show or hide the HSV trackbar window to match the segmentation mode.
    void updateModeWindow();

    void show(const cv::Mat& frame, const cv::Mat& mask, const cv::Mat& roiView,
              const HandObservation& observation, Gesture gesture, double fps);

    int waitKey(int milliseconds);
    void destroy();

  private:
    VisionParams* params_;
    bool created_ = false;
    bool hsvWindowOpen_ = false;
};

} // namespace communicake
