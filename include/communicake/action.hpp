#pragma once

#include <string>

namespace communicake {

/// Resolves and launches the command bound to a gesture.
class ActionRunner {
  public:
    explicit ActionRunner(bool dryRun = false);

    void setDryRun(bool dryRun);
    bool dryRun() const;

    /// Replace supported placeholders in a command template:
    ///   {platform} -> "windows", "linux", "macos" or "unknown"
    static std::string expand(const std::string& command, const std::string& platform);

    /// Run a resolved command. An empty command is not an error, it is simply
    /// nothing to do; in dry-run mode the command is printed instead of run.
    bool run(const std::string& command);

    const std::string& lastCommand() const;
    int lastExitCode() const;

  private:
    bool dryRun_ = false;
    std::string lastCommand_;
    int lastExitCode_ = 0;
};

} // namespace communicake
