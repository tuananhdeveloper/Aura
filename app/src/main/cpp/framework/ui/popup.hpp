#pragma once
#include "raylib.h"
#include "button.hpp"
#include "../tween.hpp"
#include <string>

using namespace std;

namespace Framework {
    class Popup {
    private:
        Rectangle windowRect;
        float screenWidth;
        float screenHeight;
        string title;
        string message;
        bool isOpen;
        unique_ptr<Button> actionButton;
        const float ROUNDNESS = 0.15f;
        const int SEGMENTS = 4;
        TweenManager tweens;
        float alpha = 1.0f;
        float scale = 1.0f;
        Vector2 buttonCenter = { 0.0f, 0.0f };

    public:
        Popup(float screenW, float screenH, const std::string& winTitle, const std::string& msg, const std::string& btnLabel)
                : screenWidth(screenW), screenHeight(screenH), title(winTitle), message(msg), isOpen(false) {
            float width = screenW * 0.82f;
            float height = 200.0f;
            windowRect = { (screenW - width) / 2.0f, (screenH - height) / 2.0f, width, height };

            Rectangle bounds = {windowRect.x + (width / 2.0f) - 75.0f, windowRect.y + height - 65.0f,
                                150.0f, 45.0f};
            buttonCenter = { bounds.x + bounds.width / 2, bounds.y + bounds.height / 2 };
            actionButton = make_unique<Button>(bounds, btnLabel, RED, RAYWHITE, 0.4f);
        }

        // Pops in with a fade and overshooting scale unless animate is false.
        void show(bool animate = true) {
            isOpen = true;
            tweens.cancelAll();
            if (!animate) {
                alpha = scale = 1.0f;
                actionButton->setAlpha(1.0f);
                return;
            }
            alpha = 0.0f;
            scale = 0.7f;
            tweens.start(Tween::to(&alpha, 1.0f, 0.2f)->ease(Ease::QuadOut));
            tweens.start(Tween::to(&scale, 1.0f, 0.35f)->ease(Ease::BackOut));
        }

        void hide() {
            tweens.cancelAll();
            isOpen = false;
        }

        // Advances the show animation; call once per frame.
        void tick(float dt) {
            tweens.update(dt);
        }

        bool isVisible() const {
            return isOpen;
        }

        bool update(Vector2 touchPosition, bool isTouching) {
            if (!isOpen) {
                return false;
            }
            if(actionButton->isPressed(touchPosition, isTouching)) {
                hide();
                return true;
            }
            return false;
        }

        void draw() {
            if (!isOpen) {
                return;
            }
            const float cx = windowRect.x + windowRect.width / 2;
            const float cy = windowRect.y + windowRect.height / 2;
            const Rectangle r = { cx - windowRect.width * scale / 2, cy - windowRect.height * scale / 2,
                                  windowRect.width * scale, windowRect.height * scale };

            DrawRectangle(0, 0, (int)screenWidth, (int)screenHeight, Fade(BLACK, 0.5f * alpha));
            DrawRectangleRounded(r, ROUNDNESS, SEGMENTS, Fade(GetColor(0x222233FF), alpha));
            DrawRectangleRoundedLines(r, ROUNDNESS, SEGMENTS, Fade(GetColor(0x444466FF), alpha));

            const int titleSize = (int)(24 * scale);
            const int titleWidth = MeasureText(title.c_str(), titleSize);
            DrawText(title.c_str(), (int)(cx - titleWidth / 2.0f),
                     (int)(cy + (25 - windowRect.height / 2) * scale), titleSize, Fade(RAYWHITE, alpha));

            const int msgSize = (int)(18 * scale);
            const int msgWidth = MeasureText(message.c_str(), msgSize);
            DrawText(message.c_str(), (int)(cx - msgWidth / 2.0f),
                     (int)(cy + (75 - windowRect.height / 2) * scale), msgSize, Fade(LIGHTGRAY, alpha));

            actionButton->setAlpha(alpha);
            actionButton->setScale(scale);
            actionButton->setOffset({ (buttonCenter.x - cx) * (scale - 1.0f), (buttonCenter.y - cy) * (scale - 1.0f) });
            actionButton->draw();
        }
    };
}
