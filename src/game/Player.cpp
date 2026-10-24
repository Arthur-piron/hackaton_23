#include "game/Player.hpp"

#include "game/Scale.hpp"
#include "game/World.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kWalkSpeed = 5.0f;
constexpr float kSprintSpeed = 8.0f;
// Plus la valeur est haute, plus le joueur atteint vite sa vitesse cible.
constexpr float kAcceleration = 14.0f;
constexpr float kTurnSharpness = 20.0f;
constexpr float kRecoilDamping = 10.0f;
// Vitesse de déplacement pour laquelle les animations sont jouées à vitesse 1.
constexpr float kWalkAnimReference = 3.0f;
constexpr float kSprintAnimReference = 6.0f;
constexpr float kMovingThreshold = 0.3f;

constexpr float kStaminaDrain = 0.28f;   // par seconde de sprint
constexpr float kStaminaRegen = 0.22f;
constexpr float kStaminaRecovered = 0.35f; // seuil pour ressprinter après épuisement

constexpr float kInvulnerability = 0.45f;
constexpr float kHurtFlashDecay = 3.0f;
constexpr float kShootPoseDuration = 0.18f;
constexpr float kDeathLinger = 0.8f; // reste au sol avant l'écran de fin

// Arme : main droite au bout du nœud arm-right, modèle réduit, canon vers l'avant.
constexpr Vector3 kHandLocal = {-0.26f, 0.0f, 0.0f};
constexpr float kWeaponScale = 0.5f * kKenneyScale;
constexpr float kMuzzleDistance = 0.45f;
constexpr Color kHurtTint = {255, 120, 120, 255};

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
    , clipDie_(model.findClip("die"))
    , clipHold_(model.findClip("holding-right"))
    , clipHoldShoot_(model.findClip("holding-right-shoot"))
    , armRight_(model.findNode("arm-right"))
    , armsMask_(model.nodeMask({"arm-right"}))
{
}

void Player::reset(Vector3 spawn)
{
    position_ = spawn;
    velocity_ = {0.0f, 0.0f, 0.0f};
    recoil_ = {0.0f, 0.0f, 0.0f};
    yaw_ = 0.0f;
    sprinting_ = false;
    stamina_ = 1.0f;
    exhausted_ = false;
    health_ = maxHealth_;
    invulnerable_ = 0.0f;
    hurtFlash_ = 0.0f;
    shootPose_ = 0.0f;
    dead_ = false;
    anim_ = AnimationState{};
    model_.play(anim_, clipIdle_);
}

Vector3 Player::forward() const
{
    return {std::sin(yaw_), 0.0f, std::cos(yaw_)};
}

Vector3 Player::muzzle() const
{
    return Vector3Add(hand_, Vector3Scale(forward(), kMuzzleDistance));
}

bool Player::deathFinished() const
{
    return dead_ && anim_.time >= model_.clipDuration(clipDie_) + kDeathLinger;
}

Matrix Player::bodyTransform() const
{
    return MatrixMultiply(MatrixMultiply(MatrixScale(kKenneyScale, kKenneyScale, kKenneyScale), MatrixRotateY(yaw_)),
                          MatrixTranslate(position_.x, position_.y, position_.z));
}

void Player::update(float dt, const World& world, Vector3 aimPoint, Vector3 move, bool sprint)
{
    invulnerable_ = std::max(0.0f, invulnerable_ - dt);
    hurtFlash_ = std::max(0.0f, hurtFlash_ - dt * kHurtFlashDecay);
    shootPose_ = std::max(0.0f, shootPose_ - dt);

    if (dead_) {
        model_.update(anim_, dt);
        return;
    }

    // Sprint : consomme l'endurance ; une fois épuisé, il faut récupérer un peu avant de relancer.
    const bool moving = Vector3Length(move) > 0.0f;
    sprinting_ = sprint && moving && !exhausted_;
    if (sprinting_) {
        stamina_ = std::max(0.0f, stamina_ - kStaminaDrain * dt);
        if (stamina_ <= 0.0f) exhausted_ = true;
    } else {
        stamina_ = std::min(1.0f, stamina_ + kStaminaRegen * dt);
        if (exhausted_ && stamina_ >= kStaminaRecovered) exhausted_ = false;
    }

    const Vector3 target = Vector3Scale(move, sprinting_ ? kSprintSpeed : kWalkSpeed);
    velocity_ = Vector3Lerp(velocity_, target, 1.0f - std::exp(-kAcceleration * dt));
    recoil_ = Vector3Scale(recoil_, std::exp(-kRecoilDamping * dt));
    const Vector3 step = Vector3Scale(Vector3Add(velocity_, recoil_), dt);
    position_ = world.resolveCircle(Vector3Add(position_, step), kRadius);

    // Les personnages Kenney regardent vers +Z quand yaw = 0.
    const Vector3 toAim = Vector3Subtract(aimPoint, position_);
    if (toAim.x * toAim.x + toAim.z * toAim.z > 0.01f) {
        yaw_ += angleDelta(yaw_, std::atan2(toAim.x, toAim.z)) * (1.0f - std::exp(-kTurnSharpness * dt));
    }

    updateAnimation(dt, Vector3Length(velocity_));
    const Matrix arm = model_.nodeTransform(anim_, armRight_, bodyTransform());
    hand_ = Vector3Transform(kHandLocal, arm);
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
    // Le bras droit tient l'arme quelle que soit l'animation des jambes.
    anim_.overlayClip = shootPose_ > 0.0f ? clipHoldShoot_ : clipHold_;
    anim_.overlayMask = armsMask_;
    model_.update(anim_, dt);
}

bool Player::takeDamage(float amount, Vector3 from, EventQueue& events)
{
    if (dead_ || invulnerable_ > 0.0f) return false;
    health_ = std::max(0.0f, health_ - amount);
    invulnerable_ = kInvulnerability;
    hurtFlash_ = 1.0f;
    const Vector3 away = Vector3Normalize({position_.x - from.x, 0.0f, position_.z - from.z});
    recoil_ = Vector3Add(recoil_, Vector3Scale(away, 6.0f));
    events.push_back({EventType::PlayerHurt, position_, amount, away});
    if (health_ > 0.0f) return false;

    dead_ = true;
    anim_.overlayClip = -1;
    anim_.overlayMask = 0;
    model_.play(anim_, clipDie_, false, 1.0f, true);
    events.push_back({EventType::PlayerDied, position_});
    return true;
}

void Player::heal(float amount)
{
    if (!dead_) health_ = std::min(maxHealth_, health_ + amount);
}

void Player::applyRecoil(Vector3 impulse)
{
    recoil_ = Vector3Add(recoil_, impulse);
}

void Player::notifyShot()
{
    shootPose_ = kShootPoseDuration;
    anim_.overlayTime = 0.0f;
}

void Player::draw(const Model* weapon) const
{
    model_.draw(anim_, bodyTransform(), hurtFlash_ > 0.0f ? kHurtTint : WHITE);
    if (weapon && !dead_) {
        const Matrix transform = MatrixMultiply(
            MatrixMultiply(MatrixScale(kWeaponScale, kWeaponScale, kWeaponScale), MatrixRotateY(yaw_)),
            MatrixTranslate(hand_.x, hand_.y, hand_.z));
        for (int m = 0; m < weapon->meshCount; ++m) {
            DrawMesh(weapon->meshes[m], weapon->materials[weapon->meshMaterial[m]], transform);
        }
    }
}
