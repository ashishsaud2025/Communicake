#include "communicake/observation_log.hpp"

#include "test_harness.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using communicake::csvQuote;
using communicake::ObservationLog;
using communicake::ObservationRecord;

namespace {

fs::path temporaryPath(const std::string& name) {
    return fs::temp_directory_path() / name;
}

std::vector<std::string> readLines(const fs::path& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}

} // namespace

TEST(plain_fields_are_not_quoted) {
    CHECK_EQ(csvQuote("three"), std::string("three"));
    CHECK_EQ(csvQuote(""), std::string(""));
    CHECK_EQ(csvQuote("start chrome"), std::string("start chrome"));
}

TEST(commas_quotes_and_spaces_are_quoted) {
    CHECK_EQ(csvQuote("echo one,two"), std::string("\"echo one,two\""));
    CHECK_EQ(csvQuote("say \"hi\""), std::string("\"say \"\"hi\"\"\""));
    CHECK_EQ(csvQuote(" padded"), std::string("\" padded\""));
    CHECK_EQ(csvQuote("two\nlines"), std::string("\"two\nlines\""));
}

TEST(a_log_writes_a_header_and_one_row_per_record) {
    const fs::path path = temporaryPath("communicake_log_basic.csv");
    std::string error;
    ObservationLog log;
    CHECK(log.open(path.string(), &error));
    CHECK(error.empty());
    CHECK(log.isOpen());

    log.write({1, false, 0, 0.0, 0.0, "none", ""});
    log.write({2, true, 3, 0.625, 18322.0, "three", "echo one,two"});
    log.close();
    CHECK(!log.isOpen());

    const std::vector<std::string> lines = readLines(path);
    CHECK_EQ(lines.size(), std::size_t(3));
    if (lines.size() == 3) {
        CHECK_EQ(lines[0], std::string("frame,hand,fingers,confidence,area,gesture,command"));
        CHECK_EQ(lines[1], std::string("1,0,0,0.000,0,none,"));
        CHECK_EQ(lines[2], std::string("2,1,3,0.625,18322,three,\"echo one,two\""));
    }
    fs::remove(path);
}

TEST(writing_to_a_closed_log_is_harmless) {
    ObservationLog log;
    log.write({7, true, 2, 0.5, 100.0, "two", ""});
    CHECK(!log.isOpen());
}

TEST(an_unwritable_log_path_is_reported) {
    std::string error;
    ObservationLog log;
    CHECK(!log.open("definitely-missing-directory/observations.csv", &error));
    CHECK(error.find("cannot write") != std::string::npos);
}

TEST_MAIN
