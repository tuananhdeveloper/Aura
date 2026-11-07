#pragma once

#include "raylib.h"
#include "platform.hpp"
#include <deque>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace Framework {
    // LibGDX-style asset manager: queue assets with load<T>(), pump update()
    // (or finishLoading()), then fetch with get<T>(). Assets are reference
    // counted, so each load() must be balanced by an unload().
    // GPU/audio loading must run on the main thread, so update() loads one
    // queued asset per call instead of using worker threads.
    class AssetManager {
        enum class Type { Texture, Sound, Music, Font };

        struct Asset {
            Type type;
            int refs = 1;
            Texture2D texture{};
            Sound sound{};
            Music music{};
            Font font{};
        };

        struct Task { std::string path; Type type; };

        std::unordered_map<std::string, Asset> assets_;
        std::deque<Task> queue_;
        int totalQueued_ = 0;
        int loadedCount_ = 0;

        template <typename T> static constexpr Type typeOf() {
            if constexpr (std::is_same_v<T, Texture2D>) return Type::Texture;
            else if constexpr (std::is_same_v<T, Sound>) return Type::Sound;
            else if constexpr (std::is_same_v<T, Music>) return Type::Music;
            else if constexpr (std::is_same_v<T, Font>) return Type::Font;
            else static_assert(!sizeof(T*), "Unsupported asset type");
        }

        static std::string resolve(const std::string& path) {
            if (Platform::isIOS()) {
                return std::string(GetApplicationDirectory()) + "assets/" + path;
            }
            return path;
        }

        static void unloadAsset(Asset& a) {
            switch (a.type) {
                case Type::Texture: UnloadTexture(a.texture); break;
                case Type::Sound: UnloadSound(a.sound); break;
                case Type::Music: UnloadMusicStream(a.music); break;
                case Type::Font: UnloadFont(a.font); break;
            }
        }

        void loadNow(const Task& t) {
            Asset a;
            a.type = t.type;
            const std::string full = resolve(t.path);
            switch (t.type) {
                case Type::Texture: a.texture = LoadTexture(full.c_str()); break;
                case Type::Sound: a.sound = LoadSound(full.c_str()); break;
                case Type::Music: a.music = LoadMusicStream(full.c_str()); break;
                case Type::Font: a.font = LoadFont(full.c_str()); break;
            }
            a.refs = assets_[t.path].refs;
            assets_[t.path] = a;
            ++loadedCount_;
        }

    public:
        AssetManager() = default;
        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;
        ~AssetManager() { clear(); }

        // Queue an asset. Already loaded/queued assets just gain a reference.
        template <typename T> void load(const std::string& path) {
            auto it = assets_.find(path);
            if (it != assets_.end()) { ++it->second.refs; return; }
            queue_.push_back({ path, typeOf<T>() });
            Asset placeholder;
            placeholder.type = typeOf<T>();
            placeholder.refs = 1;
            assets_[path] = placeholder;
            pending_.insert(path);
            ++totalQueued_;
        }

        // Loads one queued asset. Returns true when the queue is empty.
        bool update() {
            if (queue_.empty()) return true;
            Task t = queue_.front();
            queue_.pop_front();
            if (pending_.erase(t.path)) loadNow(t);
            else ++loadedCount_; // unloaded while still queued
            return queue_.empty();
        }

        void finishLoading() { while (!update()) {} }

        // 0..1 progress over the current batch of queued assets.
        float getProgress() const {
            return totalQueued_ == 0 ? 1.0f
                : static_cast<float>(loadedCount_) / static_cast<float>(totalQueued_);
        }

        int getQueuedAssets() const { return static_cast<int>(queue_.size()); }

        bool isLoaded(const std::string& path) const {
            return assets_.count(path) && !pending_.count(path);
        }

        template <typename T> T& get(const std::string& path) {
            auto it = assets_.find(path);
            if (it == assets_.end() || pending_.count(path)) {
                throw std::runtime_error("Asset not loaded: " + path);
            }
            Asset& a = it->second;
            if (a.type != typeOf<T>()) {
                throw std::runtime_error("Asset type mismatch: " + path);
            }
            if constexpr (std::is_same_v<T, Texture2D>) return a.texture;
            else if constexpr (std::is_same_v<T, Sound>) return a.sound;
            else if constexpr (std::is_same_v<T, Music>) return a.music;
            else return a.font;
        }

        // Drops one reference; the asset is freed when none remain.
        void unload(const std::string& path) {
            auto it = assets_.find(path);
            if (it == assets_.end()) return;
            if (--it->second.refs > 0) return;
            if (!pending_.count(path)) unloadAsset(it->second);
            else pending_.erase(path);
            assets_.erase(it);
        }

        void clear() {
            for (auto& [path, a] : assets_) {
                if (!pending_.count(path)) unloadAsset(a);
            }
            assets_.clear();
            pending_.clear();
            queue_.clear();
            totalQueued_ = loadedCount_ = 0;
        }

    private:
        std::unordered_set<std::string> pending_;
    };
}
