#pragma once

#include <opencv2/core.hpp>

#include <optional>
#include <vector>

namespace communicake::geometry {

/// Euclidean distance between two points.
double distance(const cv::Point& a, const cv::Point& b);

/// Absolute area of a polygon, in square pixels.
double polygonArea(const std::vector<cv::Point>& points);

/// Index of the largest contour, or -1 when every contour is smaller than `minArea`.
int largestContourIndex(const std::vector<std::vector<cv::Point>>& contours, double minArea = 0.0);

/// Area-weighted centroid, or nullopt for contours with no area (fewer than three
/// points, collinear points, or a single repeated pixel).
std::optional<cv::Point> centroid(const std::vector<cv::Point>& contour);

/// Intersect `rect` with a `frame`-sized image. The result always has width and
/// height of at least one pixel and never extends outside the frame.
cv::Rect clampRect(const cv::Rect& rect, const cv::Size& frame);

} // namespace communicake::geometry
