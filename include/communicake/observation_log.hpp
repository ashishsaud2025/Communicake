#pragma once

#include <cstddef>
#include <fstream>
#include <string>

namespace communicake {

/// One line of the observation log.
struct ObservationRecord {
    std::size_t frame = 0;
    bool handVisible = false;
    int fingerCount = 0;
    double confidence = 0.0;
    double area = 0.0;
    std::string gesture; ///< gesture the frame was counted as, "none" otherwise
    std::string command; ///< action launched for this frame, empty when none
};

/// Appends observations to a CSV file, one row per processed frame. Useful for
/// replaying a session offline when the gesture did not do what was expected.
class ObservationLog {
  public:
    ObservationLog() = default;
    ~ObservationLog();

    ObservationLog(const ObservationLog&) = delete;
    ObservationLog& operator=(const ObservationLog&) = delete;

    /// Create or truncate `path`, then write the header row.
    bool open(const std::string& path, std::string* error);
    void close();

    bool isOpen() const;
    void write(const ObservationRecord& record);

  private:
    std::ofstream file_;
};

/// Quote a field so that commas, quotes and newlines survive a round trip
/// through a spreadsheet. Exposed for the tests.
std::string csvQuote(const std::string& field);

} // namespace communicake
