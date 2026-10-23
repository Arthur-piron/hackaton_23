#include "game/Player.hpp"

#include "game/World.hpp"
#include "raymath.h"

namespace {
constexpr float kMoveSpeed = 6.0f;
// Plus la valeur est haute, plus le joueur atteint vite sa vitesse cible.
constexpr float kAcceleration = 14.0f;
constexpr float kHeight = 1.6f;
constexpr Color kColor = {220, 190, 120, 255};

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
}

void Player::reset(Vector3 spawn)
{
    position_ = spawn;
    velocity_ = {0.0f, 0.0f, 0.0f};
}

void Player::update(float dt, const World& world)
{
    const Vector3 target = Vector3Scale(readMoveInput(), kMoveSpeed);
    const float blend = 1.0f - expf(-kAcceleration * dt);
    velocity_ = Vector3Lerp(velocity_, target, blend);
    position_ = world.clampToBounds(Vector3Add(position_, Vector3Scale(velocity_, dt)), kRadius);
}

void Player::draw() const
{
    const Vector3 center = {position_.x, kHeight * 0.5f, position_.z};
    DrawCube(center, kRadius * 2.0f, kHeight, kRadius * 2.0f, kColor);
    DrawCubeWires(center, kRadius * 2.0f, kHeight, kRadius * 2.0f, BLACK);
}
