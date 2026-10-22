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

TEST(the_exit_code_is_the_commands_own_code) {
    // std::system returns a wait status on POSIX, so a naive implementation
    // reports 768 for this command. The runner must unwrap it.
    ActionRunner runner(false);
    CHECK(!runner.run("exit 3"));
    CHECK_EQ(runner.lastExitCode(), 3);
}

TEST(the_last_command_is_recorded_even_after_a_failure) {
    ActionRunner runner(false);
    runner.run("exit 4");
    CHECK_EQ(runner.lastCommand(), std::string("exit 4"));
}

TEST(dry_run_can_be_switched_on_and_off) {
    ActionRunner runner(false);
    CHECK(!runner.dryRun());
    runner.setDryRun(true);
    CHECK(runner.dryRun());
    CHECK(runner.run("exit 5")); // dry runs do not execute the command
    CHECK_EQ(runner.lastExitCode(), 0);
    runner.setDryRun(false);
    CHECK(!runner.run("exit 5"));
    CHECK_EQ(runner.lastExitCode(), 5);
}

TEST(only_the_exact_placeholder_is_expanded) {
    CHECK_EQ(ActionRunner::expand("{Platform}", "linux"), std::string("{Platform}"));
    CHECK_EQ(ActionRunner::expand("{platform }", "linux"), std::string("{platform }"));
    CHECK_EQ(ActionRunner::expand("{{platform}}", "linux"), std::string("{linux}"));
}

TEST_MAIN
