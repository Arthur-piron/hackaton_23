#include "game/Enemy.hpp"

#include "game/Scale.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kTurnSharpness = 8.0f;
constexpr float kKnockbackDamping = 7.0f;
constexpr float kFlashDuration = 0.12f;
// Moment de l'animation d'attaque où le coup porte (fraction de sa durée).
constexpr float kAttackImpactFraction = 0.45f;
constexpr float kAttackReachBonus = 0.5f;
constexpr float kCorpseDuration = 3.0f;
constexpr float kSinkDuration = 1.2f;
constexpr float kSinkDepth = 1.6f;
constexpr float kBodyRadius = 0.42f;
constexpr float kChestHeight = 0.95f;
// Yeux, dans le repère du nœud « head » (le visage regarde vers +Z).
constexpr Vector3 kZombieEye = {0.085f, 0.2f, 0.19f};
constexpr Vector3 kSkeletonEye = {0.06f, 0.13f, 0.17f};
constexpr float kEyeFadeDuration = 0.4f;

const EnemyStats kStats[] = {
    // nom, modèle, vie, vitesse, dégâts, portée, recharge, échelle, teinte, anim, vitesse anim, score, résistance
    {"Marcheur", "character-zombie.glb", 60.0f, 1.7f, 12.0f, 1.25f, 1.2f, 1.0f, {255, 255, 255, 255}, "walk", 1.6f, 10, 0.0f},
    {"Coureur", "character-zombie.glb", 35.0f, 5.2f, 8.0f, 1.2f, 0.9f, 0.92f, {255, 150, 140, 255}, "sprint", 4.5f, 15, 0.0f},
    {"Squelette", "character-skeleton.glb", 45.0f, 2.8f, 10.0f, 1.25f, 1.0f, 1.0f, {255, 255, 255, 255}, "walk", 2.2f, 15, 0.2f},
    {"Brute", "character-zombie.glb", 420.0f, 1.35f, 30.0f, 1.8f, 1.8f, 1.6f, {150, 190, 140, 255}, "walk", 1.2f, 60, 0.8f},
};

float randomRange(float min, float max)
{
    return min + (max - min) * static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f;
}

float angleDelta(float a, float b)
{
    return std::remainder(b - a, 2.0f * PI);
}
}

const EnemyStats& enemyStats(EnemyType type)
{
    return kStats[static_cast<int>(type)];
}

Enemy::Enemy(unsigned id, EnemyType type, const AnimatedModel& model, Vector3 position)
    : id_(id)
    , type_(type)
    , model_(&model)
    , position_(position)
    , health_(enemyStats(type).health)
    , groanTimer_(randomRange(1.0f, 8.0f))
    , speedJitter_(randomRange(0.85f, 1.15f))
    , clipMove_(model.findClip(enemyStats(type).moveClip))
    , clipAttack_(model.findClip("attack-melee-right"))
    , clipDie_(model.findClip("die"))
    , headNode_(model.findNode("head"))
{
    model_->play(anim_, clipMove_, true, speedJitter_);
    anim_.time = randomRange(0.0f, 1.0f); // désynchronise les démarches
}

Vector3 Enemy::hitCenter() const
{
    const float s = stats().scale;
    return {position_.x, kChestHeight * s, position_.z};
}

float Enemy::radius() const
{
    return kBodyRadius * stats().scale;
}

float Enemy::update(float dt, Vector3 target, EventQueue& events)
{
    const EnemyStats& s = stats();
    flash_ = std::max(0.0f, flash_ - dt);
    model_->update(anim_, dt);

    if (state_ == State::Dying) {
        deathTimer_ += dt;
        if (deathTimer_ > kCorpseDuration) {
            position_.y = -kSinkDepth * std::min(1.0f, (deathTimer_ - kCorpseDuration) / kSinkDuration);
            if (deathTimer_ > kCorpseDuration + kSinkDuration) removable_ = true;
        }
        return 0.0f;
    }

    // Recul dû aux balles, amorti dans le temps.
    position_ = Vector3Add(position_, Vector3Scale(knockback_, dt));
    knockback_ = Vector3Scale(knockback_, std::exp(-kKnockbackDamping * dt));

    const Vector3 toTarget = {target.x - position_.x, 0.0f, target.z - position_.z};
    const float distance = Vector3Length(toTarget);
    if (distance > 0.001f) {
        yaw_ += angleDelta(yaw_, std::atan2(toTarget.x, toTarget.z)) * (1.0f - std::exp(-kTurnSharpness * dt));
    }

    groanTimer_ -= dt;
    if (groanTimer_ <= 0.0f) {
        groanTimer_ = randomRange(4.0f, 10.0f);
        events.push_back({EventType::EnemyGroan, position_, static_cast<float>(type_)});
    }

    attackCooldown_ = std::max(0.0f, attackCooldown_ - dt);
    float damage = 0.0f;
    const float reach = s.attackRange * s.scale;

    if (state_ == State::Attacking) {
        const float duration = model_->clipDuration(clipAttack_);
        if (!attackDealt_ && anim_.time >= duration * kAttackImpactFraction) {
            attackDealt_ = true;
            if (distance <= reach + kAttackReachBonus) {
                damage = s.damage;
                events.push_back({EventType::EnemyAttack, position_, s.damage});
            }
        }
        if (model_->isFinished(anim_)) {
            state_ = State::Chasing;
            model_->play(anim_, clipMove_, true, speedJitter_);
        }
        return damage;
    }

    if (distance <= reach && attackCooldown_ <= 0.0f) {
        state_ = State::Attacking;
        attackDealt_ = false;
        attackCooldown_ = s.attackCooldown;
        model_->play(anim_, clipAttack_, false, 1.0f, true);
        return 0.0f;
    }

    if (distance > reach * 0.8f) {
        const float speed = s.speed * speedJitter_;
        position_ = Vector3Add(position_, Vector3Scale(Vector3Normalize(toTarget), speed * dt));
        model_->play(anim_, clipMove_, true, speed / s.animReferenceSpeed);
    }
    return 0.0f;
}

bool Enemy::hit(float damage, Vector3 direction, float knockback, EventQueue& events)
{
    if (state_ == State::Dying) return false;
    const EnemyStats& s = stats();
    health_ -= damage;
    flash_ = kFlashDuration;
    const Vector3 flat = Vector3Normalize({direction.x, 0.0f, direction.z});
    knockback_ = Vector3Add(knockback_, Vector3Scale(flat, knockback * (1.0f - s.knockbackResistance)));
    events.push_back({EventType::EnemyHit, hitCenter(), damage, flat});

    if (health_ > 0.0f) return false;
    state_ = State::Dying;
    deathTimer_ = 0.0f;
    model_->play(anim_, clipDie_, false, 1.0f, true);
    events.push_back({EventType::EnemyKilled, hitCenter(), static_cast<float>(s.score), flat});
    return true;
}

void Enemy::push(Vector3 offset)
{
    if (state_ == State::Dying) return;
    position_ = Vector3Add(position_, offset);
}

Matrix Enemy::bodyTransform() const
{
    const float scale = kKenneyScale * stats().scale;
    return MatrixMultiply(MatrixMultiply(MatrixScale(scale, scale, scale), MatrixRotateY(yaw_)),
                          MatrixTranslate(position_.x, position_.y, position_.z));
}

void Enemy::eyePositions(Vector3& left, Vector3& right) const
{
    const Matrix head = model_->nodeTransform(anim_, headNode_, bodyTransform());
    const Vector3 eye = type_ == EnemyType::Skeleton ? kSkeletonEye : kZombieEye;
    left = Vector3Transform(eye, head);
    right = Vector3Transform({-eye.x, eye.y, eye.z}, head);
}

float Enemy::eyeGlow() const
{
    if (state_ != State::Dying) return 1.0f;
    return std::max(0.0f, 1.0f - deathTimer_ / kEyeFadeDuration);
}

void Enemy::draw() const
{
    Color tint = stats().tint;
    if (flash_ > 0.0f) {
        // Éclat vers le blanc chaud quand l'ennemi est touché.
        const float k = flash_ / kFlashDuration;
        tint.r = static_cast<unsigned char>(tint.r + (255 - tint.r) * k);
        tint.g = static_cast<unsigned char>(tint.g + (230 - tint.g) * k);
        tint.b = static_cast<unsigned char>(tint.b + (200 - tint.b) * k);
    }
    model_->draw(anim_, bodyTransform(), tint);
}
