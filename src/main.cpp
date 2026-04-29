#include "communicake/app.hpp"
#include "communicake/cli.hpp"
#include "communicake/config.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

int main(int argc, char** argv) {
    using namespace communicake;

    std::string error;
    const auto options = parseArgs(argc, argv, &error);
    if (!options.has_value()) {
        std::fprintf(stderr, "error: %s\n\n%s", error.c_str(), usage().c_str());
        return 2;
    }
    if (options->help) {
        std::printf("%s", usage().c_str());
        return 0;
    }
    if (options->version) {
        std::printf("%s\n", versionString().c_str());
        return 0;
    }

    AppConfig config = AppConfig::defaults();
    std::string configPath = options->configPath;
    if (configPath.empty() && std::filesystem::exists("config/actions.json")) {
        configPath = "config/actions.json";
    }
    if (!configPath.empty()) {
        const auto loaded = AppConfig::load(configPath, &error);
        if (!loaded.has_value()) {
            std::fprintf(stderr, "error: %s\n", error.c_str());
            return 2;
        }
        config = *loaded;
    }

    App app(*options, config);
    return app.run();
}
