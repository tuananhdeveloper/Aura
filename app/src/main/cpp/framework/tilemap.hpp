#pragma once

#include "raylib.h"
#include <cstdint>
#include <string>
#include <vector>

namespace Framework {
    // Owns its tileset textures. Destroy or unload before closing the window.
    // load() replaces the existing map only after the new map loads completely.
    class TileMap {
    public:
        struct Layer {
            std::string name;
            int width = 0, height = 0;
            float offsetX = 0, offsetY = 0, opacity = 1;
            bool visible = true;
            std::vector<std::uint32_t> tiles;
        };

        TileMap() = default;
        ~TileMap();
        TileMap(const TileMap&) = delete;
        TileMap& operator=(const TileMap&) = delete;

        bool load(const std::string& path, std::string& error);
        void unload();
        void draw(Vector2 origin = {0, 0}, float scale = 1.0f) const;
        int width() const { return width_; }
        int height() const { return height_; }
        int tileWidth() const { return tileWidth_; }
        int tileHeight() const { return tileHeight_; }
        const std::vector<Layer>& layers() const { return layers_; }

    private:
        struct Tileset {
            std::uint32_t firstGid = 0;
            int tileWidth = 0, tileHeight = 0, columns = 0, count = 0;
            int margin = 0, spacing = 0, offsetX = 0, offsetY = 0;
            Texture2D texture{};
        };
        bool loadImpl(const std::string& path, std::string& error);
        int width_ = 0, height_ = 0, tileWidth_ = 0, tileHeight_ = 0;
        bool rightToLeft_ = false, bottomToTop_ = false;
        std::vector<Layer> layers_;
        std::vector<Tileset> tilesets_;
    };
}
