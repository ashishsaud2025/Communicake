#include "communicake/frame_source.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace communicake {
namespace {

namespace fs = std::filesystem;

class CaptureSource : public FrameSource {
  public:
    CaptureSource(cv::VideoCapture capture, bool mirror, std::string description)
        : capture_(std::move(capture)), mirror_(mirror), description_(std::move(description)) {}

    bool next(cv::Mat& frame) override {
        if (!capture_.isOpened()) {
            return false;
        }
        if (!capture_.read(frame) || frame.empty()) {
            return false;
        }
        if (mirror_) {
            cv::flip(frame, frame, 1);
        }
        return true;
    }

    double fps() const override {
        const double fps = capture_.get(cv::CAP_PROP_FPS);
        return fps > 1.0 && fps < 1000.0 ? fps : 0.0;
    }

    std::string describe() const override {
        return description_;
    }

  private:
    cv::VideoCapture capture_;
    bool mirror_ = false;
    std::string description_;
};

class ImageSequenceSource : public FrameSource {
  public:
    ImageSequenceSource(std::vector<std::string> files, std::string directory)
        : files_(std::move(files)), directory_(std::move(directory)) {}

    bool next(cv::Mat& frame) override {
        while (index_ < files_.size()) {
            frame = cv::imread(files_[index_++], cv::IMREAD_COLOR);
            if (!frame.empty()) {
                return true;
            }
        }
        return false;
    }

    std::string describe() const override {
        return "image sequence in " + directory_ + " (" + std::to_string(files_.size()) + " frames)";
    }

  private:
    std::vector<std::string> files_;
    std::string directory_;
    std::size_t index_ = 0;
};

// A sort key that compares the numeric parts of a filename in value order
// (so frame_2 sorts before frame_10) and the text parts lexicographically.
// This makes image sequences captured without zero-padding play in the right
// order instead of the surprising lexicographic one.
std::vector<std::string> frameOrderKey(const fs::path& path) {
    std::string stem = path.stem().string();
    std::vector<std::string> key;
    std::string current;
    bool currentIsDigit = false;
    for (char c : stem) {
        bool isDigit = c >= '0' && c <= '9';
        if (isDigit != currentIsDigit && !current.empty()) {
            key.push_back(current);
            current.clear();
        }
        currentIsDigit = isDigit;
        current.push_back(c);
    }
    if (!current.empty()) {
        key.push_back(current);
    }
    return key;
}

bool frameLess(const std::string& a, const std::string& b) {
    const auto keyA = frameOrderKey(fs::path(a));
    const auto keyB = frameOrderKey(fs::path(b));
    const std::size_t n = std::min(keyA.size(), keyB.size());
    for (std::size_t i = 0; i < n; ++i) {
        const bool aDigit = !keyA[i].empty() && keyA[i].front() >= '0' &&
                            keyA[i].front() <= '9';
        const bool bDigit = !keyB[i].empty() && keyB[i].front() >= '0' &&
                            keyB[i].front() <= '9';
        if (aDigit && bDigit) {
            const long long ai = std::stoll(keyA[i]);
            const long long bi = std::stoll(keyB[i]);
            if (ai != bi) {
                return ai < bi;
            }
        } else if (keyA[i] != keyB[i]) {
            return keyA[i] < keyB[i];
        }
    }
    return keyA.size() < keyB.size();
}

bool hasImageExtension(const fs::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
           extension == ".bmp";
}

int backendToApi(CameraBackend backend) {
    switch (backend) {
        case CameraBackend::DirectShow:
            return cv::CAP_DSHOW;
        case CameraBackend::MediaFoundation:
            return cv::CAP_MSMF;
        case CameraBackend::V4L2:
            return cv::CAP_V4L2;
        case CameraBackend::Auto:
            break;
    }
    return cv::CAP_ANY;
}

} // namespace

bool parseCameraBackend(const std::string& name, CameraBackend& backend) {
    if (name.empty() || name == "any" || name == "auto") {
        backend = CameraBackend::Auto;
        return true;
    }
    if (name == "dshow" || name == "directshow") {
        backend = CameraBackend::DirectShow;
        return true;
    }
    if (name == "msmf" || name == "mediafoundation") {
        backend = CameraBackend::MediaFoundation;
        return true;
    }
    if (name == "v4l2") {
        backend = CameraBackend::V4L2;
        return true;
    }
    return false;
}

double FrameSource::fps() const {
    return 0.0;
}

bool isDirectory(const std::string& path) {
    std::error_code error;
    return !path.empty() && fs::is_directory(path, error);
}

std::unique_ptr<FrameSource> openWebcam(int index, int width, int height, bool mirror,
                                        CameraBackend backend) {
    cv::VideoCapture capture;
    bool opened = capture.open(index, backendToApi(backend));
#ifdef _WIN32
    if (!opened && backend == CameraBackend::Auto) {
        // OpenCV prefers Media Foundation on Windows, but some cameras expose
        // frames only through DirectShow.
        opened = capture.open(index, cv::CAP_DSHOW);
    }
#endif
    if (!opened || !capture.isOpened()) {
        return nullptr;
    }
    if (width > 0) {
        capture.set(cv::CAP_PROP_FRAME_WIDTH, width);
    }
    if (height > 0) {
        capture.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    }
    return std::make_unique<CaptureSource>(std::move(capture), mirror,
                                           "camera " + std::to_string(index));
}

std::unique_ptr<FrameSource> openVideoFile(const std::string& path) {
    std::error_code error;
    if (!fs::is_regular_file(path, error)) {
        return nullptr; // avoids a noisy backend probe for a missing file
    }
    cv::VideoCapture capture(path);
    if (!capture.isOpened()) {
        return nullptr;
    }
    return std::make_unique<CaptureSource>(std::move(capture), false, "video " + path);
}

std::unique_ptr<FrameSource> openImageSequence(const std::string& directory) {
    std::error_code error;
    if (!fs::is_directory(directory, error)) {
        return nullptr;
    }
    std::vector<std::string> files;
    for (const fs::directory_entry& entry : fs::directory_iterator(directory, error)) {
        if (entry.is_regular_file() && hasImageExtension(entry.path())) {
            files.push_back(entry.path().string());
        }
    }
    if (files.empty()) {
        return nullptr;
    }
    std::sort(files.begin(), files.end(), frameLess);
    return std::make_unique<ImageSequenceSource>(std::move(files), directory);
}

} // namespace communicake
