#include "communicake/observation_log.hpp"

#include <cstdio>

namespace communicake {

ObservationLog::~ObservationLog() {
    close();
}

bool ObservationLog::open(const std::string& path, std::string* error) {
    close();
    file_.open(path, std::ios::out | std::ios::trunc);
    if (!file_) {
        if (error != nullptr) {
            *error = "cannot write " + path;
        }
        return false;
    }
    file_ << "frame,hand,fingers,confidence,area,gesture,command\n";
    if (!file_) {
        if (error != nullptr) {
            *error = "failed while writing the header to " + path;
        }
        close();
        return false;
    }
    return true;
}

void ObservationLog::close() {
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

bool ObservationLog::isOpen() const {
    return file_.is_open();
}

void ObservationLog::write(const ObservationRecord& record) {
    if (!file_.is_open()) {
        return;
    }
    char numbers[160];
    std::snprintf(numbers, sizeof(numbers), "%zu,%d,%d,%.3f,%.0f,", record.frame,
                  record.handVisible ? 1 : 0, record.fingerCount, record.confidence, record.area);
    file_ << numbers << csvQuote(record.gesture) << ',' << csvQuote(record.command) << '\n';
}

std::string csvQuote(const std::string& field) {
    const bool needsQuotes =
        field.find_first_of(",\"\r\n") != std::string::npos ||
        (!field.empty() && (field.front() == ' ' || field.back() == ' '));
    if (!needsQuotes) {
        return field;
    }
    std::string quoted = "\"";
    for (char character : field) {
        if (character == '"') {
            quoted += "\"\"";
        } else {
            quoted += character;
        }
    }
    quoted += '"';
    return quoted;
}

} // namespace communicake
