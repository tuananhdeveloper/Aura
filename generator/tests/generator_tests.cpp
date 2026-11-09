#include "core.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

std::string read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Missing file: " + path.u8string());
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

void touch(const fs::path& path) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Cannot create fixture");
}

template <typename F>
void expect_error(F action, const std::string& expected) {
    try { action(); }
    catch (const std::exception& e) {
        check(std::string(e.what()).find(expected) != std::string::npos, "Unexpected error");
        return;
    }
    throw std::runtime_error("Expected error: " + expected);
}

int main(int argc, char** argv) {
    if (argc != 4) return 1;
    const auto temp = fs::temp_directory_path() / ("aura-generator-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        check(fs::create_directory(temp), "Cannot create test directory");
        aura::Options o;
        o.name = "DesktopGame"; o.application_id = "com.example.desktop";
        o.parent = temp; o.desktop = true;
        aura::generate(o, argv[1]);
        const auto output = temp / o.name;
        auto app = read(output / "app/build.gradle.kts");
        check(app.find("\"desktopBuild\"") != std::string::npos, "desktop task missing");
        check(app.find("\"htmlBuild\"") == std::string::npos, "unexpected web task");
        check(!fs::exists(output / "app/src/main/cpp/ios-frameworks"), "unexpected iOS frameworks");
        check(!fs::exists(output / "app/src/main/cpp/emsdk"), "unexpected emsdk");
        check(fs::is_regular_file(output / "app/src/main/cpp/raylib/src/raylib.h"), "raylib missing");
        check(read(output / "LICENSE").find("Version 2.0, January 2004") != std::string::npos, "license missing");
        check(read(output / "app/src/main/cpp/CMakeLists.txt").find("project(DesktopGame LANGUAGES C CXX)") != std::string::npos, "project name missing");
        check(read(output / "README.md").find(":app:windowsBuild") != std::string::npos, "Windows documentation missing");
        expect_error([&] { aura::generate(o, argv[1]); }, "already exists");
        o.name = "../bad";
        expect_error([&] { aura::validate(o); }, "Project name");
        o.name = "raylib";
        expect_error([&] { aura::validate(o); }, "conflicts");
        o.name = "BadId"; o.application_id = "invalid";
        expect_error([&] { aura::validate(o); }, "Application ID");
        o.application_id = "com.example.game"; o.desktop = false;
        expect_error([&] { aura::validate(o); }, "at least one platform");
        o.desktop = true;
        o.name = "AllPlatforms"; o.application_id = "com.example.all"; o.android = o.ios = o.web = true;
        o.android_sdk = temp / fs::u8path("android sdk \xc3\xa9");
        fs::create_directories(o.android_sdk);
        expect_error([&] { aura::validate(o); }, "not an Android SDK");
        touch(o.android_sdk / "platform-tools/adb");
        fs::create_directory(o.android_sdk / "platforms");
        o.emsdk = temp / "emsdk";
        fs::create_directory(o.emsdk);
        expect_error([&] { aura::validate(o); }, "not an Emscripten SDK");
        touch(o.emsdk / "emsdk_env.sh");
        touch(o.emsdk / "upstream/emscripten/emcc");
        touch(o.emsdk / "upstream/bin/clang");
        o.python = fs::u8path(argv[3]);
        expect_error([&] { aura::validate(o); }, "Python 3.10 or newer");
        o.python = fs::u8path(argv[2]);
        aura::generate(o, argv[1]);
        auto all = temp / o.name;
        app = read(all / "app/build.gradle.kts");
        for (auto task : {"desktopBuild", "iosBuild", "iosRun", "htmlBuild"})
            check(app.find(task) != std::string::npos, "selected task missing");
        check(app.find("com.example.all") != std::string::npos, "application id missing");
        check(read(all / "local.properties").find("\\u00e9") != std::string::npos, "SDK path is not Java-properties encoded");
        check(fs::is_directory(all / "app/src/main/cpp/ios-frameworks"), "iOS frameworks missing");
        check(read(all / "app/src/main/cpp/CMakeLists.txt").find("enable_language(OBJC)") != std::string::npos, "Objective-C missing");
        check(read(all / "app/src/main/res/values/styles.xml").find("shortEdges") != std::string::npos, "cutout theme missing");
        check(read(all / "README.md").find("### Web") != std::string::npos, "web documentation missing");
        fs::remove_all(temp);
        std::cout << "Generator tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\nTest files retained at " << temp << '\n';
        return 1;
    }
}
