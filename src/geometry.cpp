#include "communicake/geometry.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>

namespace communicake::geometry {

double distance(const cv::Point& a, const cv::Point& b) {
    const double dx = static_cast<double>(a.x) - static_cast<double>(b.x);
    const double dy = static_cast<double>(a.y) - static_cast<double>(b.y);
    return std::sqrt(dx * dx + dy * dy);
}

double polygonArea(const std::vector<cv::Point>& points) {
    if (points.size() < 3) {
        return 0.0;
    }
    return std::abs(cv::contourArea(points));
}

int largestContourIndex(const std::vector<std::vector<cv::Point>>& contours, double minArea) {
    int best = -1;
    double bestArea = minArea;
    for (std::size_t i = 0; i < contours.size(); ++i) {
        const double area = polygonArea(contours[i]);
        if (area > bestArea) {
            bestArea = area;
            best = static_cast<int>(i);
        }
    }
    return best;
}

std::optional<cv::Point> centroid(const std::vector<cv::Point>& contour) {
    if (contour.size() < 3) {
        return std::nullopt;
    }
    const cv::Moments moments = cv::moments(contour);
    if (!(moments.m00 > 1e-9)) {
        return std::nullopt;
    }
    return cv::Point(cvRound(moments.m10 / moments.m00), cvRound(moments.m01 / moments.m00));
}

cv::Rect clampRect(const cv::Rect& rect, const cv::Size& frame) {
    if (frame.width <= 0 || frame.height <= 0) {
        return cv::Rect();
    }
    const int x1 = std::clamp(rect.x, 0, frame.width - 1);
    const int y1 = std::clamp(rect.y, 0, frame.height - 1);
    const int x2 = std::clamp(rect.x + rect.width, x1 + 1, frame.width);
    const int y2 = std::clamp(rect.y + rect.height, y1 + 1, frame.height);
    return cv::Rect(x1, y1, x2 - x1, y2 - y1);
}

} // namespace communicake::geometry
