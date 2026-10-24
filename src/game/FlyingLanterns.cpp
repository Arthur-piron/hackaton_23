#include "game/FlyingLanterns.hpp"

#include "game/Flashlight.hpp"
#include "game/Scale.hpp"
#include "raymath.h"

#include <cmath>

namespace {
constexpr float kOrbitRadius = 2.6f;
constexpr float kOrbitSpeed = 2.2f;      // radians par seconde
constexpr float kHeight = 1.9f;
constexpr float kBobAmplitude = 0.15f;
constexpr float kBobSpeed = 3.0f;
constexpr float kBurnRadius = 1.6f;
constexpr float kBurnPerSecond = 10.0f;
constexpr float kTickInterval = 0.25f;
constexpr float kModelScale = 0.6f;      // fraction de l'échelle Kenney
constexpr float kFlameHeight = 0.35f;    // hauteur de la flamme dans le modèle, en unités Kenney
constexpr float kSpinSpeed = 40.0f;      // degrés par seconde, la lanterne tourne sur elle-même

constexpr Color kLightColor = {255, 190, 110, 255};
constexpr float kLightRange = 6.0f;
constexpr float kLightIntensity = 1.6f;
constexpr float kLightFlicker = 0.35f;
constexpr float kLightHalo = 0.9f;
}

void FlyingLanterns::reset()
{
    angle_ = 0.0f;
    time_ = 0.0f;
    tickTimer_ = 0.0f;
    positions_.clear();
}

void FlyingLanterns::setCount(int count)
{
    count_ = count;
}

void FlyingLanterns::update(float dt, Vector3 center)
{
    angle_ += kOrbitSpeed * dt;
    time_ += dt;
    positions_.clear();
    for (int i = 0; i < count_; ++i) {
        // Lanternes réparties à égale distance sur l'orbite.
        const float a = angle_ + 2.0f * PI * static_cast<float>(i) / static_cast<float>(count_);
        const float bob = kBobAmplitude * std::sin(time_ * kBobSpeed + static_cast<float>(i));
        positions_.push_back({center.x + std::cos(a) * kOrbitRadius, kHeight + bob,
                              center.z + std::sin(a) * kOrbitRadius});
    }
}

std::vector<FlyingLanterns::Burn> FlyingLanterns::burn(float dt, std::vector<Enemy>& enemies)
{
    std::vector<Burn> burns;
    if (positions_.empty()) return burns;
    tickTimer_ -= dt;
    if (tickTimer_ > 0.0f) return burns;
    tickTimer_ = kTickInterval;
    for (Enemy& e : enemies) {
        const float sensitivity = Flashlight::lightSensitivity(e.type());
        if (!e.isAlive() || sensitivity <= 0.0f) continue;
        for (const Vector3& p : positions_) {
            const Vector2 offset = {e.position().x - p.x, e.position().z - p.z};
            if (Vector2Length(offset) > kBurnRadius + e.radius()) continue;
            burns.push_back({&e, kBurnPerSecond * kTickInterval * sensitivity});
            break; // une seule morsure par mort, même sous deux lanternes
        }
    }
    return burns;
}

std::vector<PointLight> FlyingLanterns::lights() const
{
    std::vector<PointLight> lights;
    for (const Vector3& p : positions_) {
        lights.push_back({p, kLightColor, kLightRange, kLightIntensity, kLightFlicker, kLightHalo});
    }
    return lights;
}

void FlyingLanterns::draw(const Model& model) const
{
    const float scale = kKenneyScale * kModelScale;
    for (const Vector3& p : positions_) {
        // Origine du modèle à la base : on le descend pour mettre la flamme à la hauteur de la lumière.
        DrawModelEx(model, {p.x, p.y - kFlameHeight * scale, p.z}, {0.0f, 1.0f, 0.0f}, time_ * kSpinSpeed,
                    {scale, scale, scale}, WHITE);
    }
}
