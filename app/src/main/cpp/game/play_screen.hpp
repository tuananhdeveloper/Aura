#pragma once
#include "../framework/asset_manager.hpp"
#include "../framework/preferences.hpp"
#include "../framework/screen.hpp"
#include "../framework/viewport.hpp"
#include "../framework/ui/button.hpp"
#include "../framework/ui/popup.hpp"
#include "../framework/utils/collision.hpp"

#include <algorithm>
#include <memory>
#include <vector>

using namespace std;

class PlayScreen : public Framework::Screen {
private:
    struct Cell { int x; int y; };

    enum : int {
        MIN_WORLD_WIDTH = 450,
        MIN_WORLD_HEIGHT = 800
    };

    int tileSize = 35;
    int cols = 0;
    int rows = 0;
    int offsetX = 0;
    int offsetY = 0;
    std::vector<Cell> snake;
    Cell dir{ 1, 0 };
    Cell food{ 0, 0 };
    int score = 0;
    int highScore = 0;
    std::shared_ptr<Framework::ParticleEmitter> foodGlow;
    std::shared_ptr<Framework::ParticleEmitter> headTrail;
    struct Torch {
        std::shared_ptr<Framework::ParticleEmitter> fire, smoke;
        Vector2 pos{ 0, 0 };
    };
    std::vector<Torch> torches;
    Framework::ParticleSystem ambient;  // drawn behind the board
    std::shared_ptr<Framework::ParticleEmitter> ambientEmitter;
    float uiAlpha = 0.0f;       // HUD fade-in
    float controlsSlide = 0.0f; // px the d-pad is pushed down while sliding in
    std::unique_ptr<Framework::Preferences> prefs;
    float moveTimer = 0.0f;
    float moveDelay = 0.15f;

    std::unique_ptr<Framework::ExtendViewport> viewport;
    std::unique_ptr<Framework::Popup> gameOverPopup;
    std::unique_ptr<Framework::Button> btnLeft, btnRight, btnUp, btnDown;

    Framework::AssetManager assets;
    Music bgMusic{};
    Sound eatSound{};

    void respawnFood() {
        do {
            food = { GetRandomValue(0, cols - 1), GetRandomValue(0, rows - 1) };
        } while (isSnakeAt(food.x, food.y));
    }

    Vector2 cellCenter(Cell c) const {
        return { static_cast<float>(offsetX + c.x * tileSize + tileSize / 2),
                 static_cast<float>(offsetY + c.y * tileSize + tileSize / 2) };
    }

    // Confetti fountain from the middle of the board's top edge.
    void celebrate() {
        const Vector2 origin = { offsetX + cols * tileSize / 2.0f, offsetY + rows * tileSize * 0.8f };
        for (Color c : { Color{ 255, 90, 90, 255 }, Color{ 255, 220, 80, 255 },
                         Color{ 90, 200, 255, 255 }, Color{ 150, 255, 120, 255 } }) {
            particles.burst(Framework::ParticlePresets::confetti(c), origin, 25);
        }
    }

    Vector2 foodCenter() const { return cellCenter(food); }

    bool isSnakeAt(int x, int y) const {
        for (const auto& part : snake) {
            if (part.x == x && part.y == y) return true;
        }
        return false;
    }

    // Re-layout all screen-space UI and the board whenever ExtendViewport's
    // virtual world grows after a device/window resize or orientation change.
    void updateLayout() {
        if (!viewport) return;

        screenWidth = viewport->getWorldWidth();
        screenHeight = viewport->getWorldHeight();

        const int paddingSide = 20;
        const int paddingTop = 90;
        const int controlsHeight = 230;
        const int availableWidth = std::max(1, screenWidth - paddingSide * 2);
        const int availableHeight = std::max(1, screenHeight - paddingTop - controlsHeight);

        cols = std::max(8, availableWidth / tileSize);
        rows = std::max(8, availableHeight / tileSize);
        offsetX = paddingSide + (availableWidth - cols * tileSize) / 2;
        offsetY = paddingTop;

        if (ambientEmitter) {
            ambientEmitter->config().spawnRect = { static_cast<float>(cols * tileSize),
                                                   static_cast<float>(rows * tileSize) };
            ambientEmitter->setPosition({ offsetX + cols * tileSize / 2.0f,
                                          offsetY + rows * tileSize / 2.0f });
        }

        if (!torches.empty()) {
            const float m = 8.0f;
            const float l = offsetX - m, r = offsetX + cols * tileSize + m;
            const float t = offsetY - m, b = offsetY + rows * tileSize + m;
            const Vector2 corners[4] = { { l, t }, { r, t }, { l, b }, { r, b } };
            for (size_t i = 0; i < torches.size(); ++i) {
                torches[i].pos = corners[i];
                torches[i].fire->setPosition({ corners[i].x, corners[i].y - 4 });
                torches[i].smoke->setPosition({ corners[i].x, corners[i].y - 14 });
            }
        }

        const bool popupWasVisible = gameOverPopup && gameOverPopup->isVisible();
        gameOverPopup = std::make_unique<Framework::Popup>(
            static_cast<float>(screenWidth), static_cast<float>(screenHeight),
            "GAME OVER", "You crashed into the wall!", "RESTART");
        if (popupWasVisible) gameOverPopup->show(false);

        const float dpadY = static_cast<float>(screenHeight - controlsHeight);
        btnLeft = std::make_unique<Framework::Button>(Rectangle{ 30, dpadY + 40, 80, 80 },
                                                       "\u25c4", DARKGRAY, RAYWHITE, 0.3f);
        btnRight = std::make_unique<Framework::Button>(Rectangle{ 210, dpadY + 40, 80, 80 },
                                                        "\u25ba", DARKGRAY, RAYWHITE, 0.3f);
        btnUp = std::make_unique<Framework::Button>(Rectangle{ 120, dpadY - 30, 80, 80 },
                                                     "\u25b2", DARKGRAY, RAYWHITE, 0.3f);
        btnDown = std::make_unique<Framework::Button>(Rectangle{ 120, dpadY + 110, 80, 80 },
                                                       "\u25bc", DARKGRAY, RAYWHITE, 0.3f);
    }

public:
    void init() override {
        // Min dimensions remain visible; larger screen axes add world space.
        viewport = std::make_unique<Framework::ExtendViewport>(
            MIN_WORLD_WIDTH, MIN_WORLD_HEIGHT);
        viewport->initialize(GetColor(0x181824FF));
        updateLayout();

#if (defined(__APPLE__) && !TARGET_OS_IOS) || defined(__WIN32__) || defined(__linux__)
        ChangeDirectory("assets");
#endif

        prefs = std::make_unique<Framework::Preferences>("snake");
        highScore = prefs->getInteger("highScore", 0);

        assets.load<Music>("bgm.mp3");
        assets.load<Sound>("eat.mp3");
        assets.finishLoading();
        bgMusic = assets.get<Music>("bgm.mp3");
        eatSound = assets.get<Sound>("eat.mp3");

        PlayMusicStream(bgMusic);
        SetMusicVolume(bgMusic, 0.5f);
        resetGame();

        auto glow = Framework::ParticlePresets::magic();
        glow.emissionRate = 15;
        glow.spawnRadius = tileSize * 0.4f;
        foodGlow = particles.emit(glow, foodCenter());

        headTrail = particles.emit(Framework::ParticlePresets::trail(), cellCenter(snake[0]));
        headTrail->setAutoRemove(false);

        // Corner torches: flame plus a thin wisp of smoke above it.
        auto flame = Framework::ParticlePresets::fire();
        flame.emissionRate = 35;
        flame.maxParticles = 60;
        flame.startSize = { 3, 6 };
        flame.endSize = { 0, 1 };
        flame.speed = { 15, 40 };
        flame.spawnRadius = 3;
        auto wisp = Framework::ParticlePresets::smoke();
        wisp.emissionRate = 6;
        wisp.maxParticles = 30;
        wisp.startSize = { 3, 5 };
        wisp.endSize = { 9, 14 };
        for (int i = 0; i < 4; ++i) {
            Torch t;
            t.fire = particles.emit(flame, { 0, 0 });
            t.smoke = particles.emit(wisp, { 0, 0 });
            torches.push_back(t);
        }
        ambientEmitter = ambient.emit(Framework::ParticlePresets::ambient(), { 0, 0 });
        updateLayout();

        // Intro: HUD fades in, controls slide up from below the screen.
        controlsSlide = 300.0f;
        tweens.start(Framework::Tween::to(&uiAlpha, 1.0f, 0.5f)->ease(Framework::Ease::QuadOut));
        tweens.start(Framework::Tween::to(&controlsSlide, 0.0f, 0.6f)->delay(0.1f)->ease(Framework::Ease::BackOut));
    }

    void resetGame() {
        snake = { { cols/2, rows/2 }, { cols/2 - 1, rows/2 }, { cols/2 - 2, rows/2 } };
        dir = { 1, 0 };
        score = 0;
        moveTimer = 0.0f;
        moveDelay = 0.15f;
        respawnFood();
        if (gameOverPopup) gameOverPopup->hide();
        PlayMusicStream(bgMusic);
        if (headTrail) headTrail->start();
    }

    void update(float dt) override {
        if (gameOverPopup) gameOverPopup->tick(dt);
        if (foodGlow) foodGlow->setPosition(foodCenter());
        if (headTrail && !snake.empty()) headTrail->setPosition(cellCenter(snake[0]));
        ambient.update(dt);
        if (viewport) {
            viewport->refresh();
            if (screenWidth != viewport->getWorldWidth() ||
                screenHeight != viewport->getWorldHeight()) {
                updateLayout();
            }
        }

        UpdateMusicStream(bgMusic);

        if (gameOverPopup && gameOverPopup->isVisible()) return;

        moveTimer += dt;
        if (moveTimer >= moveDelay) {
            moveTimer = 0.0f;
            const Cell newHead = { snake[0].x + dir.x, snake[0].y + dir.y };

            if (newHead.x < 0 || newHead.x >= cols || newHead.y < 0 ||
                newHead.y >= rows || isSnakeAt(newHead.x, newHead.y)) {
                if (score > highScore) {
                    highScore = score;
                    prefs->putInteger("highScore", highScore);
                    prefs->flush();
                }
                particles.burst(Framework::ParticlePresets::explosion(), cellCenter(snake[0]), 80);

                // Sparks spray back from the impact point, opposite the travel direction.
                auto hit = Framework::ParticlePresets::sparks();
                hit.angle = std::atan2(static_cast<float>(-dir.y), static_cast<float>(-dir.x)) * RAD2DEG;
                hit.spread = 90;
                const Vector2 c = cellCenter(snake[0]);
                particles.burst(hit, { c.x + dir.x * tileSize * 0.5f, c.y + dir.y * tileSize * 0.5f }, 30);
                if (headTrail) headTrail->stop();
                gameOverPopup->show();
                StopMusicStream(bgMusic);
            } else {
                snake.insert(snake.begin(), newHead);

                const Rectangle foodRect = {
                    static_cast<float>(offsetX + food.x * tileSize),
                    static_cast<float>(offsetY + food.y * tileSize),
                    static_cast<float>(tileSize), static_cast<float>(tileSize)
                };
                const Rectangle headRect = {
                    static_cast<float>(offsetX + newHead.x * tileSize),
                    static_cast<float>(offsetY + newHead.y * tileSize),
                    static_cast<float>(tileSize), static_cast<float>(tileSize)
                };

                if (Framework::Collision::rectRect(headRect, foodRect)) {
                    score += 10;
                    if (score % 100 == 0) celebrate();
                    PlaySound(eatSound);
                    particles.burst(Framework::ParticlePresets::sparks(), foodCenter(), 24);
                    respawnFood();
                } else {
                    snake.pop_back();
                }
            }
        }
    }

    void draw() override {
        if (!viewport) return;
        viewport->begin();
        ClearBackground(GetColor(0x181824FF));
        ambient.draw();

        DrawRectangleRec({ static_cast<float>(offsetX), static_cast<float>(offsetY),
                           static_cast<float>(cols * tileSize), static_cast<float>(rows * tileSize) },
                         GetColor(0x222233FF));

        DrawRectangle(offsetX + food.x * tileSize + 2, offsetY + food.y * tileSize + 2,
                      tileSize - 4, tileSize - 4, RED);
        for (size_t i = 0; i < snake.size(); ++i) {
            DrawRectangle(offsetX + snake[i].x * tileSize + 2,
                          offsetY + snake[i].y * tileSize + 2,
                          tileSize - 4, tileSize - 4,
                          (i == 0) ? LIME : GREEN);
        }

        for (const auto& t : torches) {
            DrawRectangleRounded({ t.pos.x - 5, t.pos.y - 2, 10, 10 }, 0.4f, 4, GetColor(0x5A4632FF));
        }
        particles.draw();

        DrawText(TextFormat("SCORE: %i", score), 25, 25, 28, Fade(RAYWHITE, uiAlpha));
        DrawText(TextFormat("BEST: %i", highScore), 25, 58, 20, Fade(GRAY, uiAlpha));
        for (auto* b : { btnLeft.get(), btnRight.get(), btnUp.get(), btnDown.get() }) {
            b->setOffset({ 0.0f, controlsSlide });
            b->draw();
        }
        gameOverPopup->draw();
        viewport->end();
    }

    bool touchDown(int x, int y, int) override {
        if (!viewport) return false;
        const Vector2 touchPos = viewport->unproject({ static_cast<float>(x), static_cast<float>(y) });

        if (gameOverPopup && gameOverPopup->isVisible()) {
            if (gameOverPopup->update(touchPos, true)) resetGame();
            return true;
        }

        if (btnLeft->isPressed(touchPos, true) && dir.x == 0) dir = { -1, 0 };
        else if (btnRight->isPressed(touchPos, true) && dir.x == 0) dir = { 1, 0 };
        else if (btnUp->isPressed(touchPos, true) && dir.y == 0) dir = { 0, -1 };
        else if (btnDown->isPressed(touchPos, true) && dir.y == 0) dir = { 0, 1 };
        else return false;
        return true;
    }

    void pause() override {
        if (prefs) prefs->flush();
        PauseMusicStream(bgMusic);
    }

    void resume() override {
        ResumeMusicStream(bgMusic);
    }

    void unload() override {
        if (prefs) prefs->flush();
        tweens.cancelAll();
        foodGlow.reset();
        headTrail.reset();
        torches.clear();
        ambientEmitter.reset();
        particles.clear();
        ambient.clear();
        assets.clear();
        viewport.reset();
    }
};
