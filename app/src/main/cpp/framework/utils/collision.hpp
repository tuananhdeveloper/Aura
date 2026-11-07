#pragma once
#include "raylib.h"

namespace Framework {
    class Collision {
    public:
        static bool pointRect(Vector2 point, Rectangle rect) {
            return CheckCollisionPointRec(point, rect);
        }

        static bool pointCircle(Vector2 point, Vector2 center, float radius) {
            return CheckCollisionPointCircle(point, center, radius);
        }

        static bool rectRect(Rectangle r1, Rectangle r2) {
            return CheckCollisionRecs(r1, r2);
        }

        static bool circleCircle(Vector2 center1, float rad1, Vector2 center2, float rad2) {
            return CheckCollisionCircles(center1, rad1, center2, rad2);
        }
    };
}