#pragma once
#include "raylib.h"
#include "../utils/collision.hpp"
#include <string>

using namespace std;

namespace Framework {
    class Button {
    private:
        Rectangle bounds;
        string text;
        Color baseColor;
        Color textColor;
        float roundness;
        const int SEGMENTS = 4;
        float alpha = 1.0f;
        float scale = 1.0f;
        Vector2 offset = { 0.0f, 0.0f };

    public:
        Button() : bounds({0,0,0,0}), text(""), baseColor(DARKGRAY), textColor(RAYWHITE), roundness(0.3f) {}

        Button(Rectangle rect, const string& label, Color bg = DARKGRAY, Color fg = RAYWHITE, float round = 0.3f)
                : bounds(rect), text(label), baseColor(bg), textColor(fg), roundness(round) {}

        bool isPressed(Vector2 touchPosition, bool isTouching) {
            return isTouching && Collision::pointRect(touchPosition, bounds);
        }


        // Visual-only transforms for animation (hit testing uses the base bounds).
        float* alphaRef() { return &alpha; }
        float* scaleRef() { return &scale; }
        void setAlpha(float a) { alpha = a; }
        void setScale(float s) { scale = s; }
        void setOffset(Vector2 o) { offset = o; }

        void setBounds(Rectangle rect) {
            bounds = rect;
        }

        void setText(const string& label) {
            text = label;
        }

        void draw() {
            const float cx = bounds.x + bounds.width / 2 + offset.x;
            const float cy = bounds.y + bounds.height / 2 + offset.y;
            const Rectangle r = { cx - bounds.width * scale / 2, cy - bounds.height * scale / 2,
                                  bounds.width * scale, bounds.height * scale };
            const float a = alpha < 0.0f ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);

            DrawRectangleRounded(r, roundness, SEGMENTS, Fade(baseColor, a));
            DrawRectangleRoundedLines(r, roundness, SEGMENTS, Fade(WHITE, 0.4f * a));

            int fontSize = (int)(20 * scale);
            int textWidth = MeasureText(text.c_str(), fontSize);
            DrawText(text.c_str(),
                     (int)(cx - textWidth / 2),
                     (int)(cy - fontSize / 2),
                     fontSize,
                     Fade(textColor, a));
        }
    };
}
