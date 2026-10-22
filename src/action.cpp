#include "communicake/action.hpp"

#include <cstdio>
#include <cstdlib>

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace communicake {
namespace {

/// std::system does not return the command's exit code on POSIX: it returns a
/// wait status whose low byte says how the child ended. Reporting that raw
/// value would print "exit code 256" for a command that exited with 1.
int exitCodeFromStatus(int status) {
#ifdef _WIN32
    return status;
#else
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return status;
#endif
}

} // namespace

ActionRunner::ActionRunner(bool dryRun) : dryRun_(dryRun) {}

void ActionRunner::setDryRun(bool dryRun) {
    dryRun_ = dryRun;
}

bool ActionRunner::dryRun() const {
    return dryRun_;
}

std::string ActionRunner::expand(const std::string& command, const std::string& platform) {
    const std::string placeholder = "{platform}";
    std::string result = command;
    std::string::size_type position = 0;
    while ((position = result.find(placeholder, position)) != std::string::npos) {
        result.replace(position, placeholder.size(), platform);
        position += platform.size();
    }
    return result;
}

bool ActionRunner::run(const std::string& command) {
    lastCommand_ = command;
    if (command.empty()) {
        lastExitCode_ = 0;
        return false;
    }
    if (dryRun_) {
        std::printf("[dry-run] %s\n", command.c_str());
        lastExitCode_ = 0;
        return true;
    }
    std::printf("[action] %s\n", command.c_str());
    std::fflush(stdout);
    lastExitCode_ = exitCodeFromStatus(std::system(command.c_str()));
    return lastExitCode_ == 0;
}

const std::string& ActionRunner::lastCommand() const {
    return lastCommand_;
}

int ActionRunner::lastExitCode() const {
    return lastExitCode_;
}

} // namespace communicake
