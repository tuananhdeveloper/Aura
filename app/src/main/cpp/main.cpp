#include "framework/core.hpp"
#include "framework/game.hpp"
#include "play_screen.hpp"
#include "game/my_game.hpp"
#include <stdbool.h>

using namespace std;

Framework::Core app("@@NAME@@");
auto game = make_unique<MyGame>();

#if defined(__APPLE__) && TARGET_OS_IOS
extern "C" {
    void ios_ready() {
        app.ready(game.get());
    }

    void ios_update(bool update) {
        app.update(game.get());
    }

    void ios_pause() {
        app.pause(game.get());
    }

    void ios_resume() {
        app.resume(game.get());
    }

    void ios_stop() {
        app.stop(game.get());
    }

    void ios_destroy() {
        app.destroy(game.get());
    }
}
#else
int main() {
    app.run(game.get());
    return 0;
}
#endif