#pragma once

#include "game.hpp"
#include "raylib.h"
#include "platform.hpp"
#include "input_manager.hpp"
#include <memory>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

#if defined(__ANDROID__)
#include <android_native_app_glue.h>
extern "C" struct android_app* GetAndroidApp(void);
#endif

using namespace std;

namespace Framework {
    class Core {
    private:
        const char* windowTitle;
        const int FPS = 60;

        enum class State { Running, Paused, Stopped };
        State state = State::Running;

#if defined(__EMSCRIPTEN__)
        Game* webGame = nullptr;

        static void webFrame(void* context) {
            Core* core = static_cast<Core*>(context);
            core->update(core->webGame);
        }
#endif

#if defined(__ANDROID__)
        // raylib blocks inside EndDrawing() while the Android app is in the
        // background and clears its focus flags before the next frame, so
        // polling never sees the pause. Hook the activity commands instead.
        static inline Core* androidCore = nullptr;
        static inline Game* androidGame = nullptr;
        static inline void (*androidPrevCmd)(struct android_app*, int32_t) = nullptr;

        static void androidCommand(struct android_app* app, int32_t cmd) {
            if (androidPrevCmd) androidPrevCmd(app, cmd);
            if (!androidCore) return;
            switch (cmd) {
                case APP_CMD_PAUSE:
                    androidCore->transitionTo(androidGame, State::Paused);
                    break;
                case APP_CMD_STOP:
                    androidCore->transitionTo(androidGame, State::Stopped);
                    break;
                case APP_CMD_RESUME:
                    androidCore->transitionTo(androidGame, State::Running);
                    break;
                default: break;
            }
        }

        void installAndroidHooks(Game* game) {
            struct android_app* app = GetAndroidApp();
            if (!app || app->onAppCmd == androidCommand) return;
            androidCore = this;
            androidGame = game;
            androidPrevCmd = app->onAppCmd;
            app->onAppCmd = androidCommand;
        }
#endif

        State queryState() const {
            if (IsWindowHidden() || IsWindowMinimized()) return State::Stopped;
            if (!IsWindowFocused()) return State::Paused;
            return State::Running;
        }

        void transitionTo(Game* game, State next) {
            if (next == state) return;
            State prev = state;
            state = next;
            if (!game) return;

            if (next == State::Running) {
                game->resume();
            } else if (next == State::Paused) {
                if (prev == State::Running) game->pause();
                else { game->resume(); game->pause(); }
            } else {
                if (prev == State::Running) game->pause();
                game->stop();
            }
        }

    public:
        Core(const char* title): windowTitle(title) {}

        void ready(Game* game) {
            Platform::configureWindow(windowTitle, Platform::defaultWidth(), Platform::defaultHeight());
#if defined(__ANDROID__)
            installAndroidHooks(game);
#endif
#if !defined(__EMSCRIPTEN__)
            SetTargetFPS(FPS);
#endif
            InitAudioDevice();

            if (game != nullptr) {
                game->init();
                InputManager::get().setInputProcessor(&game->getInput());
            }
        }

        // Platform entry points (e.g. iOS app delegate) can force a state.
        void pause(Game* game) { transitionTo(game, State::Paused); }
        void resume(Game* game) { transitionTo(game, State::Running); }
        void stop(Game* game) { transitionTo(game, State::Stopped); }

        void update(Game* game) {
#if !defined(__ANDROID__)
            transitionTo(game, queryState());
#endif
            float dt = GetFrameTime();

            if (state == State::Running) InputManager::get().poll();

            if (game != nullptr && state == State::Running) {
                game->update(dt);
            }

            BeginDrawing();
            ClearBackground(RAYWHITE);

            if (game != nullptr) {
                game->draw();
            }

            EndDrawing();
        }

        void destroy(Game* game) {
            InputManager::get().setInputProcessor(nullptr);
#if defined(__ANDROID__)
            androidCore = nullptr;
            androidGame = nullptr;
#endif
            if (game) {
                if (state == State::Running) game->pause();
                if (state != State::Stopped) game->stop();
                state = State::Stopped;
                game->unload();
            }

            CloseAudioDevice();
            CloseWindow();
        }

        int run(Game* instance) {
            ready(instance);

#if defined(__EMSCRIPTEN__)
            webGame = instance;
            emscripten_set_main_loop_arg(webFrame, this, 0, 1);
#else
            while (!WindowShouldClose()) {
                update(instance);
            }

            destroy(instance);
#endif

            return 0;
        }
    };
}
