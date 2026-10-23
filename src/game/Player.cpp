#include "game/Player.hpp"

#include "game/Scale.hpp"
#include "game/World.hpp"
#include "raymath.h"

#include <cmath>

namespace {
constexpr float kWalkSpeed = 5.0f;
constexpr float kSprintSpeed = 8.0f;
// Plus la valeur est haute, plus le joueur atteint vite sa vitesse cible.
constexpr float kAcceleration = 14.0f;
constexpr float kTurnSharpness = 20.0f;
constexpr float kModelScale = kKenneyScale;
// Vitesse de déplacement pour laquelle les animations sont jouées à vitesse 1.
constexpr float kWalkAnimReference = 3.0f;
constexpr float kSprintAnimReference = 6.0f;
constexpr float kMovingThreshold = 0.3f;

// raylib nomme les touches d'après leur position sur un clavier QWERTY :
// KEY_W/A/S/D correspondent physiquement à Z/Q/S/D sur un clavier AZERTY.
Vector3 readMoveInput()
{
    Vector3 dir = {0.0f, 0.0f, 0.0f};
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) dir.z -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) dir.z += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) dir.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dir.x += 1.0f;
    return Vector3Length(dir) > 0.0f ? Vector3Normalize(dir) : dir;
}

// Plus court chemin angulaire de a vers b, dans [-PI, PI].
float angleDelta(float a, float b)
{
    return std::remainder(b - a, 2.0f * PI);
}
}

Player::Player(const AnimatedModel& model)
    : model_(model)
    , clipIdle_(model.findClip("idle"))
    , clipWalk_(model.findClip("walk"))
    , clipSprint_(model.findClip("sprint"))
{
}

void Player::reset(Vector3 spawn)
{
    position_ = spawn;
    velocity_ = {0.0f, 0.0f, 0.0f};
    yaw_ = 0.0f;
    sprinting_ = false;
    anim_ = AnimationState{};
    model_.play(anim_, clipIdle_);
}

Vector3 Player::forward() const
{
    return {std::sin(yaw_), 0.0f, std::cos(yaw_)};
}

void Player::update(float dt, const World& world, Vector3 aimPoint)
{
    const Vector3 input = readMoveInput();
    sprinting_ = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && Vector3Length(input) > 0.0f;
    const Vector3 target = Vector3Scale(input, sprinting_ ? kSprintSpeed : kWalkSpeed);
    velocity_ = Vector3Lerp(velocity_, target, 1.0f - std::exp(-kAcceleration * dt));
    position_ = world.clampToBounds(Vector3Add(position_, Vector3Scale(velocity_, dt)), kRadius);

    // Les personnages Kenney regardent vers +Z quand yaw = 0.
    const Vector3 toAim = Vector3Subtract(aimPoint, position_);
    if (toAim.x * toAim.x + toAim.z * toAim.z > 0.01f) {
        const float desired = std::atan2(toAim.x, toAim.z);
        yaw_ += angleDelta(yaw_, desired) * (1.0f - std::exp(-kTurnSharpness * dt));
    }

    updateAnimation(dt, Vector3Length(velocity_));
}

void Player::updateAnimation(float dt, float speed)
{
    if (speed < kMovingThreshold) {
        model_.play(anim_, clipIdle_);
    } else if (sprinting_) {
        model_.play(anim_, clipSprint_, true, speed / kSprintAnimReference);
    } else {
        model_.play(anim_, clipWalk_, true, speed / kWalkAnimReference);
    }
    model_.update(anim_, dt);
}

void Player::draw() const
{
    const Matrix transform = MatrixMultiply(
        MatrixMultiply(MatrixScale(kModelScale, kModelScale, kModelScale), MatrixRotateY(yaw_)),
        MatrixTranslate(position_.x, position_.y, position_.z));
    model_.draw(anim_, transform);
}
