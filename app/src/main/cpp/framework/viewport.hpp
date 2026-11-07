#pragma once

#include "raylib.h"
#include <algorithm>
#include <cmath>

namespace Framework {

// A small raylib Camera2D wrapper with a centered world origin, similar to
// LibGDX's OrthographicCamera. World coordinates still start at (0, 0) by
// default because the camera target and screen offset are both centered.
class OrthographicCamera {
private:
    Camera2D camera_{};
    float worldWidth_;
    float worldHeight_;

public:
    OrthographicCamera(float worldWidth, float worldHeight)
        : worldWidth_(worldWidth), worldHeight_(worldHeight) {
        camera_.target = { worldWidth_ * 0.5f, worldHeight_ * 0.5f };
        camera_.offset = { worldWidth_ * 0.5f, worldHeight_ * 0.5f };
        camera_.rotation = 0.0f;
        camera_.zoom = 1.0f;
    }

    void setPosition(Vector2 position) { camera_.target = position; }
    Vector2 getPosition() const { return camera_.target; }

    void setZoom(float zoom) {
        camera_.zoom = zoom > 0.0f ? zoom : 0.0001f;
    }
    float getZoom() const { return camera_.zoom; }

    void setRotation(float degrees) { camera_.rotation = degrees; }
    float getRotation() const { return camera_.rotation; }

    float getWorldWidth() const { return worldWidth_; }
    float getWorldHeight() const { return worldHeight_; }

    // Used by resizable viewports. Keep an intentionally moved camera target;
    // if it was centered before the resize, keep it centered afterwards.
    void resizeWorld(float worldWidth, float worldHeight) {
        const bool wasCentered =
            std::fabs(camera_.target.x - worldWidth_ * 0.5f) < 0.001f &&
            std::fabs(camera_.target.y - worldHeight_ * 0.5f) < 0.001f;

        worldWidth_ = worldWidth;
        worldHeight_ = worldHeight;
        camera_.offset = { worldWidth_ * 0.5f, worldHeight_ * 0.5f };

        if (wasCentered) camera_.target = camera_.offset;
    }

    Camera2D getRaylibCamera() const { return camera_; }
    void begin() const { BeginMode2D(camera_); }
    void end() const { EndMode2D(); }
};

// Renders to a virtual canvas, then scales that canvas to the current window.
// This keeps game/UI coordinates and touch input consistent across sizes.
class Viewport {
protected:
    int worldWidth_;
    int worldHeight_;
    Rectangle screenBounds_{};
    RenderTexture2D target_{};
    bool initialized_ = false;
    Color clearColor_ = BLACK;
    OrthographicCamera camera_;

    virtual void calculateScreenBounds(int screenWidth, int screenHeight) = 0;

    // Allows derived viewports to change the virtual world dimensions during
    // resize (needed by ExtendViewport) while keeping the render target valid.
    void resizeWorld(int width, int height) {
        width = std::max(1, width);
        height = std::max(1, height);
        if (width == worldWidth_ && height == worldHeight_) return;

        worldWidth_ = width;
        worldHeight_ = height;
        camera_.resizeWorld(static_cast<float>(width), static_cast<float>(height));

        if (initialized_) {
            UnloadRenderTexture(target_);
            target_ = LoadRenderTexture(worldWidth_, worldHeight_);
            initialized_ = target_.id != 0;
        }
    }

public:
    Viewport(int worldWidth, int worldHeight)
        : worldWidth_(std::max(1, worldWidth)),
          worldHeight_(std::max(1, worldHeight)),
          camera_(static_cast<float>(std::max(1, worldWidth)),
                  static_cast<float>(std::max(1, worldHeight))) {}

    virtual ~Viewport() {
        if (initialized_) UnloadRenderTexture(target_);
    }

    Viewport(const Viewport&) = delete;
    Viewport& operator=(const Viewport&) = delete;

    bool initialize(Color clearColor = BLACK) {
        if (initialized_) UnloadRenderTexture(target_);
        target_ = LoadRenderTexture(worldWidth_, worldHeight_);
        initialized_ = target_.id != 0;
        clearColor_ = clearColor;
        refresh();
        return initialized_;
    }

    void refresh() {
        calculateScreenBounds(GetScreenWidth(), GetScreenHeight());
    }

    void setClearColor(Color color) { clearColor_ = color; }
    int getWorldWidth() const { return worldWidth_; }
    int getWorldHeight() const { return worldHeight_; }
    Rectangle getScreenBounds() const { return screenBounds_; }
    OrthographicCamera& getCamera() { return camera_; }
    const OrthographicCamera& getCamera() const { return camera_; }

    // Converts a raylib screen/touch position into virtual world coordinates.
    // Points in letterbox bars intentionally map outside the world rectangle.
    Vector2 unproject(Vector2 screenPosition) const {
        if (screenBounds_.width <= 0.0f || screenBounds_.height <= 0.0f) {
            return { 0.0f, 0.0f };
        }

        return {
            (screenPosition.x - screenBounds_.x) * worldWidth_ / screenBounds_.width,
            (screenPosition.y - screenBounds_.y) * worldHeight_ / screenBounds_.height
        };
    }

    void begin() {
        if (!initialized_) return;
        refresh();
        if (!initialized_) return;
        BeginTextureMode(target_);
        ClearBackground(clearColor_);
        camera_.begin();
    }

    void end() {
        if (!initialized_) return;
        camera_.end();
        EndTextureMode();

        ClearBackground(clearColor_);
        const Rectangle source = {
            0.0f, 0.0f,
            static_cast<float>(target_.texture.width),
            -static_cast<float>(target_.texture.height)
        };
        DrawTexturePro(target_.texture, source, screenBounds_, { 0.0f, 0.0f }, 0.0f, WHITE);
    }
};

// Preserves the whole virtual world and adds bars when aspect ratios differ.
class FitViewport : public Viewport {
protected:
    void calculateScreenBounds(int screenWidth, int screenHeight) override {
        if (screenWidth <= 0 || screenHeight <= 0) {
            screenBounds_ = { 0.0f, 0.0f, 0.0f, 0.0f };
            return;
        }

        const float scale = std::min(
            static_cast<float>(screenWidth) / worldWidth_,
            static_cast<float>(screenHeight) / worldHeight_);
        const float width = worldWidth_ * scale;
        const float height = worldHeight_ * scale;
        screenBounds_ = {
            (screenWidth - width) * 0.5f,
            (screenHeight - height) * 0.5f,
            width,
            height
        };
    }

public:
    FitViewport(int worldWidth, int worldHeight) : Viewport(worldWidth, worldHeight) {}
};

// Fills the screen while preserving aspect ratio; edges of the virtual world
// may be cropped on screens with a different aspect ratio.
class FillViewport : public Viewport {
protected:
    void calculateScreenBounds(int screenWidth, int screenHeight) override {
        if (screenWidth <= 0 || screenHeight <= 0) {
            screenBounds_ = { 0.0f, 0.0f, 0.0f, 0.0f };
            return;
        }

        const float scale = std::max(
            static_cast<float>(screenWidth) / worldWidth_,
            static_cast<float>(screenHeight) / worldHeight_);
        const float width = worldWidth_ * scale;
        const float height = worldHeight_ * scale;
        screenBounds_ = {
            (screenWidth - width) * 0.5f,
            (screenHeight - height) * 0.5f,
            width,
            height
        };
    }

public:
    FillViewport(int worldWidth, int worldHeight) : Viewport(worldWidth, worldHeight) {}
};

// LibGDX-style ExtendViewport. The minimum world size is always visible; the
// world expands along the longer screen axis to use the extra space. Optional
// maximum world dimensions cap that expansion, producing centered bars once a
// cap is reached. A maximum of 0 means unbounded on that axis.
class ExtendViewport : public Viewport {
private:
    int minWorldWidth_;
    int minWorldHeight_;
    int maxWorldWidth_;
    int maxWorldHeight_;

protected:
    void calculateScreenBounds(int screenWidth, int screenHeight) override {
        if (screenWidth <= 0 || screenHeight <= 0) {
            screenBounds_ = { 0.0f, 0.0f, 0.0f, 0.0f };
            return;
        }

        // Fit the minimum world dimensions first. Extending the other axis
        // preserves this scale and reveals additional world instead of cropping.
        const float scale = std::min(
            static_cast<float>(screenWidth) / minWorldWidth_,
            static_cast<float>(screenHeight) / minWorldHeight_);

        int extendedWidth = std::max(minWorldWidth_,
            static_cast<int>(std::lround(static_cast<float>(screenWidth) / scale)));
        int extendedHeight = std::max(minWorldHeight_,
            static_cast<int>(std::lround(static_cast<float>(screenHeight) / scale)));

        if (maxWorldWidth_ > 0) extendedWidth = std::min(extendedWidth, maxWorldWidth_);
        if (maxWorldHeight_ > 0) extendedHeight = std::min(extendedHeight, maxWorldHeight_);

        resizeWorld(extendedWidth, extendedHeight);

        const float width = worldWidth_ * scale;
        const float height = worldHeight_ * scale;
        screenBounds_ = {
            (screenWidth - width) * 0.5f,
            (screenHeight - height) * 0.5f,
            width,
            height
        };
    }

public:
    ExtendViewport(int minWorldWidth, int minWorldHeight,
                   int maxWorldWidth = 0, int maxWorldHeight = 0)
        : Viewport(minWorldWidth, minWorldHeight),
          minWorldWidth_(std::max(1, minWorldWidth)),
          minWorldHeight_(std::max(1, minWorldHeight)),
          maxWorldWidth_(std::max(0, maxWorldWidth)),
          maxWorldHeight_(std::max(0, maxWorldHeight)) {}
};

} // namespace Framework
