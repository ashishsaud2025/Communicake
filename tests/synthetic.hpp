#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <cmath>

// Deterministic stand-ins for camera frames: a white hand on a black
// background, so the vision pipeline can be tested without a webcam.
namespace synthetic {

constexpr int kDefaultCanvas = 400;

inline cv::Mat blank(int size = kDefaultCanvas) {
    return cv::Mat::zeros(size, size, CV_8UC3);
}

inline cv::Point palmCentre(int size = kDefaultCanvas) {
    return cv::Point(size / 2, size / 2 + 20);
}

inline cv::Mat makeHand(int fingers, int size = kDefaultCanvas, double rotationDeg = 0.0) {
    cv::Mat canvas = blank(size);
    const cv::Point centre = palmCentre(size);
    const cv::Scalar white(255, 255, 255);

    cv::ellipse(canvas, centre, cv::Size(70, 55), 0, 0, 360, white, cv::FILLED, cv::LINE_AA);

    const double radius = 150.0;
    const double spacing = fingers >= 5 ? 25.0 : (fingers == 4 ? 28.0 : 30.0);
    for (int i = 0; i < fingers; ++i) {
        const double angleDeg = (i - (fingers - 1) / 2.0) * spacing;
        const double angle = angleDeg * 3.14159265358979323846 / 180.0;
        const cv::Point tip(cvRound(centre.x + radius * std::sin(angle)),
                            cvRound(centre.y - radius * std::cos(angle)));
        cv::line(canvas, centre, tip, white, 26, cv::LINE_AA);
        cv::circle(canvas, tip, 13, white, cv::FILLED, cv::LINE_AA);
    }

    if (rotationDeg != 0.0) {
        const cv::Mat matrix = cv::getRotationMatrix2D(centre, rotationDeg, 1.0);
        cv::warpAffine(canvas, canvas, matrix, canvas.size(), cv::INTER_LINEAR,
                       cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    }
    return canvas;
}

inline cv::Mat makeBlob(int size = kDefaultCanvas) {
    cv::Mat canvas = blank(size);
    cv::circle(canvas, cv::Point(size / 2, size / 2), 85, cv::Scalar(255, 255, 255), cv::FILLED,
               cv::LINE_AA);
    return canvas;
}

inline cv::Mat makeSpeckle(int size = kDefaultCanvas) {
    cv::Mat canvas = blank(size);
    for (int i = 0; i < 40; ++i) {
        cv::circle(canvas, cv::Point(20 + (i * 37) % (size - 40), 20 + (i * 53) % (size - 40)), 2,
                   cv::Scalar(255, 255, 255), cv::FILLED);
    }
    return canvas;
}

} // namespace synthetic
