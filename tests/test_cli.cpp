#include "communicake/cli.hpp"

#include "test_harness.hpp"

using communicake::Options;
using communicake::parseArgs;

namespace {

Options parse(const std::vector<std::string>& arguments) {
    std::string error;
    const auto options = parseArgs(arguments, &error);
    CHECK(options.has_value());
    CHECK(error.empty());
    return options.value_or(Options());
}

} // namespace

TEST(the_defaults_are_a_plain_camera_run) {
    const Options options = parse({});
    CHECK_EQ(options.camera, 0);
    CHECK(!options.headless);
    CHECK(!options.dryRun);
    CHECK(options.input.empty());
}

TEST(every_documented_option_is_parsed) {
    const Options options = parse({"--camera", "2",   "--input",  "clip.mp4", "--config", "c.json",
                                   "--profile", "p.json", "--dump-mask", "masks", "--log",
                                   "observations.csv", "--width", "640", "--height", "480",
                                   "--headless", "--dry-run", "--verbose"});
    CHECK_EQ(options.camera, 2);
    CHECK_EQ(options.input, std::string("clip.mp4"));
    CHECK_EQ(options.configPath, std::string("c.json"));
    CHECK_EQ(options.profilePath, std::string("p.json"));
    CHECK_EQ(options.dumpMaskDir, std::string("masks"));
    CHECK_EQ(options.logPath, std::string("observations.csv"));
    CHECK_EQ(options.width, 640);
    CHECK_EQ(options.height, 480);
    CHECK(options.headless);
    CHECK(options.dryRun);
    CHECK(options.verbose);
}

TEST(equals_syntax_is_accepted) {
    const Options options = parse({"--camera=1", "--width=1280"});
    CHECK_EQ(options.camera, 1);
    CHECK_EQ(options.width, 1280);
}

TEST(help_and_version_are_flags) {
    CHECK(parse({"-h"}).help);
    CHECK(parse({"--help"}).help);
    CHECK(parse({"--version"}).version);
}

TEST(unknown_options_are_rejected) {
    std::string error;
    CHECK(!parseArgs({"--turbo"}, &error).has_value());
    CHECK(error.find("--turbo") != std::string::npos);
}

TEST(missing_values_are_rejected) {
    std::string error;
    CHECK(!parseArgs({"--camera"}, &error).has_value());
    CHECK(error.find("missing value") != std::string::npos);
}

TEST(non_numeric_values_are_rejected) {
    std::string error;
    CHECK(!parseArgs({"--camera", "front"}, &error).has_value());
    CHECK(error.find("whole number") != std::string::npos);
    CHECK(!parseArgs({"--width", "-4"}, &error).has_value());
}

TEST(a_benchmark_needs_a_positive_frame_count) {
    const Options options = parse({"--benchmark", "120"});
    CHECK_EQ(options.benchmark, 120);

    std::string error;
    CHECK(!parseArgs({"--benchmark", "0"}, &error).has_value());
    CHECK(error.find("at least one frame") != std::string::npos);
    CHECK(!parseArgs({"--benchmark"}, &error).has_value());
}

TEST_MAIN
