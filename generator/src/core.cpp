#include "core.hpp"

#include <codecvt>
#include <fstream>
#include <iomanip>
#include <locale>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cerrno>
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;
#endif

namespace aura {
namespace {
std::string read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot read: " + path.u8string());
    std::ostringstream result;
    result << in.rdbuf();
    if (in.bad()) throw std::runtime_error("Cannot read: " + path.u8string());
    return result.str();
}

void write(const fs::path& path, const std::string& value) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out || !(out << value)) throw std::runtime_error("Cannot write: " + path.u8string());
    out.close();
    if (!out) throw std::runtime_error("Cannot finish writing: " + path.u8string());
}

void replace_all(std::string& text, const std::string& from, const std::string& to) {
    std::size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
}

std::string kotlin_string(const std::string& value) {
    std::string out;
    for (char c : value) {
        if (c == '\\' || c == '"' || c == '$') out += '\\';
        if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else out += c;
    }
    return out;
}

std::string property_string(const std::string& value) {
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
    std::ostringstream out;
    for (char16_t c : converter.from_bytes(value)) {
        if (c < 0x20 || c > 0x7e) {
            out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
        } else {
            if (c == '\\' || c == ' ' || c == ':' || c == '=' || c == '#' || c == '!') out << '\\';
            out << static_cast<char>(c);
        }
    }
    return out.str();
}

bool python_supported(const fs::path& python) {
    const std::string script = "import sys; sys.exit(0 if sys.version_info >= (3,10) else 1)";
#ifdef _WIN32
    std::wstring command = L"\"" + python.wstring() + L"\" -c \"import sys; sys.exit(0 if sys.version_info >= (3,10) else 1)\"";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(python.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                        nullptr, nullptr, &startup, &process))
        throw std::runtime_error("Cannot start Python: " + python.u8string());
    DWORD wait = WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 1;
    bool ok = wait == WAIT_OBJECT_0 && GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (!ok) throw std::runtime_error("Cannot wait for Python: " + python.u8string());
    return code == 0;
#else
    std::string executable = python.string();
    char* args[] = {executable.data(), const_cast<char*>("-c"), const_cast<char*>(script.c_str()), nullptr};
    pid_t pid;
    int error = posix_spawn(&pid, executable.c_str(), nullptr, nullptr, args, environ);
    if (error != 0) throw std::runtime_error("Cannot start Python: " + python.u8string() + " (" + std::to_string(error) + ")");
    int status;
    pid_t result;
    do { result = waitpid(pid, &status, 0); } while (result == -1 && errno == EINTR);
    if (result == -1) throw std::runtime_error("Cannot wait for Python: " + python.u8string());
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
#endif
}

bool has_file(const fs::path& a, const fs::path& b) {
    return fs::is_regular_file(a) || fs::is_regular_file(b);
}

std::string documentation(const Options& o) {
    std::string text = "# " + o.name + "\n\nA starter project.\n\n## Build\n";
    if (o.desktop) text += "\n### Desktop\nRequires CMake 3.28+ and a native compiler.\n"
        "- Run: `./gradlew :app:run`\n- Build only: `./gradlew :app:desktopBuild`\n"
        "- Windows cross-build (Linux/macOS): `./gradlew :app:windowsBuild` (requires MinGW-w64)\n";
    if (o.android) text += "\n### Android\nRequires Android Studio with the Android SDK, NDK, and CMake installed. The SDK path is saved in `local.properties`.\n"
        "- Build: `./gradlew :app:assembleDebug`\n";
    if (o.ios) text += "\n### iOS\nRequires macOS, Xcode, and CMake 3.28+.\n"
        "- Build: `./gradlew :app:iosBuild`\n"
        "- Run: `./gradlew :app:iosRun` (set `MY_IOS_UDID` in `local.properties`; currently also requires Android to be selected)\n";
    if (o.web) text += "\n### Web\nRequires an installed Emscripten SDK, Python 3.10+, and CMake. Set the SDK and Python paths in `app/build.gradle.kts`.\n"
        "- Build: `./gradlew :app:htmlBuild`\n";
    return text + "\n## License\nAura's original framework and starter code is licensed under the Apache License 2.0; see `LICENSE`. "
        "Third-party components and assets may have different terms; review their licenses before distributing your project.\n";
}

void update(const fs::path& path, const std::string& from, const std::string& to) {
    auto content = read(path);
    replace_all(content, from, to);
    write(path, content);
}
}

void validate(const Options& o) {
    if (o.android) {
        if (!fs::is_directory(o.android_sdk))
            throw std::runtime_error("Select an existing Android SDK directory.");
        if (!has_file(o.android_sdk / "platform-tools/adb", o.android_sdk / "platform-tools/adb.exe") ||
            !fs::is_directory(o.android_sdk / "platforms"))
            throw std::runtime_error("The selected directory is not an Android SDK. Choose the root containing platform-tools and platforms.");
    }
    if (!std::regex_match(o.name, std::regex("[A-Za-z][A-Za-z0-9_-]{0,63}")))
        throw std::runtime_error("Project name must start with a letter and contain only letters, digits, - and _ (maximum 64 characters).");
    const std::set<std::string> reserved = {"raylib", "native_app_glue", "glfw", "all", "clean", "help", "install",
        "test", "package", "rebuild_cache", "edit_cache", "list_install_components", "ZERO_CHECK", "ALL_BUILD", "RUN_TESTS"};
    if (reserved.count(o.name)) throw std::runtime_error("Project name conflicts with a native build target: " + o.name);
    if (!std::regex_match(o.application_id, std::regex("[a-z][a-z0-9_]*(\\.[a-z][a-z0-9_]*)+")))
        throw std::runtime_error("Application ID must contain lowercase identifier segments, for example com.example.game.");
    if (!o.android && !o.ios && !o.desktop && !o.web)
        throw std::runtime_error("Select at least one platform.");
    if (!fs::is_directory(o.parent)) throw std::runtime_error("The output parent directory must exist.");
    if (fs::exists(o.parent / o.name)) throw std::runtime_error("Output already exists: " + (o.parent / o.name).u8string());
    if (o.web) {
        if (!fs::is_directory(o.emsdk)) throw std::runtime_error("Select an existing Emscripten SDK directory.");
        if (!has_file(o.emsdk / "emsdk_env.sh", o.emsdk / "emsdk_env.bat") ||
            !has_file(o.emsdk / "upstream/emscripten/emcc", o.emsdk / "upstream/emscripten/emcc.bat") ||
            !has_file(o.emsdk / "upstream/bin/clang", o.emsdk / "upstream/bin/clang.exe"))
            throw std::runtime_error("Selected directory is not an Emscripten SDK: " + o.emsdk.u8string());
        if (!fs::is_regular_file(o.python)) throw std::runtime_error("Select a Python 3.10+ executable for the web SDK.");
        if (!python_supported(fs::absolute(o.python))) throw std::runtime_error("Python 3.10 or newer is required: " + o.python.u8string());
    }
}

void generate(const Options& o, const fs::path& distribution) {
    validate(o);
    const fs::path source = distribution / "template";
    const fs::path resources = distribution / "resources";
    if (!fs::is_directory(source) || !fs::is_directory(resources))
        throw std::runtime_error("Bundled template or resources are missing. Keep them alongside the executable.");
    const fs::path output = fs::absolute(o.parent / o.name);
    if (!fs::create_directory(output)) throw std::runtime_error("Cannot create output; it may already exist: " + output.u8string());
    for (const auto& entry : fs::recursive_directory_iterator(source)) {
        fs::path relative = entry.path().lexically_relative(source);
        if (!o.ios && relative.generic_u8string().find("app/src/main/cpp/ios-frameworks") == 0) continue;
        fs::path target = output / relative;
        if (entry.is_symlink()) throw std::runtime_error("Unexpected symlink in template: " + entry.path().u8string());
        if (entry.is_directory()) fs::create_directory(target);
        else if (entry.is_regular_file()) fs::copy_file(entry.path(), target);
        else throw std::runtime_error("Unsupported template entry: " + entry.path().u8string());
    }
#ifndef _WIN32
    fs::permissions(output / "gradlew", fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec, fs::perm_options::add);
    if (o.ios) {
        for (const auto& entry : fs::recursive_directory_iterator(output / "app/src/main/cpp/ios-frameworks")) {
            if (entry.is_regular_file() && entry.path().parent_path().filename() == entry.path().filename().string() + ".framework")
                fs::permissions(entry.path(), fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec, fs::perm_options::add);
        }
    }
#endif
    auto app = read(resources / "app.gradle.kts");
    replace_all(app, "@@ANDROID@@", o.android ? read(resources / "android.gradle.kts") : "");
    replace_all(app, "@@DESKTOP@@", o.desktop ? read(resources / "desktop.gradle.kts") : "");
    replace_all(app, "@@IOS@@", o.ios ? read(resources / "ios.gradle.kts") : "");
    replace_all(app, "@@WEB@@", o.web ? read(resources / "web.gradle.kts") : "");
    replace_all(app, "@@PACKAGE@@", o.application_id);
    replace_all(app, "@@PYTHON@@", o.python.empty() ? "" : kotlin_string(fs::absolute(o.python).u8string()));
    replace_all(app, "@@EMSDK@@", o.emsdk.empty() ? "" : kotlin_string(fs::absolute(o.emsdk).u8string()));
    write(output / "app/build.gradle.kts", app);
    if (o.android) {
        update(output / "settings.gradle.kts", "@@NAME@@", o.name);
        write(output / "local.properties", "# Android SDK location\nsdk.dir=" + property_string(fs::absolute(o.android_sdk).u8string()) + "\n");
    } else {
        write(output / "settings.gradle.kts", "rootProject.name = \"" + o.name + "\"\ninclude(\":app\")\n");
        write(output / "build.gradle.kts", "");
    }
    update(output / "app/src/main/res/values/strings.xml", "@@APP_NAME@@", o.name);
    const auto cmake = output / "app/src/main/cpp/CMakeLists.txt";
    update(cmake, "@@NAME@@", o.name);
    update(cmake, "@@PACKAGE@@", o.application_id);
    update(cmake, "elseif (CMAKE_SYSTEM_NAME STREQUAL \"iOS\")",
           "elseif (CMAKE_SYSTEM_NAME STREQUAL \"iOS\")\n    enable_language(OBJC)");
    update(output / "app/src/main/AndroidManifest.xml", "@@NAME@@", o.name);
    update(output / "app/src/main/cpp/framework/preferences.hpp", "@@NAME@@", o.name);
    update(output / "app/src/main/cpp/main.cpp", "@@NAME@@", o.name);
    for (const auto& file : {"app/src/main/cpp/main.cpp", "app/src/main/cpp/framework/platform.hpp"}) {
        auto path = output / file;
        write(path, "#if defined(__APPLE__)\n#include <TargetConditionals.h>\n#endif\n" + read(path));
    }
    update(output / "gradle.properties", "org.gradle.configuration-cache=true", "org.gradle.configuration-cache=false");
    write(output / ".gitignore", ".gradle/\n**/build/\nlocal.properties\n.cxx/\n.externalNativeBuild/\napp/src/main/cpp/emsdk/\n");
    write(output / "README.md", documentation(o));
}
}
