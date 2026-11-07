#pragma once

#include "platform.hpp"
#include "raylib.h"
#include "rgestures.h"

namespace Framework {
    class Input {
    public:
        static bool isPressed() {
            if (Platform::isMobile()) {
                return GetTouchPointCount() > 0;
            }

            if (Platform::isWeb()) {
                return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || GetTouchPointCount() > 0;
            }

            return IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        }

        static Vector2 getPosition() {
            if (Platform::isMobile() && GetTouchPointCount() > 0) {
                return GetTouchPosition(0);
            }

            if (Platform::isWeb() && GetTouchPointCount() > 0) {
                return GetTouchPosition(0);
            }

            return GetMousePosition();
        }
    };
}
