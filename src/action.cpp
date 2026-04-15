#include "communicake/action.hpp"

#include <cstdio>
#include <cstdlib>

namespace communicake {

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
    lastExitCode_ = std::system(command.c_str());
    return lastExitCode_ == 0;
}

const std::string& ActionRunner::lastCommand() const {
    return lastCommand_;
}

int ActionRunner::lastExitCode() const {
    return lastExitCode_;
}

} // namespace communicake
