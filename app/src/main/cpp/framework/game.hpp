#pragma once

#include "raylib.h"
#include "screen.hpp"
#include <memory>

using namespace std;

namespace Framework {
    class Game {
    protected:
        std::unique_ptr<Screen> currentScreen;
        InputMultiplexer input;

    public:
        Game() = default;
        virtual ~Game() = default;

        // Processors added here receive input before the current screen.
        InputMultiplexer& getInput() { return input; }

        void setScreen(unique_ptr<Screen> screen) {
            if (currentScreen) {
                input.removeProcessor(currentScreen.get());
                currentScreen->unload();
            }

            currentScreen = std::move(screen);

            if (currentScreen) {
                currentScreen->refreshScreenSize();
                currentScreen->init();
                input.addProcessor(currentScreen.get());
            }
        }

        virtual void init() = 0;

        virtual void update(float dt) {
            if (currentScreen) {
                currentScreen->tweens.update(dt);
                currentScreen->particles.update(dt);
                currentScreen->update(dt);
            }
        }

        virtual void draw() {
            if (currentScreen) {
                currentScreen->draw();
            }
        }

        virtual void pause() {
            if (currentScreen) currentScreen->pause();
        }

        virtual void resume() {
            if (currentScreen) currentScreen->resume();
        }

        virtual void stop() {
            if (currentScreen) currentScreen->stop();
        }

        virtual void unload() {
            if (currentScreen) {
                input.removeProcessor(currentScreen.get());
                currentScreen->unload();
                currentScreen.reset();
            }
        }
    };
}
