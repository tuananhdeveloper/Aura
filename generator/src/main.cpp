#include "core.hpp"
#include "dialogs.hpp"
#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <array>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <future>
#include <stdexcept>
#include <string>

namespace {
constexpr int path_size = 4096;

template <std::size_t N>
void set_text(std::array<char, N>& field, const std::string& value) {
    if (value.size() >= N) throw std::runtime_error("The selected path is too long.");
    std::memcpy(field.data(), value.c_str(), value.size() + 1);
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

template <std::size_t N>
void text_row(int y, const char* label, std::array<char, N>& text, int id, int& editing, bool browse, bool file, std::string& error) {
    GuiLabel({20, static_cast<float>(y), 220, 30}, label);
    if (GuiTextBox({245, static_cast<float>(y), browse ? 520.0f : 630.0f, 30},
                   text.data(), static_cast<int>(N), editing == id))
        editing = editing == id ? -1 : id;
    if (browse && GuiButton({775, static_cast<float>(y), 100, 30}, "Browse...")) {
        editing = -1;
        try {
            auto result = aura::choose_path(file ? "Select Python executable" : label, text.data(), file);
            if (!result.empty()) set_text(text, result);
        } catch (const std::exception& e) { error = e.what(); }
    }
}

aura::fs::path application_directory() {
#ifdef _WIN32
    return aura::windows_application_directory();
#else
    return aura::fs::u8path(GetApplicationDirectory());
#endif
}
}

int main() {
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(900, 520, "Aura Project Generator");
    SetTargetFPS(60);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 18);

    std::array<char, path_size> name = {}, application_id = {};
    std::array<char, path_size> parent = {}, android_sdk = {}, emsdk = {}, python = {};
    std::string error;
    set_text(name, "MyGame");
    set_text(application_id, "com.example.mygame");
#ifdef _WIN32
    try { set_text(parent, aura::windows_home_directory()); }
    catch (const std::exception& e) { error = e.what(); }
#else
    const char* home = std::getenv("HOME");
    if (home) {
        try { set_text(parent, home); } catch (const std::exception& e) { error = e.what(); }
    }
#endif
    aura::fs::path distribution;
    try { distribution = application_directory(); }
    catch (const std::exception& e) { error = e.what(); }
    bool android = false, ios = false, desktop = true, web = false;
    int editing = -1;
    std::future<void> job;
    std::string status = "Choose platforms and a destination, then select Generate.";
    while (!WindowShouldClose()) {
        if (job.valid() && job.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            try {
                job.get();
                status = "Project generated successfully.";
            } catch (const std::exception& e) {
                status = "Generation failed. An incomplete output may have been left in place.";
                error = e.what();
            }
        }
        BeginDrawing();
        ClearBackground(RAYWHITE);
        if (!error.empty()) GuiLock();
        GuiLabel({20, 15, 850, 35}, "Create an Aura starter project");
        text_row(60, "Project name", name, 0, editing, false, false, error);
        text_row(105, "Application ID", application_id, 1, editing, false, false, error);
        text_row(150, "Parent directory", parent, 2, editing, true, false, error);
        GuiLabel({20, 200, 220, 30}, "Platforms");
        GuiCheckBox({245, 204, 22, 22}, "Desktop", &desktop);
        GuiCheckBox({380, 204, 22, 22}, "Android", &android);
        GuiCheckBox({515, 204, 22, 22}, "iOS", &ios);
        GuiCheckBox({620, 204, 22, 22}, "Web", &web);
        text_row(250, "Android SDK (Android)", android_sdk, 3, editing, true, false, error);
        text_row(295, "Emscripten SDK (Web)", emsdk, 4, editing, true, false, error);
        text_row(340, "Python executable (Web)", python, 5, editing, true, true, error);
        GuiLabel({20, 445, 850, 30}, status.c_str());
        if (!job.valid() && GuiButton({705, 395, 170, 35}, "Generate")) {
            try {
                aura::Options o;
                o.name = trim(name.data());
                o.application_id = trim(application_id.data());
                o.parent = aura::fs::u8path(trim(parent.data()));
                o.android = android; o.ios = ios; o.desktop = desktop; o.web = web;
                o.android_sdk = aura::fs::u8path(trim(android_sdk.data()));
                o.emsdk = aura::fs::u8path(trim(emsdk.data()));
                o.python = aura::fs::u8path(trim(python.data()));
                status = "Generating project...";
                job = std::async(std::launch::async, [o, distribution] { aura::generate(o, distribution); });
            } catch (const std::exception& e) { error = e.what(); }
        }
        GuiUnlock();
        if (!error.empty()) {
            GuiSetStyle(DEFAULT, TEXT_WRAP_MODE, TEXT_WRAP_WORD);
            bool closed = GuiWindowBox({50, 150, 800, 200}, "Generation error");
            GuiLabel({70, 190, 760, 110}, error.c_str());
            closed = GuiButton({350, 310, 200, 30}, "OK") || closed;
            GuiSetStyle(DEFAULT, TEXT_WRAP_MODE, TEXT_WRAP_NONE);
            if (closed) error.clear();
        }
        EndDrawing();
    }
    if (job.valid()) {
        try { job.get(); }
        catch (const std::exception& e) { TraceLog(LOG_ERROR, "Generation failed: %s", e.what()); }
    }
    CloseWindow();
}
