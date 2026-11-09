#pragma once

#include <filesystem>
#include <string>

namespace aura {
std::string choose_path(const std::string& title, const std::string& initial, bool file);
#ifdef _WIN32
std::filesystem::path windows_application_directory();
std::string windows_home_directory();
#endif
}
