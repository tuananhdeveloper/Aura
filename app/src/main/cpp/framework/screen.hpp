#pragma once
#include "raylib.h"
#include "input_manager.hpp"
#include "tween.hpp"
#include "particle_system.hpp"

namespace Framework {
    class Screen : public InputProcessor {
    protected:
        int screenWidth;
        int screenHeight;

    public:
        // Updated by Game before update(); stopped with the screen.
        TweenManager tweens;
        // Updated by Game before update(); draw it inside your viewport.
        ParticleSystem particles;

        Screen() : screenWidth(0), screenHeight(0) {}
        virtual ~Screen() = default;

        void refreshScreenSize() {
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();
        }

        virtual void init() = 0;
        virtual void update(float dt) = 0;
        virtual void draw() = 0;
        virtual void unload() = 0;

        // Lifecycle hooks (optional)
        virtual void pause() {}   // app lost focus / went to background
        virtual void resume() {}  // app regained focus
        virtual void stop() {}    // window hidden/minimized or app is closing
    };
}