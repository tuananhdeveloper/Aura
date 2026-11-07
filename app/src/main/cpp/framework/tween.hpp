#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <vector>

namespace Framework {
    enum class Ease {
        Linear,
        QuadIn, QuadOut, QuadInOut,
        CubicIn, CubicOut, CubicInOut,
        SineIn, SineOut, SineInOut,
        ExpoOut,
        BackIn, BackOut,
        ElasticOut,
        BounceOut
    };

    // Maps linear progress t in [0,1] to eased progress. Back/Elastic overshoot
    // outside [0,1] on purpose.
    inline float applyEase(Ease e, float t) {
        constexpr float PI_F = 3.14159265358979f;
        switch (e) {
            case Ease::Linear: return t;
            case Ease::QuadIn: return t * t;
            case Ease::QuadOut: return 1.0f - (1.0f - t) * (1.0f - t);
            case Ease::QuadInOut:
                return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
            case Ease::CubicIn: return t * t * t;
            case Ease::CubicOut: return 1.0f - std::pow(1.0f - t, 3.0f);
            case Ease::CubicInOut:
                return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
            case Ease::SineIn: return 1.0f - std::cos(t * PI_F / 2.0f);
            case Ease::SineOut: return std::sin(t * PI_F / 2.0f);
            case Ease::SineInOut: return -(std::cos(PI_F * t) - 1.0f) / 2.0f;
            case Ease::ExpoOut: return t >= 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
            case Ease::BackIn: {
                const float c1 = 1.70158f;
                return (c1 + 1.0f) * t * t * t - c1 * t * t;
            }
            case Ease::BackOut: {
                const float c1 = 1.70158f;
                const float u = t - 1.0f;
                return 1.0f + (c1 + 1.0f) * u * u * u + c1 * u * u;
            }
            case Ease::ElasticOut: {
                if (t <= 0.0f) return 0.0f;
                if (t >= 1.0f) return 1.0f;
                return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * (2.0f * PI_F / 3.0f)) + 1.0f;
            }
            case Ease::BounceOut: {
                const float n1 = 7.5625f, d1 = 2.75f;
                if (t < 1.0f / d1) return n1 * t * t;
                if (t < 2.0f / d1) { t -= 1.5f / d1; return n1 * t * t + 0.75f; }
                if (t < 2.5f / d1) { t -= 2.25f / d1; return n1 * t * t + 0.9375f; }
                t -= 2.625f / d1;
                return n1 * t * t + 0.984375f;
            }
        }
        return t;
    }

    // Animates a float from one value to another. Configure with the chained
    // setters, then hand it to a TweenManager:
    //
    //   tweens.start(Tween::to(&alpha, 1.0f, 0.3f)->ease(Ease::QuadOut));
    //
    // Tweens that write through a float* must be cancelled (TweenManager::cancel)
    // before that float is destroyed.
    class Tween : public std::enable_shared_from_this<Tween> {
    public:
        using Ptr = std::shared_ptr<Tween>;

    private:
        float* target_ = nullptr;
        std::function<void(float)> onUpdate_;
        std::function<void()> onStart_;
        std::function<void()> onComplete_;
        const void* tag_ = nullptr;
        std::vector<Ptr> chained_;

        float from_ = 0.0f;
        float to_ = 0.0f;
        bool fromFixed_ = false;
        float duration_ = 0.0f;
        float delay_ = 0.0f;
        Ease ease_ = Ease::Linear;
        int repeat_ = 0;  // extra iterations; -1 repeats forever
        bool yoyo_ = false;

        float elapsed_ = 0.0f;
        bool started_ = false;
        bool finished_ = false;
        bool cancelled_ = false;

        void apply(float p) {
            const float v = from_ + (to_ - from_) * applyEase(ease_, p);
            if (target_) *target_ = v;
            if (onUpdate_) onUpdate_(v);
        }

        void begin() {
            started_ = true;
            if (target_ && !fromFixed_) from_ = *target_;
            if (onStart_) onStart_();
        }

    public:
        // Animates *target from its current value (read when the tween starts).
        static Ptr to(float* target, float to, float duration) {
            auto t = std::make_shared<Tween>();
            t->target_ = target;
            t->tag_ = target;
            t->to_ = to;
            t->duration_ = duration;
            return t;
        }

        static Ptr fromTo(float* target, float from, float to, float duration) {
            auto t = Tween::to(target, to, duration);
            t->from_ = from;
            t->fromFixed_ = true;
            return t;
        }

        // Calls onUpdate each frame with the interpolated value.
        static Ptr value(float from, float to, float duration, std::function<void(float)> onUpdate) {
            auto t = std::make_shared<Tween>();
            t->from_ = from;
            t->fromFixed_ = true;
            t->to_ = to;
            t->duration_ = duration;
            t->onUpdate_ = std::move(onUpdate);
            return t;
        }

        // Pause before the tween starts (e.g. to stagger several elements).
        Ptr delay(float seconds) { delay_ = std::max(0.0f, seconds); return shared_from_this(); }
        Ptr ease(Ease e) { ease_ = e; return shared_from_this(); }
        // repeat(n) runs n extra times; repeat(-1) loops forever.
        Ptr repeat(int count) { repeat_ = count; return shared_from_this(); }
        // With repeat, alternate direction each iteration.
        Ptr yoyo(bool enabled = true) { yoyo_ = enabled; return shared_from_this(); }
        // Groups tweens so TweenManager::cancel(tag) can stop them together.
        Ptr tag(const void* t) { tag_ = t; return shared_from_this(); }
        Ptr onStart(std::function<void()> f) { onStart_ = std::move(f); return shared_from_this(); }
        Ptr onComplete(std::function<void()> f) { onComplete_ = std::move(f); return shared_from_this(); }
        // Starts `next` when this tween completes (not if this one is cancelled).
        Ptr then(Ptr next) { chained_.push_back(std::move(next)); return shared_from_this(); }

        void cancel() { cancelled_ = true; }
        bool isFinished() const { return finished_; }
        bool isCancelled() const { return cancelled_; }
        bool isDone() const { return finished_ || cancelled_; }
        const void* getTag() const { return tag_; }
        std::vector<Ptr> takeChained() { return std::move(chained_); }

        void update(float dt) {
            if (isDone()) return;
            elapsed_ += dt;
            if (elapsed_ < delay_) return;
            if (!started_) begin();

            const float local = elapsed_ - delay_;
            const bool infinite = repeat_ < 0;
            const bool ends = duration_ <= 0.0f ||
                (!infinite && local >= duration_ * static_cast<float>(repeat_ + 1));

            if (ends) {
                const bool reversed = yoyo_ && duration_ > 0.0f && (repeat_ % 2 == 1);
                apply(reversed ? 0.0f : 1.0f);
                finished_ = true;
                if (onComplete_) onComplete_();
                return;
            }

            const int cycle = static_cast<int>(local / duration_);
            float p = (local - static_cast<float>(cycle) * duration_) / duration_;
            if (yoyo_ && (cycle % 2 == 1)) p = 1.0f - p;
            apply(p);
        }
    };

    // Owns running tweens. Call update(dt) once per frame; Screen already
    // owns one that Game updates for you.
    class TweenManager {
        std::vector<Tween::Ptr> tweens_;

    public:
        Tween::Ptr start(Tween::Ptr t) {
            if (t) tweens_.push_back(t);
            return t;
        }

        // Stops every tween with this tag (a float* target by default).
        void cancel(const void* tag) {
            for (auto& t : tweens_) if (t->getTag() == tag) t->cancel();
        }

        void cancelAll() {
            for (auto& t : tweens_) t->cancel();
        }

        bool isActive(const void* tag) const {
            for (const auto& t : tweens_) if (!t->isDone() && t->getTag() == tag) return true;
            return false;
        }

        size_t size() const { return tweens_.size(); }

        void update(float dt) {
            // Snapshot: callbacks may start or cancel tweens while we iterate.
            const auto snapshot = tweens_;
            for (auto& t : snapshot) {
                t->update(dt);
                if (t->isFinished() && !t->isCancelled()) {
                    for (auto& next : t->takeChained()) start(next);
                }
            }
            tweens_.erase(std::remove_if(tweens_.begin(), tweens_.end(),
                                         [](const Tween::Ptr& t) { return t->isDone(); }),
                          tweens_.end());
        }
    };
}
