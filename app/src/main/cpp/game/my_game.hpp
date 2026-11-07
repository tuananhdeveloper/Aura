#include "framework/game.hpp"
#include "play_screen.hpp"

class MyGame : public Framework::Game {
public:
    void init() override {
        setScreen(std::make_unique<PlayScreen>());
    }
};