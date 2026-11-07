#pragma once

#include "raylib.h"
#include <algorithm>
#include <array>
#include <vector>

namespace Framework {
    // LibGDX-style event listener. Coordinates are screen pixels; use
    // Viewport::unproject() to convert to world space. Return true from a
    // handler to mark the event as consumed.
    class InputProcessor {
    public:
        virtual ~InputProcessor() = default;
        virtual bool touchDown(int x, int y, int pointer) { return false; }
        virtual bool touchUp(int x, int y, int pointer) { return false; }
        virtual bool touchDragged(int x, int y, int pointer) { return false; }
        virtual bool keyDown(int key) { return false; }
        virtual bool keyUp(int key) { return false; }
        virtual bool scrolled(float amountX, float amountY) { return false; }
    };

    // Dispatches events to processors in order until one consumes it.
    // Non-owning: processors must outlive their registration.
    class InputMultiplexer : public InputProcessor {
        std::vector<InputProcessor*> processors_;

        template <typename F> bool dispatch(F&& f) {
            // Copy so processors may add/remove during dispatch.
            const auto snapshot = processors_;
            for (auto* p : snapshot) if (f(p)) return true;
            return false;
        }

    public:
        void addProcessor(InputProcessor* p) {
            if (p && std::find(processors_.begin(), processors_.end(), p) == processors_.end())
                processors_.push_back(p);
        }
        void addProcessor(size_t index, InputProcessor* p) {
            if (!p) return;
            removeProcessor(p);
            processors_.insert(processors_.begin() + std::min(index, processors_.size()), p);
        }
        void removeProcessor(InputProcessor* p) {
            processors_.erase(std::remove(processors_.begin(), processors_.end(), p), processors_.end());
        }
        void clear() { processors_.clear(); }
        size_t size() const { return processors_.size(); }

        bool touchDown(int x, int y, int pointer) override {
            return dispatch([&](auto* p) { return p->touchDown(x, y, pointer); });
        }
        bool touchUp(int x, int y, int pointer) override {
            return dispatch([&](auto* p) { return p->touchUp(x, y, pointer); });
        }
        bool touchDragged(int x, int y, int pointer) override {
            return dispatch([&](auto* p) { return p->touchDragged(x, y, pointer); });
        }
        bool keyDown(int key) override {
            return dispatch([&](auto* p) { return p->keyDown(key); });
        }
        bool keyUp(int key) override {
            return dispatch([&](auto* p) { return p->keyUp(key); });
        }
        bool scrolled(float ax, float ay) override {
            return dispatch([&](auto* p) { return p->scrolled(ax, ay); });
        }
    };

    // Polls raylib once per frame (call poll() before updating the game) and
    // turns state changes into InputProcessor events. Supports multi-touch:
    // each finger is a pointer, ordered by touch slot. The mouse is pointer 0
    // while no finger is down.
    class InputManager {
    public:
        static constexpr int MAX_POINTERS = 10;

    private:
        struct Pointer {
            bool down = false;
            int touchId = -1;
            int x = 0, y = 0;
        };

        std::array<Pointer, MAX_POINTERS> pointers_{};
        InputProcessor* processor_ = nullptr;
        bool mouseDown_ = false;
        int lastMouseX_ = 0, lastMouseY_ = 0;

        void pollTouches() {
            const int count = std::min(GetTouchPointCount(), MAX_POINTERS);
            std::array<bool, MAX_POINTERS> seen{};

            for (int i = 0; i < count; ++i) {
                const int id = GetTouchPointId(i);
                const Vector2 pos = GetTouchPosition(i);
                const int x = static_cast<int>(pos.x), y = static_cast<int>(pos.y);

                int slot = -1;
                for (int s = 0; s < MAX_POINTERS; ++s)
                    if (pointers_[s].down && pointers_[s].touchId == id) { slot = s; break; }

                if (slot < 0) {
                    for (int s = 0; s < MAX_POINTERS; ++s)
                        if (!pointers_[s].down) { slot = s; break; }
                    if (slot < 0) continue;
                    pointers_[slot] = { true, id, x, y };
                    seen[slot] = true;
                    if (processor_) processor_->touchDown(x, y, slot);
                } else {
                    seen[slot] = true;
                    if (pointers_[slot].x != x || pointers_[slot].y != y) {
                        pointers_[slot].x = x;
                        pointers_[slot].y = y;
                        if (processor_) processor_->touchDragged(x, y, slot);
                    }
                }
            }

            for (int s = 0; s < MAX_POINTERS; ++s) {
                if (pointers_[s].down && !seen[s] && pointers_[s].touchId >= 0) {
                    const Pointer p = pointers_[s];
                    pointers_[s] = {};
                    if (processor_) processor_->touchUp(p.x, p.y, s);
                }
            }
        }

        bool anyTouchActive() const {
            for (const auto& p : pointers_) if (p.down && p.touchId >= 0) return true;
            return false;
        }

        void pollMouse() {
            const Vector2 pos = GetMousePosition();
            const int x = static_cast<int>(pos.x), y = static_cast<int>(pos.y);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                mouseDown_ = true;
                pointers_[0] = { true, -1, x, y };
                if (processor_) processor_->touchDown(x, y, 0);
            } else if (mouseDown_ && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                mouseDown_ = false;
                pointers_[0] = {};
                if (processor_) processor_->touchUp(x, y, 0);
            } else if (mouseDown_ && (x != lastMouseX_ || y != lastMouseY_)) {
                pointers_[0].x = x;
                pointers_[0].y = y;
                if (processor_) processor_->touchDragged(x, y, 0);
            }
            lastMouseX_ = x;
            lastMouseY_ = y;
        }

        void pollKeys() {
            if (!processor_) return;
            int key;
            while ((key = GetKeyPressed()) != 0) processor_->keyDown(key);
            // Poll released keys only for keys we could have pressed.
            for (int k = 1; k < 350; ++k) if (IsKeyReleased(k)) processor_->keyUp(k);
        }

    public:
        static InputManager& get() {
            static InputManager instance;
            return instance;
        }

        void setInputProcessor(InputProcessor* p) { processor_ = p; }
        InputProcessor* getInputProcessor() const { return processor_; }

        void poll() {
            pollTouches();
            // Mouse input is skipped while fingers are down so touch-emulated
            // mouse events on mobile/web are not reported twice.
            if (!anyTouchActive()) pollMouse();

            pollKeys();

            const Vector2 wheel = GetMouseWheelMoveV();
            if (processor_ && (wheel.x != 0.0f || wheel.y != 0.0f))
                processor_->scrolled(wheel.x, wheel.y);
        }

        // Polling-style queries (like Gdx.input.isTouched(pointer)).
        bool isTouched(int pointer = 0) const {
            return pointer >= 0 && pointer < MAX_POINTERS && pointers_[pointer].down;
        }
        int getX(int pointer = 0) const { return isTouched(pointer) ? pointers_[pointer].x : 0; }
        int getY(int pointer = 0) const { return isTouched(pointer) ? pointers_[pointer].y : 0; }
        int getTouchCount() const {
            int n = 0;
            for (const auto& p : pointers_) if (p.down) ++n;
            return n;
        }
    };
}
