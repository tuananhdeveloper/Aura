#include "../framework/tilemap.hpp"
#include "rlgl.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
    std::unordered_map<std::string, std::string> files;
    std::vector<std::string> images;
    std::vector<unsigned> unloaded;
    struct Vertex { float u, v, x, y; };
    std::vector<Vertex> vertices;
    float u = 0, v = 0;
    unsigned char alpha = 0;
    unsigned nextTexture = 1;
    bool near(float a, float b) { return std::fabs(a - b) < 0.0001f; }
}

extern "C" {
    char* LoadFileText(const char* path) {
        auto it = files.find(path);
        if (it == files.end()) return nullptr;
        char* result = static_cast<char*>(std::malloc(it->second.size() + 1));
        std::memcpy(result, it->second.c_str(), it->second.size() + 1);
        return result;
    }
    void UnloadFileText(char* text) { std::free(text); }
    Texture2D LoadTexture(const char* path) {
        images.emplace_back(path);
        if (std::string(path) == "missing.png") return {};
        return { nextTexture++, 32, 16, 1, 0 };
    }
    bool IsTextureValid(Texture2D t) { return t.id != 0; }
    void UnloadTexture(Texture2D t) { unloaded.push_back(t.id); }
    void rlSetTexture(unsigned int) {}
    void rlBegin(int) {}
    void rlEnd() {}
    void rlColor4ub(unsigned char, unsigned char, unsigned char, unsigned char a) { alpha = a; }
    void rlTexCoord2f(float x, float y) { u = x; v = y; }
    void rlVertex2f(float x, float y) { vertices.push_back({u, v, x, y}); }
}

int main() {
    files["maps/level.tmx"] =
        "<map orientation='orthogonal' width='8' height='1' tilewidth='16' tileheight='16'>"
        "<tileset firstgid='1' source='../sets/tiles.tsx'/>"
        "<layer name='ground' width='8' height='1' opacity='0.5' offsetx='2' offsety='3'>"
        "<data encoding='csv'>1,2147483649,1073741825,3221225473,"
        "536870913,2684354561,1610612737,3758096385</data></layer>"
        "<layer name='hidden' visible='0' width='1' height='1'><data encoding='csv'>2</data></layer>"
        "</map>";
    files["sets/tiles.tsx"] =
        "<tileset tilewidth='16' tileheight='16' columns='2' tilecount='2'>"
        "<image source='../images/tiles.png' width='32' height='16'/></tileset>";

    Framework::TileMap map;
    std::string error;
    assert(map.load("maps/level.tmx", error));
    assert(error.empty() && map.width() == 8 && map.layers().size() == 2);
    assert(images.size() == 1 && images[0] == "images/tiles.png");
    map.draw({10, 20}, 2);
    assert(vertices.size() == 32 && alpha == 128);
    assert(near(vertices[0].x, 14) && near(vertices[0].y, 26));
    assert(near(vertices[2].x, 46) && near(vertices[2].y, 58));
    const float corners[4][2] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
    for (int flags = 0; flags < 8; ++flags) {
        for (int i = 0; i < 4; ++i) {
            float x = corners[i][0], y = corners[i][1];
            if (flags & 1) x = 1 - x;
            if (flags & 2) y = 1 - y;
            if (flags & 4) std::swap(x, y);
            assert(near(vertices[flags * 4 + i].u, x / 2));
            assert(near(vertices[flags * 4 + i].v, y));
        }
    }
    files["bad.tmx"] = "<map orientation='isometric'/>";
    assert(!map.load("bad.tmx", error) && !error.empty());
    assert(map.width() == 8 && unloaded.empty());
    files["bad.tmx"] =
        "<map orientation='orthogonal' width='1' height='1' tilewidth='16' tileheight='16'>"
        "<tileset firstgid='1' tilewidth='16' tileheight='16' columns='2' tilecount='2'>"
        "<image source='ok.png'/></tileset>"
        "<layer width='1' height='1'><data encoding='csv'>3</data></layer></map>";
    assert(!map.load("bad.tmx", error) && error.find("GID") != std::string::npos);
    assert(unloaded.size() == 1 && map.width() == 8);
    files["bad.tmx"].replace(files["bad.tmx"].find(">3</data>"), 9, ">1,2</data>");
    assert(!map.load("bad.tmx", error) && !error.empty());
    assert(unloaded.size() == 2);
    files["missing-image.tmx"] =
        "<map orientation='orthogonal' width='1' height='1' tilewidth='16' tileheight='16'>"
        "<tileset firstgid='1' tilewidth='16' tileheight='16' columns='1' tilecount='1'>"
        "<image source='missing.png'/></tileset></map>";
    assert(!map.load("missing-image.tmx", error) && error.find("image") != std::string::npos);
    files["base64.tmx"] =
        "<map orientation='orthogonal' width='1' height='1' tilewidth='16' tileheight='16'>"
        "<layer width='1' height='1'><data encoding='base64'>AAAAAA==</data></layer></map>";
    assert(!map.load("base64.tmx", error) && error.find("CSV") != std::string::npos);
    assert(!map.load("missing.tmx", error) && !error.empty());
    files["replacement.tmx"] =
        "<map orientation='orthogonal' width='2' height='1' tilewidth='16' tileheight='16' renderorder='left-down'>"
        "<tileset firstgid='1' tilewidth='8' tileheight='8' columns='2' tilecount='2' margin='1' spacing='2'>"
        "<tileoffset x='3' y='-2'/><image source='ok.png'/></tileset>"
        "<tileset firstgid='10' tilewidth='16' tileheight='16' columns='2' tilecount='2'>"
        "<image source='other.png'/></tileset>"
        "<layer width='2' height='1'><data encoding='csv'>2,10</data></layer></map>";
    assert(map.load("replacement.tmx", error));
    assert(unloaded.size() == 3); // Previous map released after successful replacement.
    vertices.clear();
    map.draw();
    assert(vertices.size() == 8);
    assert(near(vertices[0].x, 16) && near(vertices[0].y, 0)); // Right tile first.
    assert(near(vertices[4].x, 3) && near(vertices[4].y, 6)); // Bottom-aligned with tile offset.
    assert(near(vertices[4].u, 11.0f / 32.0f) && near(vertices[4].v, 1.0f / 16.0f));
    map.unload();
    assert(map.width() == 0 && unloaded.size() == 5);
    std::cout << "Tilemap tests passed\n";
}
