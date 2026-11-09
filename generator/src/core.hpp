#pragma once

#include <filesystem>
#include <string>

namespace aura {
namespace fs = std::filesystem;

struct Options {
    std::string name;
    std::string application_id;
    fs::path parent;
    bool android = false;
    bool ios = false;
    bool desktop = false;
    bool web = false;
    fs::path android_sdk;
    fs::path emsdk;
    fs::path python;
};

void validate(const Options& options);
void generate(const Options& options, const fs::path& distribution);
}
