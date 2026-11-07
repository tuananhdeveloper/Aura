#include "tilemap.hpp"
#include "../third_party/tinyxml2/tinyxml2.h"
#include "rlgl.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

namespace Framework {
    namespace {
        using tinyxml2::XMLElement;
        constexpr std::uint32_t flipH = 0x80000000u;
        constexpr std::uint32_t flipV = 0x40000000u;
        constexpr std::uint32_t flipD = 0x20000000u;
        constexpr std::uint32_t gidMask = 0x0fffffffu;
        constexpr std::size_t maxCells = 10000000;

        bool fail(std::string& error, const std::string& message) {
            error = message;
            return false;
        }

        bool readXml(const std::string& path, tinyxml2::XMLDocument& doc, std::string& error) {
            char* text = LoadFileText(path.c_str());
            if (!text) return fail(error, "Cannot read XML file: " + path);
            const auto result = doc.Parse(text);
            UnloadFileText(text);
            if (result != tinyxml2::XML_SUCCESS)
                return fail(error, "Invalid XML in " + path + ": " + doc.ErrorStr());
            return true;
        }

        std::string relativePath(const std::string& base, const std::string& path) {
            std::string joined = path;
            if (!path.empty() && path[0] != '/' && path[0] != '\\' &&
                !(path.size() > 1 && path[1] == ':')) {
                const auto slash = base.find_last_of("/\\");
                if (slash != std::string::npos) joined = base.substr(0, slash + 1) + path;
            }
            std::replace(joined.begin(), joined.end(), '\\', '/');
            std::string prefix;
            std::size_t start = 0;
            if (joined.size() > 1 && joined[1] == ':') {
                prefix = joined.substr(0, 2);
                start = 2;
            }
            if (start < joined.size() && joined[start] == '/') {
                prefix += '/';
                ++start;
            }
            std::vector<std::string> parts;
            while (start < joined.size()) {
                const auto end = joined.find('/', start);
                const auto part = joined.substr(start, end == std::string::npos ? end : end - start);
                if (part == ".." && !parts.empty() && parts.back() != "..") parts.pop_back();
                else if (part == ".." && (prefix.empty() || prefix.back() != '/')) parts.push_back(part);
                else if (!part.empty() && part != "." && part != "..") parts.push_back(part);
                if (end == std::string::npos) break;
                start = end + 1;
            }
            for (const auto& part : parts) {
                if (!prefix.empty() && prefix.back() != '/') prefix += '/';
                prefix += part;
            }
            return prefix;
        }

        bool positive(const XMLElement* e, const char* name, int& value, std::string& error) {
            if (e->QueryIntAttribute(name, &value) != tinyxml2::XML_SUCCESS || value <= 0)
                return fail(error, std::string("Expected positive ") + name + " on <" + e->Name() + ">");
            return true;
        }

        bool optionalInt(const XMLElement* e, const char* name, int& value, std::string& error) {
            if (!e->Attribute(name)) return true;
            if (e->QueryIntAttribute(name, &value) != tinyxml2::XML_SUCCESS)
                return fail(error, std::string("Invalid integer ") + name + " on <" + e->Name() + ">");
            return true;
        }

        bool optionalFloat(const XMLElement* e, const char* name, float& value, std::string& error) {
            if (!e->Attribute(name)) return true;
            if (e->QueryFloatAttribute(name, &value) != tinyxml2::XML_SUCCESS || !std::isfinite(value))
                return fail(error, std::string("Invalid number ") + name + " on <" + e->Name() + ">");
            return true;
        }

        bool parseCsv(const char* text, std::size_t count, std::vector<std::uint32_t>& out,
                      std::string& error) {
            if (!text) return fail(error, "Missing CSV tile data");
            const char* p = text;
            const char* end = p + std::strlen(p);
            auto spaces = [&]() { while (p < end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')) ++p; };
            while (true) {
                spaces();
                if (p == end) break;
                std::uint32_t value = 0;
                const auto parsed = std::from_chars(p, end, value);
                if (parsed.ec != std::errc{} || parsed.ptr == p)
                    return fail(error, "Invalid CSV tile GID");
                out.push_back(value);
                if (out.size() > count) return fail(error, "Too many CSV tile GIDs");
                p = parsed.ptr;
                spaces();
                if (p == end) break;
                if (*p++ != ',') return fail(error, "Expected comma between CSV tile GIDs");
                spaces();
                if (p == end) return fail(error, "Trailing comma in CSV tile data");
            }
            return out.size() == count || fail(error, "Incorrect number of CSV tile GIDs");
        }
    }

    TileMap::~TileMap() { unload(); }

    void TileMap::unload() {
        for (const auto& tileset : tilesets_) if (tileset.texture.id) UnloadTexture(tileset.texture);
        tilesets_.clear();
        layers_.clear();
        width_ = height_ = tileWidth_ = tileHeight_ = 0;
        rightToLeft_ = bottomToTop_ = false;
    }

    bool TileMap::load(const std::string& path, std::string& error) {
        error.clear();
        TileMap next;
        if (!next.loadImpl(path, error)) return false;
        std::swap(width_, next.width_);
        std::swap(height_, next.height_);
        std::swap(tileWidth_, next.tileWidth_);
        std::swap(tileHeight_, next.tileHeight_);
        std::swap(rightToLeft_, next.rightToLeft_);
        std::swap(bottomToTop_, next.bottomToTop_);
        layers_.swap(next.layers_);
        tilesets_.swap(next.tilesets_);
        return true;
    }

    bool TileMap::loadImpl(const std::string& path, std::string& error) {
        tinyxml2::XMLDocument doc;
        if (!readXml(path, doc, error)) return false;
        const auto* map = doc.FirstChildElement("map");
        if (!map || !map->Attribute("orientation", "orthogonal"))
            return fail(error, "Only orthogonal TMX maps are supported");
        int infinite = 0;
        if (!optionalInt(map, "infinite", infinite, error)) return false;
        if (infinite) return fail(error, "Infinite/chunked maps are not supported");
        if (!positive(map, "width", width_, error) || !positive(map, "height", height_, error) ||
            !positive(map, "tilewidth", tileWidth_, error) || !positive(map, "tileheight", tileHeight_, error))
            return false;
        const char* order = map->Attribute("renderorder");
        if (order && std::strcmp(order, "right-down") != 0 && std::strcmp(order, "right-up") != 0 &&
            std::strcmp(order, "left-down") != 0 && std::strcmp(order, "left-up") != 0)
            return fail(error, "Unsupported render order");
        if (order) {
            rightToLeft_ = std::strncmp(order, "left", 4) == 0;
            bottomToTop_ = std::strstr(order, "-up") != nullptr;
        }

        for (auto* element = map->FirstChildElement(); element; element = element->NextSiblingElement()) {
            if (std::strcmp(element->Name(), "tileset") == 0) {
                unsigned first = 0;
                if (element->QueryUnsignedAttribute("firstgid", &first) != tinyxml2::XML_SUCCESS || first == 0 || first > gidMask)
                    return fail(error, "Invalid tileset firstgid");
                tinyxml2::XMLDocument tsx;
                const XMLElement* source = element;
                std::string sourcePath = path;
                if (const char* external = element->Attribute("source")) {
                    sourcePath = relativePath(path, external);
                    if (!readXml(sourcePath, tsx, error)) return false;
                    source = tsx.FirstChildElement("tileset");
                    if (!source) return fail(error, "Missing tileset in " + sourcePath);
                }
                Tileset ts;
                ts.firstGid = first;
                if (!positive(source, "tilewidth", ts.tileWidth, error) ||
                    !positive(source, "tileheight", ts.tileHeight, error) ||
                    !positive(source, "tilecount", ts.count, error) ||
                    !positive(source, "columns", ts.columns, error) ||
                    !optionalInt(source, "margin", ts.margin, error) ||
                    !optionalInt(source, "spacing", ts.spacing, error)) return false;
                if (ts.margin < 0 || ts.spacing < 0 || static_cast<std::uint64_t>(ts.firstGid) + ts.count - 1 > gidMask)
                    return fail(error, "Invalid tileset dimensions or GID range");
                if (const auto* offset = source->FirstChildElement("tileoffset")) {
                    if (!optionalInt(offset, "x", ts.offsetX, error) ||
                        !optionalInt(offset, "y", ts.offsetY, error)) return false;
                }
                for (auto* tile = source->FirstChildElement("tile"); tile; tile = tile->NextSiblingElement("tile"))
                    if (tile->FirstChildElement("animation") || tile->FirstChildElement("image"))
                        return fail(error, "Animated tiles and image collection tilesets are not supported");
                const auto* image = source->FirstChildElement("image");
                if (!image || !image->Attribute("source") || image->Attribute("trans"))
                    return fail(error, "Tileset must use one image without a transparent-color key");
                const std::string imagePath = relativePath(sourcePath, image->Attribute("source"));
                ts.texture = LoadTexture(imagePath.c_str());
                if (!IsTextureValid(ts.texture)) {
                    if (ts.texture.id) UnloadTexture(ts.texture);
                    return fail(error, "Cannot load tileset image: " + imagePath);
                }
                tilesets_.push_back(ts);
                const auto rows = (static_cast<std::int64_t>(ts.count) + ts.columns - 1) / ts.columns;
                const auto neededWidth = 2LL * ts.margin + static_cast<std::int64_t>(ts.columns) * ts.tileWidth +
                                         static_cast<std::int64_t>(ts.columns - 1) * ts.spacing;
                const auto neededHeight = 2LL * ts.margin + rows * ts.tileHeight + (rows - 1) * ts.spacing;
                if (neededWidth > ts.texture.width || neededHeight > ts.texture.height)
                    return fail(error, "Tileset image is smaller than its declared grid: " + imagePath);
            } else if (std::strcmp(element->Name(), "layer") == 0) {
                Layer layer;
                if (element->Attribute("tintcolor") || element->Attribute("parallaxx") ||
                    element->Attribute("parallaxy") || element->Attribute("blendmode"))
                    return fail(error, "Layer tint, parallax, and blend modes are not supported");
                if (const char* name = element->Attribute("name")) layer.name = name;
                if (!positive(element, "width", layer.width, error) || !positive(element, "height", layer.height, error) ||
                    !optionalFloat(element, "offsetx", layer.offsetX, error) ||
                    !optionalFloat(element, "offsety", layer.offsetY, error) ||
                    !optionalFloat(element, "opacity", layer.opacity, error)) return false;
                int visible = 1, x = 0, y = 0;
                if (!optionalInt(element, "visible", visible, error) ||
                    !optionalInt(element, "x", x, error) || !optionalInt(element, "y", y, error)) return false;
                if (layer.opacity < 0 || layer.opacity > 1 || (visible != 0 && visible != 1))
                    return fail(error, "Invalid layer opacity or visibility");
                layer.visible = visible != 0;
                layer.offsetX += static_cast<float>(x) * tileWidth_;
                layer.offsetY += static_cast<float>(y) * tileHeight_;
                const std::uint64_t cells = static_cast<std::uint64_t>(layer.width) * layer.height;
                if (cells > maxCells) return fail(error, "Tile layer exceeds supported size");
                const auto* data = element->FirstChildElement("data");
                if (!data || !data->Attribute("encoding", "csv") || data->Attribute("compression") ||
                    data->FirstChildElement())
                    return fail(error, "Only uncompressed CSV tile layers are supported");
                if (!parseCsv(data->GetText(), static_cast<std::size_t>(cells), layer.tiles, error)) return false;
                layers_.push_back(std::move(layer));
            } else if (std::strcmp(element->Name(), "properties") != 0) {
                return fail(error, std::string("Unsupported map element: ") + element->Name());
            }
        }
        std::sort(tilesets_.begin(), tilesets_.end(), [](const Tileset& a, const Tileset& b) { return a.firstGid < b.firstGid; });
        for (std::size_t i = 1; i < tilesets_.size(); ++i)
            if (static_cast<std::uint64_t>(tilesets_[i - 1].firstGid) + tilesets_[i - 1].count > tilesets_[i].firstGid)
                return fail(error, "Overlapping tileset GID ranges");
        for (const auto& layer : layers_) for (const auto raw : layer.tiles) {
            const auto gid = raw & gidMask;
            if (!gid) {
                continue;
            }
            bool found = false;
            for (const auto& ts : tilesets_)
                if (gid >= ts.firstGid && static_cast<std::uint64_t>(gid) < static_cast<std::uint64_t>(ts.firstGid) + ts.count)
                    found = true;
            if (!found) return fail(error, "Tile GID does not belong to a tileset: " + std::to_string(gid));
        }
        return true;
    }

    void TileMap::draw(Vector2 origin, float scale) const {
        if (!std::isfinite(scale) || scale <= 0) return;
        for (const auto& layer : layers_) {
            if (!layer.visible || layer.opacity == 0) continue;
            for (int iy = 0; iy < layer.height; ++iy) {
                const int y = bottomToTop_ ? layer.height - 1 - iy : iy;
                for (int ix = 0; ix < layer.width; ++ix) {
                    const int x = rightToLeft_ ? layer.width - 1 - ix : ix;
                    const auto raw = layer.tiles[static_cast<std::size_t>(y) * layer.width + x];
                    const auto gid = raw & gidMask;
                    if (!gid) continue;
                    auto it = std::upper_bound(tilesets_.begin(), tilesets_.end(), gid,
                        [](std::uint32_t id, const Tileset& ts) { return id < ts.firstGid; });
                    if (it == tilesets_.begin()) continue;
                    const auto& ts = *--it;
                    const auto local = gid - ts.firstGid;
                    const float sx = static_cast<float>(static_cast<std::int64_t>(ts.margin) + (local % ts.columns) * (static_cast<std::int64_t>(ts.tileWidth) + ts.spacing));
                    const float sy = static_cast<float>(static_cast<std::int64_t>(ts.margin) + (local / ts.columns) * (static_cast<std::int64_t>(ts.tileHeight) + ts.spacing));
                    const float left = origin.x + (layer.offsetX + static_cast<float>(x) * tileWidth_ + ts.offsetX) * scale;
                    const float top = origin.y + (layer.offsetY + static_cast<float>(y) * tileHeight_ + tileHeight_ - ts.tileHeight + ts.offsetY) * scale;
                    const float w = ts.tileWidth * scale, h = ts.tileHeight * scale;
                    rlSetTexture(ts.texture.id);
                    rlBegin(RL_QUADS);
                    rlColor4ub(255, 255, 255, static_cast<unsigned char>(std::round(layer.opacity * 255)));
                    const float corners[4][2] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
                    for (const auto& corner : corners) {
                        float u = corner[0], v = corner[1];
                        if (raw & flipH) u = 1 - u;
                        if (raw & flipV) v = 1 - v;
                        if (raw & flipD) std::swap(u, v);
                        rlTexCoord2f((sx + u * ts.tileWidth) / ts.texture.width,
                                     (sy + v * ts.tileHeight) / ts.texture.height);
                        rlVertex2f(left + corner[0] * w, top + corner[1] * h);
                    }
                    rlEnd();
                    rlSetTexture(0);
                }
            }
        }
    }
}
