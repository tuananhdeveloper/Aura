#pragma once

#include "raylib.h"
#include "platform.hpp"
#include <cstdlib>
#include <map>
#include <string>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

namespace Framework {
    // LibGDX-style key/value store. put*() changes stay in memory until
    // flush() is called (e.g. on pause()/stop() or after a high score).
    //
    // Storage: Android internal storage, iOS Documents, desktop per-user app
    // data dir, and localStorage on web.
    class Preferences {
        std::string name_;
        std::map<std::string, std::string> values_;
        bool dirty_ = false;

        static std::string escape(const std::string& s) {
            std::string out;
            for (char c : s) {
                if (c == '\\') out += "\\\\";
                else if (c == '\n') out += "\\n";
                else if (c == '\r') out += "\\r";
                else if (c == '=') out += "\\e";
                else out += c;
            }
            return out;
        }

        static std::string unescape(const std::string& s) {
            std::string out;
            for (size_t i = 0; i < s.size(); ++i) {
                if (s[i] == '\\' && i + 1 < s.size()) {
                    const char n = s[++i];
                    out += n == 'n' ? '\n' : n == 'r' ? '\r' : n == 'e' ? '=' : n;
                } else {
                    out += s[i];
                }
            }
            return out;
        }

        std::string fileName() const { return name_ + ".prefs"; }

        static std::string baseDir() {
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
            return "";  // Android's fopen wrapper maps relative names to internal storage
#else
            auto env = [](const char* k) { const char* v = getenv(k); return v ? std::string(v) : std::string(); };
            std::string dir;
    #if defined(_WIN32)
            dir = env("APPDATA");
    #elif defined(__APPLE__)
            const std::string home = env("HOME");
            if (!home.empty()) dir = Platform::isIOS() ? home + "/Documents" : home + "/Library/Application Support";
    #else
            const std::string xdg = env("XDG_DATA_HOME");
            const std::string home = env("HOME");
            dir = !xdg.empty() ? xdg : (!home.empty() ? home + "/.local/share" : "");
    #endif
            if (dir.empty()) return "";
            dir += "/MyRaylibGame/";
            MakeDirectory(dir.c_str());
            return dir;
#endif
        }

        std::string readRaw() const {
#if defined(__EMSCRIPTEN__)
            char* data = (char*)EM_ASM_PTR({
                var v = window.localStorage.getItem(UTF8ToString($0));
                if (v === null) return 0;
                var n = lengthBytesUTF8(v) + 1;
                var p = _malloc(n);
                stringToUTF8(v, p, n);
                return p;
            }, fileName().c_str());
            if (!data) return "";
            std::string out(data);
            free(data);
            return out;
#else
            const std::string path = baseDir() + fileName();
            char* text = LoadFileText(path.c_str());
            if (!text) return "";
            std::string out(text);
            UnloadFileText(text);
            return out;
#endif
        }

        bool writeRaw(const std::string& data) const {
#if defined(__EMSCRIPTEN__)
            EM_ASM({ window.localStorage.setItem(UTF8ToString($0), UTF8ToString($1)); },
                   fileName().c_str(), data.c_str());
            return true;
#else
            const std::string path = baseDir() + fileName();
            return SaveFileText(path.c_str(), data.c_str());
#endif
        }

        void parse(const std::string& text) {
            size_t pos = 0;
            while (pos < text.size()) {
                size_t end = text.find('\n', pos);
                if (end == std::string::npos) end = text.size();
                const std::string line = text.substr(pos, end - pos);
                pos = end + 1;
                const size_t eq = line.find('=');
                if (eq == std::string::npos || eq == 0) continue;
                values_[unescape(line.substr(0, eq))] = unescape(line.substr(eq + 1));
            }
        }

        template <typename T> void put(const std::string& key, const std::string& value) {
            auto it = values_.find(key);
            if (it != values_.end() && it->second == value) return;
            values_[key] = value;
            dirty_ = true;
        }

    public:
        explicit Preferences(const std::string& name) : name_(name) { reload(); }

        // Discards unsaved changes and reads from storage.
        void reload() {
            values_.clear();
            dirty_ = false;
            parse(readRaw());
        }

        bool contains(const std::string& key) const { return values_.count(key) > 0; }

        void putBoolean(const std::string& key, bool v) { put<bool>(key, v ? "true" : "false"); }
        void putInteger(const std::string& key, int v) { put<int>(key, std::to_string(v)); }
        void putLong(const std::string& key, long long v) { put<long long>(key, std::to_string(v)); }
        void putFloat(const std::string& key, float v) { put<float>(key, std::to_string(v)); }
        void putString(const std::string& key, const std::string& v) { put<std::string>(key, v); }

        bool getBoolean(const std::string& key, bool def = false) const {
            auto it = values_.find(key);
            return it == values_.end() ? def : it->second == "true";
        }
        int getInteger(const std::string& key, int def = 0) const {
            auto it = values_.find(key);
            if (it == values_.end()) return def;
            char* end = nullptr;
            const long v = strtol(it->second.c_str(), &end, 10);
            return end == it->second.c_str() ? def : static_cast<int>(v);
        }
        long long getLong(const std::string& key, long long def = 0) const {
            auto it = values_.find(key);
            if (it == values_.end()) return def;
            char* end = nullptr;
            const long long v = strtoll(it->second.c_str(), &end, 10);
            return end == it->second.c_str() ? def : v;
        }
        float getFloat(const std::string& key, float def = 0.0f) const {
            auto it = values_.find(key);
            if (it == values_.end()) return def;
            char* end = nullptr;
            const float v = strtof(it->second.c_str(), &end);
            return end == it->second.c_str() ? def : v;
        }
        std::string getString(const std::string& key, const std::string& def = "") const {
            auto it = values_.find(key);
            return it == values_.end() ? def : it->second;
        }

        void remove(const std::string& key) {
            if (values_.erase(key)) dirty_ = true;
        }
        void clear() {
            if (!values_.empty()) dirty_ = true;
            values_.clear();
        }

        // Persists changes. Returns false if writing failed.
        bool flush() {
            if (!dirty_) return true;
            std::string out;
            for (const auto& [k, v] : values_) out += escape(k) + "=" + escape(v) + "\n";
            if (!writeRaw(out)) return false;
            dirty_ = false;
            return true;
        }
    };
}
