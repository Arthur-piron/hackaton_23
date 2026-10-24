#include "game/Merchant.hpp"

#include "core/Fonts.hpp"

#include "game/Scale.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
// Teinte du marchand : même modèle que le gardien, habit violet sombre.
constexpr Color kTint = {200, 160, 255, 255};
constexpr float kLookDistance = 9.0f;
constexpr float kMaxTurn = 70.0f * DEG2RAD;
constexpr float kTurnSharpness = 4.0f;
constexpr float kIdleSpeed = 0.8f;

// Pièce qui tourne au-dessus de sa tête quand la boutique est ouverte.
constexpr float kCoinHeight = 2.55f;
constexpr float kCoinBob = 0.08f;
constexpr float kCoinRadius = 0.22f;
constexpr float kCoinHalfThickness = 0.03f;
constexpr float kCoinSpin = 2.5f;
constexpr Color kCoinColor = {255, 196, 50, 255};

constexpr float kSignHeight = 3.0f;
constexpr float kSignDistance = 14.0f;
constexpr Color kOpenColor = {255, 215, 120, 255};
constexpr Color kSignColor = {230, 200, 120, 200};
constexpr Color kClosedColor = {170, 160, 150, 200};
constexpr Color kDeniedColor = {235, 90, 80, 255};
constexpr Color kShadow = {0, 0, 0, 200};

float angleDelta(float a, float b)
{
    return std::remainder(b - a, 2.0f * PI);
}

void drawCenteredText(const char* text, Vector2 at, int size, Color color)
{
    const int x = static_cast<int>(at.x) - fonts::measureText(text, size) / 2;
    const int y = static_cast<int>(at.y);
    fonts::drawText(text, x + 2, y + 2, size, kShadow);
    fonts::drawText(text, x, y, size, color);
}
}

Merchant::Merchant(const AnimatedModel& model, const MerchantDef& def)
    : model_(&model)
    , position_(def.position)
    , restYaw_(def.yaw * DEG2RAD)
    , yaw_(restYaw_)
{
    model_->play(anim_, model_->findClip("idle"), true, kIdleSpeed);
}

float Merchant::distanceTo(Vector3 player) const
{
    return Vector2Distance({player.x, player.z}, {position_.x, position_.z});
}

void Merchant::update(float dt, Vector3 player)
{
    time_ += dt;
    model_->update(anim_, dt);
    // Il tourne la tête (et le buste) vers le joueur qui approche, sans quitter son comptoir.
    float target = restYaw_;
    if (distanceTo(player) < kLookDistance) {
        const float toPlayer = std::atan2(player.x - position_.x, player.z - position_.z);
        target = restYaw_ + std::clamp(angleDelta(restYaw_, toPlayer), -kMaxTurn, kMaxTurn);
    }
    yaw_ += angleDelta(yaw_, target) * (1.0f - std::exp(-kTurnSharpness * dt));
}

void Merchant::draw(bool open) const
{
    const Matrix transform =
        MatrixMultiply(MatrixMultiply(MatrixScale(kKenneyScale, kKenneyScale, kKenneyScale), MatrixRotateY(yaw_)),
                       MatrixTranslate(position_.x, position_.y, position_.z));
    model_->draw(anim_, transform, kTint);
    if (!open) return;
    const float spin = time_ * kCoinSpin;
    const Vector3 axis = {std::sin(spin) * kCoinHalfThickness, 0.0f, std::cos(spin) * kCoinHalfThickness};
    const Vector3 center = {position_.x, kCoinHeight + kCoinBob * std::sin(time_ * 2.0f), position_.z};
    DrawCylinderEx(Vector3Subtract(center, axis), Vector3Add(center, axis), kCoinRadius, kCoinRadius, 16, kCoinColor);
}

void Merchant::drawPrompt(const Camera3D& camera, Vector3 player, bool open) const
{
    const float distance = distanceTo(player);
    if (distance > kSignDistance) return;
    const Vector2 screen = GetWorldToScreen(Vector3Add(position_, {0.0f, kSignHeight, 0.0f}), camera);
    if (distance > kInteractDistance) {
        drawCenteredText(open ? "MARCHAND" : "MARCHAND - fermé", screen, 18, open ? kSignColor : kClosedColor);
        return;
    }
    if (open) {
        drawCenteredText("E : marchand", {screen.x, screen.y - 30.0f}, 24, kOpenColor);
        drawCenteredText("Armes, améliorations, soins", screen, 18, kSignColor);
    } else {
        drawCenteredText("Revenez entre les vagues", {screen.x, screen.y - 30.0f}, 24, kDeniedColor);
        drawCenteredText("Le marchand se cache pendant l'assaut", screen, 18, kClosedColor);
    }
}
