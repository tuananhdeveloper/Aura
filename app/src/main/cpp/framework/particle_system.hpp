#pragma once

#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

namespace Framework {
    enum class ParticleShape { Circle, Square };

    // Describes how an emitter spawns and animates particles. Ranges are
    // {min, max} and sampled per particle. Angles are in degrees in screen
    // space: 0 = right, 90 = down, -90 = up.
    struct ParticleConfig {
        float emissionRate = 0.0f;           // particles/second while emitting
        float duration = 0.0f;               // seconds to emit before stopping (0 = until stop())
        int maxParticles = 500;

        Vector2 lifetime = { 0.5f, 1.0f };   // seconds
        Vector2 speed = { 50.0f, 100.0f };   // px/s
        float angle = -90.0f;                // emission direction
        float spread = 360.0f;               // total cone width
        float spawnRadius = 0.0f;            // random offset from the emitter origin
        Vector2 spawnRect = { 0.0f, 0.0f };  // if non-zero: spawn inside a centered w x h box instead

        Vector2 gravity = { 0.0f, 0.0f };    // px/s^2
        float drag = 0.0f;                   // velocity decay per second

        Vector2 startSize = { 4.0f, 6.0f };  // radius / half-extent in px
        Vector2 endSize = { 0.0f, 0.0f };
        Color startColor = WHITE;            // alpha fades with the colors
        Color endColor = { 255, 255, 255, 0 };

        Vector2 rotation = { 0.0f, 0.0f };   // initial degrees (squares only)
        Vector2 angularVelocity = { 0.0f, 0.0f };  // degrees/s

        ParticleShape shape = ParticleShape::Circle;
        bool additive = false;               // glow for fire/magic
    };

    // Ready-made effects; tweak any field on the returned config.
    namespace ParticlePresets {
        inline ParticleConfig fire() {
            ParticleConfig c;
            c.emissionRate = 60;
            c.lifetime = { 0.4f, 0.9f };
            c.speed = { 30.0f, 70.0f };
            c.angle = -90; c.spread = 30;
            c.spawnRadius = 6;
            c.gravity = { 0, -40 };
            c.startSize = { 6, 10 }; c.endSize = { 0, 2 };
            c.startColor = { 255, 200, 60, 255 };
            c.endColor = { 220, 40, 0, 0 };
            c.additive = true;
            return c;
        }

        inline ParticleConfig smoke() {
            ParticleConfig c;
            c.emissionRate = 20;
            c.lifetime = { 1.2f, 2.2f };
            c.speed = { 15.0f, 35.0f };
            c.angle = -90; c.spread = 40;
            c.spawnRadius = 5;
            c.gravity = { 8, -20 };
            c.drag = 0.6f;
            c.startSize = { 6, 9 }; c.endSize = { 18, 28 };
            c.startColor = { 120, 120, 130, 140 };
            c.endColor = { 70, 70, 80, 0 };
            return c;
        }

        // One-shot: use ParticleSystem::burst().
        inline ParticleConfig explosion() {
            ParticleConfig c;
            c.lifetime = { 0.4f, 1.0f };
            c.speed = { 80.0f, 300.0f };
            c.spread = 360;
            c.drag = 2.5f;
            c.startSize = { 4, 9 }; c.endSize = { 0, 1 };
            c.startColor = { 255, 230, 120, 255 };
            c.endColor = { 255, 60, 0, 0 };
            c.additive = true;
            return c;
        }

        // One-shot: impact sparks; aim with `angle` and a narrow `spread`.
        inline ParticleConfig sparks() {
            ParticleConfig c;
            c.lifetime = { 0.25f, 0.6f };
            c.speed = { 120.0f, 320.0f };
            c.spread = 360;
            c.gravity = { 0, 500 };
            c.drag = 1.0f;
            c.startSize = { 2, 3 }; c.endSize = { 0, 0 };
            c.startColor = { 255, 240, 160, 255 };
            c.endColor = { 255, 140, 20, 0 };
            c.shape = ParticleShape::Square;
            c.rotation = { 0, 360 };
            c.angularVelocity = { -360, 360 };
            c.additive = true;
            return c;
        }

        inline ParticleConfig magic() {
            ParticleConfig c;
            c.emissionRate = 40;
            c.lifetime = { 0.8f, 1.6f };
            c.speed = { 10.0f, 40.0f };
            c.spread = 360;
            c.spawnRadius = 14;
            c.gravity = { 0, -25 };
            c.drag = 0.5f;
            c.startSize = { 2, 5 }; c.endSize = { 0, 1 };
            c.startColor = { 160, 120, 255, 255 };
            c.endColor = { 80, 220, 255, 0 };
            c.angularVelocity = { -180, 180 };
            c.additive = true;
            return c;
        }
    }

    namespace ParticlePresets {
        // Slow drifting specks for atmosphere; set spawnRect to the area to cover.
        inline ParticleConfig ambient() {
            ParticleConfig c;
            c.emissionRate = 6;
            c.maxParticles = 60;
            c.lifetime = { 3.0f, 6.0f };
            c.speed = { 4.0f, 14.0f };
            c.angle = -90; c.spread = 120;
            c.startSize = { 1.5f, 3.0f }; c.endSize = { 0.5f, 1.0f };
            c.startColor = { 140, 150, 255, 90 };
            c.endColor = { 140, 150, 255, 0 };
            c.additive = true;
            return c;
        }

        // Short-lived puffs left behind a moving object.
        inline ParticleConfig trail() {
            ParticleConfig c;
            c.emissionRate = 30;
            c.lifetime = { 0.3f, 0.6f };
            c.speed = { 5.0f, 20.0f };
            c.spread = 360;
            c.spawnRadius = 4;
            c.drag = 2.0f;
            c.startSize = { 3, 5 }; c.endSize = { 0, 1 };
            c.startColor = { 120, 255, 120, 160 };
            c.endColor = { 30, 160, 60, 0 };
            c.additive = true;
            return c;
        }

        // One-shot, tinted by the caller; shoots up then falls.
        inline ParticleConfig confetti(Color color) {
            ParticleConfig c;
            c.lifetime = { 0.9f, 1.6f };
            c.speed = { 150.0f, 380.0f };
            c.angle = -90; c.spread = 100;
            c.gravity = { 0, 520 };
            c.drag = 1.2f;
            c.startSize = { 3, 5 }; c.endSize = { 2, 3 };
            c.startColor = color;
            c.endColor = { color.r, color.g, color.b, 0 };
            c.shape = ParticleShape::Square;
            c.rotation = { 0, 360 };
            c.angularVelocity = { -540, 540 };
            return c;
        }
    }

    class ParticleEmitter {
        struct Particle {
            Vector2 pos, vel;
            float age, life;
            float size0, size1;
            float rot, angVel;
        };

        static float lerp(float a, float b, float t) { return a + (b - a) * t; }

        static uint32_t nextSeed() {
            static uint32_t seed = 0x9E3779B9u;
            seed = seed * 1664525u + 1013904223u;
            return seed | 1u;
        }

        uint32_t rng_ = nextSeed();

        float rand01() {
            rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
            return static_cast<float>(rng_ & 0xFFFFFF) / static_cast<float>(0x1000000);
        }
        float range(Vector2 r) { return lerp(r.x, r.y, rand01()); }

        static Color mix(Color a, Color b, float t) {
            auto ch = [t](unsigned char x, unsigned char y) {
                return static_cast<unsigned char>(std::clamp(lerp(x, y, t), 0.0f, 255.0f));
            };
            return { ch(a.r, b.r), ch(a.g, b.g), ch(a.b, b.b), ch(a.a, b.a) };
        }

        ParticleConfig config_;
        Vector2 position_;
        std::vector<Particle> particles_;
        bool emitting_ = false;
        bool autoRemove_ = true;
        float emitTime_ = 0.0f;
        float emitAccum_ = 0.0f;

        void spawn() {
            if (static_cast<int>(particles_.size()) >= config_.maxParticles) return;
            const float dir = (config_.angle + (rand01() - 0.5f) * config_.spread) * DEG2RAD;
            const float speed = range(config_.speed);
            const float ang = rand01() * 2.0f * PI;
            const float rad = config_.spawnRadius * std::sqrt(rand01());

            Particle p;
            if (config_.spawnRect.x > 0.0f || config_.spawnRect.y > 0.0f) {
                p.pos = { position_.x + (rand01() - 0.5f) * config_.spawnRect.x,
                          position_.y + (rand01() - 0.5f) * config_.spawnRect.y };
            } else {
                p.pos = { position_.x + std::cos(ang) * rad, position_.y + std::sin(ang) * rad };
            }
            p.vel = { std::cos(dir) * speed, std::sin(dir) * speed };
            p.age = 0.0f;
            p.life = std::max(0.01f, range(config_.lifetime));
            p.size0 = range(config_.startSize);
            p.size1 = range(config_.endSize);
            p.rot = range(config_.rotation);
            p.angVel = range(config_.angularVelocity);
            particles_.push_back(p);
        }

    public:
        explicit ParticleEmitter(const ParticleConfig& config, Vector2 position = { 0, 0 },
                                 bool startEmitting = true)
            : config_(config), position_(position), emitting_(startEmitting) {}

        ParticleConfig& config() { return config_; }
        void setPosition(Vector2 p) { position_ = p; }
        Vector2 getPosition() const { return position_; }

        // When false the system keeps this emitter after it finishes so start()
        // can reuse it. Defaults to true (removed once finished).
        void setAutoRemove(bool v) { autoRemove_ = v; }
        bool isAutoRemove() const { return autoRemove_; }

        // Resumes continuous emission (restarts the duration timer).
        void start() { emitting_ = true; emitTime_ = 0.0f; }
        // Stops spawning; live particles finish their life.
        void stop() { emitting_ = false; }
        bool isEmitting() const { return emitting_; }

        void burst(int count) { for (int i = 0; i < count; ++i) spawn(); }

        size_t particleCount() const { return particles_.size(); }
        // True when nothing is emitting and every particle has died.
        bool isFinished() const { return !emitting_ && particles_.empty(); }

        void update(float dt) {
            if (emitting_ && config_.emissionRate > 0.0f) {
                emitAccum_ += config_.emissionRate * dt;
                while (emitAccum_ >= 1.0f) { spawn(); emitAccum_ -= 1.0f; }
            }
            if (emitting_ && config_.duration > 0.0f) {
                emitTime_ += dt;
                if (emitTime_ >= config_.duration) emitting_ = false;
            }

            const float damp = std::exp(-config_.drag * dt);
            for (auto& p : particles_) {
                p.age += dt;
                p.vel.x = (p.vel.x + config_.gravity.x * dt) * damp;
                p.vel.y = (p.vel.y + config_.gravity.y * dt) * damp;
                p.pos.x += p.vel.x * dt;
                p.pos.y += p.vel.y * dt;
                p.rot += p.angVel * dt;
            }
            particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                                            [](const Particle& p) { return p.age >= p.life; }),
                             particles_.end());
        }

        void draw() const {
            if (particles_.empty()) return;
            if (config_.additive) BeginBlendMode(BLEND_ADDITIVE);
            for (const auto& p : particles_) {
                const float t = p.age / p.life;
                const float size = lerp(p.size0, p.size1, t);
                if (size <= 0.0f) continue;
                const Color c = mix(config_.startColor, config_.endColor, t);
                if (config_.shape == ParticleShape::Circle) {
                    DrawCircleV(p.pos, size, c);
                } else {
                    const Rectangle r = { p.pos.x, p.pos.y, size * 2.0f, size * 2.0f };
                    DrawRectanglePro(r, { size, size }, p.rot, c);
                }
            }
            if (config_.additive) EndBlendMode();
        }
    };

    // Owns emitters. Screen has one that Game updates; draw it inside the
    // viewport so particles use world coordinates.
    class ParticleSystem {
        std::vector<std::shared_ptr<ParticleEmitter>> emitters_;

    public:
        // Fire-and-forget burst of `count` particles; removed once they die.
        void burst(const ParticleConfig& config, Vector2 position, int count) {
            auto e = std::make_shared<ParticleEmitter>(config, position, false);
            e->burst(count);
            emitters_.push_back(std::move(e));
        }

        // Continuous emitter. Move it with setPosition(); call stop() when done
        // and it is removed after its particles fade out.
        std::shared_ptr<ParticleEmitter> emit(const ParticleConfig& config, Vector2 position) {
            auto e = std::make_shared<ParticleEmitter>(config, position, true);
            emitters_.push_back(e);
            return e;
        }

        void clear() { emitters_.clear(); }
        size_t emitterCount() const { return emitters_.size(); }

        void update(float dt) {
            for (auto& e : emitters_) e->update(dt);
            emitters_.erase(std::remove_if(emitters_.begin(), emitters_.end(),
                                           [](const std::shared_ptr<ParticleEmitter>& e) { return e->isAutoRemove() && e->isFinished(); }),
                            emitters_.end());
        }

        void draw() const {
            for (const auto& e : emitters_) e->draw();
        }
    };
}
