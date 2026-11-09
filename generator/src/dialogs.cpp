#include "dialogs.hpp"
#include "portable-file-dialogs.h"

#include <cstdlib>
#include <stdexcept>

namespace aura {
namespace fs = std::filesystem;

std::string choose_path(const std::string& title, const std::string& initial, bool file) {
#if defined(__linux__)
    bool available = false;
    const char* path = std::getenv("PATH");
    std::string paths = path ? path : "";
    std::size_t start = 0;
    while (true) {
        auto end = paths.find(':', start);
        auto dir = paths.substr(start, end - start);
        for (auto program : {"zenity", "matedialog", "qarma", "kdialog"}) {
            std::error_code ec;
            auto candidate = fs::path(dir.empty() ? "." : dir) / program;
            auto status = fs::status(candidate, ec);
            if (!ec && fs::is_regular_file(status) &&
                (status.permissions() & (fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec)) != fs::perms::none)
                available = true;
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
    if (!available) throw std::runtime_error("Install zenity or kdialog to browse, or type the path directly.");
#endif
    if (file) {
        auto result = pfd::open_file(title, initial).result();
        return result.empty() ? "" : result.front();
    }
    return pfd::select_folder(title, initial).result();
}

#ifdef _WIN32
fs::path windows_application_directory() {
    std::wstring path(32768, L'\0');
    DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!length || length == path.size()) throw std::runtime_error("Cannot determine the executable directory.");
    path.resize(length);
    return fs::path(path).parent_path();
}

std::string windows_home_directory() {
    const wchar_t* home = _wgetenv(L"USERPROFILE");
    return home ? fs::path(home).u8string() : "";
}
#endif
}
