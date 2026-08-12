#include "communicake/action.hpp"

#include "test_harness.hpp"

using communicake::ActionRunner;

TEST(platform_placeholders_are_expanded) {
    CHECK_EQ(ActionRunner::expand("open {platform}://x", "windows"), std::string("open windows://x"));
    CHECK_EQ(ActionRunner::expand("{platform}-{platform}", "linux"), std::string("linux-linux"));
    CHECK_EQ(ActionRunner::expand("plain", "macos"), std::string("plain"));
}

TEST(a_dry_run_records_the_command_without_running_it) {
    ActionRunner runner(true);
    CHECK(runner.dryRun());
    CHECK(runner.run("definitely-not-a-real-command-communicake"));
    CHECK_EQ(runner.lastCommand(), std::string("definitely-not-a-real-command-communicake"));
    CHECK_EQ(runner.lastExitCode(), 0);
}

TEST(an_empty_command_does_nothing) {
    ActionRunner runner(false);
    CHECK(!runner.run(""));
}

TEST(a_real_command_reports_its_exit_code) {
    ActionRunner runner(false);
    CHECK(runner.run("echo communicake"));
    CHECK_EQ(runner.lastExitCode(), 0);
}

TEST(a_failing_command_reports_a_nonzero_exit_code) {
    ActionRunner runner(false);
    CHECK(!runner.run("exit 1"));
    CHECK(runner.lastExitCode() != 0);
}

TEST_MAIN
