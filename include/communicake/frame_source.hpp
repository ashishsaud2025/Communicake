#pragma once

#include <opencv2/core.hpp>

#include <memory>
#include <string>

namespace communicake {

/// Produces frames. Independent of where they come from, so the application
/// loop can be driven by a webcam, a recording or a directory of images.
class FrameSource {
  public:
    virtual ~FrameSource() = default;

    /// Read the next frame. Returns false at the end of the stream.
    virtual bool next(cv::Mat& frame) = 0;

    virtual double fps() const;
    virtual std::string describe() const = 0;
};

/// Camera capture backend. `Auto` lets OpenCV pick, and falls back to the
/// platform default when the first choice fails.
enum class CameraBackend { Auto = 0, DirectShow, MediaFoundation, V4L2 };

/// Parse a backend name (`any`, `dshow`, `msmf`, `v4l2`). Returns false for an
/// unknown name.
bool parseCameraBackend(const std::string& name, CameraBackend& backend);

/// Webcam by index. Returns nullptr when the device cannot be opened.
std::unique_ptr<FrameSource> openWebcam(int index, int width = 0, int height = 0,
                                        bool mirror = true,
                                        CameraBackend backend = CameraBackend::Auto);

/// A video file. Returns nullptr when the file cannot be opened.
std::unique_ptr<FrameSource> openVideoFile(const std::string& path);

/// Every *.png, *.jpg, *.jpeg and *.bmp in a directory, in name order.
std::unique_ptr<FrameSource> openImageSequence(const std::string& directory);

/// True when `path` names an existing directory.
bool isDirectory(const std::string& path);

} // namespace communicake
