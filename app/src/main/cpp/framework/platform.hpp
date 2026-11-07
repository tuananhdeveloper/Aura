#pragma once

#include "raylib.h"

namespace Framework {
    enum class PlatformType {
        Android,
        IOS,
        Desktop,
        Web
    };

    class Platform {
    public:
        static PlatformType current() {
            #if defined(__ANDROID__) || defined(ANDROID) || defined(PLATFORM_ANDROID)
                return PlatformType::Android;
            #elif defined(__EMSCRIPTEN__) || defined(PLATFORM_WEB)
                return PlatformType::Web;
            #elif defined(__APPLE__)
                #if TARGET_OS_IOS
                    return PlatformType::IOS;
                #else
                    return PlatformType::Desktop;
                #endif
            #else
                return PlatformType::Desktop;
            #endif
        }

        static bool isAndroid() {
            return current() == PlatformType::Android;
        }

        static bool isIOS() {
            return current() == PlatformType::IOS;
        }

        static bool isWeb() {
            return current() == PlatformType::Web;
        }

        static bool isDesktop() {
            return current() == PlatformType::Desktop;
        }

        static bool isMobile() {
            return isAndroid() || isIOS();
        }

        static int defaultWidth() {
            return isMobile() ? 0 : 1280;
        }

        static int defaultHeight() {
            return isMobile() ? 0 : 720;
        }

        static void configureWindow(const char* title, int width, int height) {
            if (isMobile()) {
                SetConfigFlags(FLAG_FULLSCREEN_MODE);
            } else {
                SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
            }

            InitWindow(width, height, title);
        }
    };
}
