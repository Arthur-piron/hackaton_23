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
// Vampire : il tourne autour du joueur pour le prendre de dos, et fuit la lumière.
constexpr float kFlankCommitDistance = 3.0f;  // en deçà, il fonce sur le joueur
constexpr float kFlankBehindAngle = 1.9f;     // radians depuis le regard du joueur : « dans son dos »
constexpr float kFlankStep = 0.6f;            // radians gagnés autour du joueur à chaque visée
constexpr float kFlankTighten = 0.8f;         // le cercle se resserre à chaque visée
constexpr float kFleeSpeedBoost = 1.35f;
constexpr float kLightAversion = 1.4f;
// Spectre : flotte au-dessus du sol, semi-transparent, se dissipe à sa mort.
constexpr float kGhostHover = 0.3f;
constexpr float kGhostBob = 0.12f;
constexpr float kGhostBobSpeed = 2.2f;
constexpr unsigned char kGhostAlpha = 120;
constexpr float kGhostFadeDuration = 1.5f;
// Ralentissement (carte Gel...) : teinte bleu glacé.
constexpr Color kFrostTint = {150, 205, 255, 255};
constexpr float kFrostTintStrength = 0.6f;
// Élite : la teinte du corps tire légèrement vers la couleur du modificateur.
constexpr float kEliteTintStrength = 0.3f;

const EnemyStats kStats[] = {
    // nom, modèle, vie, vitesse, dégâts, portée, recharge, échelle, teinte, anim, vitesse anim, score, résistance,
    // craint la lumière, éthéré
    {"Marcheur", "character-zombie.glb", 60.0f, 1.7f, 12.0f, 1.25f, 1.2f, 1.0f, {255, 255, 255, 255}, "walk", 1.6f, 10, 0.0f,
     false, false},
    {"Coureur", "character-zombie.glb", 35.0f, 5.2f, 8.0f, 1.2f, 0.9f, 0.92f, {255, 150, 140, 255}, "sprint", 4.5f, 15, 0.0f,
     false, false},
    {"Squelette", "character-skeleton.glb", 45.0f, 2.8f, 10.0f, 1.25f, 1.0f, 1.0f, {255, 255, 255, 255}, "walk", 2.2f, 15,
     0.2f, false, false},
    {"Brute", "character-zombie.glb", 420.0f, 1.35f, 30.0f, 1.8f, 1.8f, 1.6f, {150, 190, 140, 255}, "walk", 1.2f, 60, 0.8f,
     false, false},
    {"Vampire", "character-vampire.glb", 80.0f, 3.4f, 16.0f, 1.25f, 1.1f, 1.0f, {255, 255, 255, 255}, "walk", 2.2f, 25,
     0.3f, true, false},
    {"Spectre", "character-ghost.glb", 70.0f, 2.3f, 14.0f, 1.3f, 1.4f, 1.05f, {190, 225, 255, kGhostAlpha}, "idle", 2.3f,
     25, 0.6f, false, true},
    {"Le Fossoyeur", "character-zombie.glb", 2200.0f, 1.3f, 40.0f, 1.4f, 2.2f, 2.2f, {125, 110, 95, 255}, "walk", 1.0f,
     500, 1.0f, false, false},
    {"Le Rongé", "character-zombie.glb", 6500.0f, 1.6f, 55.0f, 1.3f, 2.0f, 3.0f, {175, 140, 160, 255}, "walk", 1.0f,
     1500, 1.0f, false, false},
};
static_assert(sizeof(kStats) / sizeof(kStats[0]) == static_cast<size_t>(EnemyType::Count), "un profil par type d'ennemi");

float randomRange(float min, float max)
{
    return min + (max - min) * static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f;
}

float angleDelta(float a, float b)
{
    return std::remainder(b - a, 2.0f * PI);
}

Vector3 flatTo(Vector3 from, Vector3 to)
{
    return {to.x - from.x, 0.0f, to.z - from.z};
}

unsigned char lerpChannel(unsigned char from, unsigned char to, float k)
{
    return static_cast<unsigned char>(from + (to - from) * k);
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
    , maxHealth_(enemyStats(type).health)
    , groanTimer_(randomRange(1.0f, 8.0f))
    , speedJitter_(randomRange(0.85f, 1.15f))
    , clipMove_(model.findClip(enemyStats(type).moveClip))
    , clipFlee_(model.findClip("sprint"))
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
    return {position_.x, kChestHeight * s + hoverHeight(), position_.z};
}

float Enemy::hoverHeight() const
{
    if (!stats().ethereal) return 0.0f;
    const float hover = kGhostHover + std::sin(age_ * kGhostBobSpeed + static_cast<float>(id_)) * kGhostBob;
    // À sa mort, le spectre retombe en se dissipant.
    if (state_ == State::Dying) return hover * std::max(0.0f, 1.0f - deathTimer_ / kGhostFadeDuration);
    return hover;
}

void Enemy::scaleHealth(float factor)
{
    health_ *= factor;
    maxHealth_ *= factor;
}

void Enemy::makeElite(EliteKind kind)
{
    if (kind == EliteKind::None || elite_ != EliteKind::None) return;
    elite_ = kind;
    scaleHealth(elites::kHealthMultiplier);
}

float Enemy::radius() const
{
    return kBodyRadius * stats().scale;
}

Vector3 Enemy::flankPoint(const EnemySenses& senses) const
{
    const Vector3 fromPlayer = flatTo(senses.player, position_);
    const float distance = Vector3Length(fromPlayer);
    if (distance <= kFlankCommitDistance) return senses.player;
    // Angle autour du joueur : 0 devant lui, ±PI dans son dos.
    const float facing = std::atan2(senses.playerForward.x, senses.playerForward.z);
    const float bearing = std::atan2(fromPlayer.x, fromPlayer.z);
    const float offset = angleDelta(facing, bearing);
    if (std::fabs(offset) >= kFlankBehindAngle) return senses.player;
    // Il glisse vers le dos du joueur par le côté où il se trouve déjà, en se rapprochant.
    float side = offset > 0.0f ? 1.0f : -1.0f;
    if (std::fabs(offset) < 1e-3f) side = (id_ % 2) == 0 ? 1.0f : -1.0f;
    const float next = bearing + side * kFlankStep;
    const float radius = std::max(kFlankCommitDistance, distance * kFlankTighten);
    return {senses.player.x + std::sin(next) * radius, 0.0f, senses.player.z + std::cos(next) * radius};
}

float Enemy::update(float dt, const EnemySenses& senses, EventQueue& events)
{
    const EnemyStats& s = stats();
    flash_ = std::max(0.0f, flash_ - dt);
    age_ += dt;
    slowTimer_ = std::max(0.0f, slowTimer_ - dt);
    if (slowTimer_ <= 0.0f) slowFraction_ = 0.0f;
    const float eliteSpeed = elite_ == EliteKind::Swift ? elites::kSwiftSpeed : 1.0f;
    const float pace = speedJitter_ * eliteSpeed * senses.haste * (1.0f - slowFraction_);
    model_->update(anim_, dt);

    if (state_ == State::Dying) {
        deathTimer_ += dt;
        if (deathTimer_ > kCorpseDuration) {
            position_.y = -kSinkDepth * std::min(1.0f, (deathTimer_ - kCorpseDuration) / kSinkDuration);
            if (deathTimer_ > kCorpseDuration + kSinkDuration) removable_ = true;
        }
        return 0.0f;
    }

    sinceHit_ += dt;
    if (elite_ == EliteKind::Regenerating && sinceHit_ >= elites::kRegenDelay) {
        health_ = std::min(maxHealth_, health_ + maxHealth_ * elites::kRegenFraction * dt);
    }

    // Recul dû aux balles, amorti dans le temps.
    position_ = Vector3Add(position_, Vector3Scale(knockback_, dt));
    knockback_ = Vector3Scale(knockback_, std::exp(-kKnockbackDamping * dt));

    const Vector3 toPlayer = flatTo(position_, senses.player);
    const float playerDistance = Vector3Length(toPlayer);
    const bool chasingPlayer = Vector3Distance(senses.goal, senses.player) < 1e-3f;

    // Le vampire pris dans la lampe torche détale ; sinon il contourne le joueur.
    fleeing_ = s.fearsLight && senses.inFlashlight;
    Vector3 destination = senses.goal;
    if (s.fearsLight && chasingPlayer && !fleeing_) destination = flankPoint(senses);

    const Vector3 toDestination = flatTo(position_, destination);
    Vector3 heading = Vector3Normalize(toDestination);
    if (fleeing_) {
        heading = Vector3Normalize(senses.lightEscape);
        if (Vector3Length(heading) < 0.5f) heading = Vector3Normalize(Vector3Negate(toPlayer));
    } else if (s.fearsLight && playerDistance > kFlankCommitDistance) {
        // Loin du joueur, il évite les flaques de lumière du décor.
        heading = Vector3Normalize(Vector3Add(heading, Vector3Scale(senses.lightEscape, kLightAversion)));
    }

    // Il regarde le joueur, sauf quand il s'enfuit.
    const Vector3 look = fleeing_ ? heading : flatTo(position_, senses.goal);
    const bool facingLocked = state_ == State::Gesturing && gestureTimed_;
    if (!facingLocked && Vector3Length(look) > 0.001f) {
        yaw_ += angleDelta(yaw_, std::atan2(look.x, look.z)) * (1.0f - std::exp(-kTurnSharpness * dt));
    }

    groanTimer_ -= dt;
    if (groanTimer_ <= 0.0f) {
        groanTimer_ = randomRange(4.0f, 10.0f);
        events.push_back({EventType::EnemyGroan, position_, static_cast<float>(type_)});
    }

    attackCooldown_ = std::max(0.0f, attackCooldown_ - dt);
    float damage = 0.0f;
    const float reach = s.attackRange * s.scale;

    if (state_ == State::Gesturing) {
        gestureLeft_ -= dt;
        if (gestureTimed_ ? gestureLeft_ <= 0.0f : model_->isFinished(anim_)) {
            state_ = State::Chasing;
            model_->play(anim_, clipMove_, true, pace);
        }
        return 0.0f;
    }

    if (state_ == State::Attacking) {
        const float duration = model_->clipDuration(clipAttack_);
        if (!attackDealt_ && anim_.time >= duration * kAttackImpactFraction) {
            attackDealt_ = true;
            if (playerDistance <= reach + kAttackReachBonus) {
                damage = s.damage;
                events.push_back({EventType::EnemyAttack, position_, s.damage});
            }
        }
        if (model_->isFinished(anim_)) {
            state_ = State::Chasing;
            model_->play(anim_, clipMove_, true, pace);
        }
        return damage;
    }

    if (!fleeing_ && playerDistance <= reach && attackCooldown_ <= 0.0f) {
        state_ = State::Attacking;
        attackDealt_ = false;
        attackCooldown_ = s.attackCooldown;
        model_->play(anim_, clipAttack_, false, 1.0f - slowFraction_, true);
        return 0.0f;
    }

    if (fleeing_) {
        const float speed = s.speed * pace * kFleeSpeedBoost;
        position_ = Vector3Add(position_, Vector3Scale(heading, speed * dt));
        model_->play(anim_, clipFlee_ >= 0 ? clipFlee_ : clipMove_, true, speed / s.animReferenceSpeed);
    } else if (!chasingPlayer || playerDistance > reach * 0.8f) {
        const float speed = s.speed * pace;
        position_ = Vector3Add(position_, Vector3Scale(heading, speed * dt));
        model_->play(anim_, clipMove_, true, speed / s.animReferenceSpeed);
    }
    return 0.0f;
}

bool Enemy::hit(float damage, Vector3 direction, float knockback, EventQueue& events)
{
    if (state_ == State::Dying) return false;
    const EnemyStats& s = stats();
    const Vector3 flat = Vector3Normalize({direction.x, 0.0f, direction.z});
    // Blindé : un coup qui arrive face à lui (sens opposé à son regard) est à moitié arrêté.
    const Vector3 facing = {std::sin(yaw_), 0.0f, std::cos(yaw_)};
    const bool deflected = elite_ == EliteKind::Armored && Vector3DotProduct(flat, facing) < 0.0f;
    if (deflected) damage *= elites::kArmorFrontFactor;
    health_ -= damage;
    flash_ = kFlashDuration;
    sinceHit_ = 0.0f;
    knockback_ = Vector3Add(knockback_, Vector3Scale(flat, knockback * (1.0f - s.knockbackResistance)));
    // EnemyHit doit rester le premier événement publié (Game::damageEnemy y précise la nature du coup).
    events.push_back({EventType::EnemyHit, hitCenter(), damage, flat});
    if (deflected) events.push_back({EventType::BulletImpact, hitCenter(), 0.0f, flat});

    if (health_ > 0.0f) return false;
    state_ = State::Dying;
    deathTimer_ = 0.0f;
    model_->play(anim_, clipDie_, false, 1.0f, true);
    const int score = isElite() ? s.score * elites::kRewardMultiplier : s.score;
    events.push_back({EventType::EnemyKilled, hitCenter(), static_cast<float>(score), flat});
    return true;
}

void Enemy::gesture(const char* clip, float speed, float duration)
{
    const int index = model_->findClip(clip);
    if (state_ == State::Dying || index < 0) return;
    state_ = State::Gesturing;
    gestureLeft_ = duration;
    gestureTimed_ = duration > 0.0f;
    model_->play(anim_, index, duration > 0.0f, speed, true);
}

void Enemy::face(Vector3 direction)
{
    if (std::fabs(direction.x) + std::fabs(direction.z) > 1e-4f) yaw_ = std::atan2(direction.x, direction.z);
}

void Enemy::slow(float fraction, float duration)
{
    if (state_ == State::Dying) return;
    slowFraction_ = std::max(slowFraction_, std::clamp(fraction, 0.0f, 1.0f));
    slowTimer_ = std::max(slowTimer_, duration);
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
                          MatrixTranslate(position_.x, position_.y + hoverHeight(), position_.z));
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
    if (isElite()) {
        const Color elite = eliteInfo(elite_).color;
        tint.r = lerpChannel(tint.r, elite.r, kEliteTintStrength);
        tint.g = lerpChannel(tint.g, elite.g, kEliteTintStrength);
        tint.b = lerpChannel(tint.b, elite.b, kEliteTintStrength);
    }
    if (slowTimer_ > 0.0f) {
        const float k = kFrostTintStrength;
        tint.r = lerpChannel(tint.r, kFrostTint.r, k);
        tint.g = lerpChannel(tint.g, kFrostTint.g, k);
        tint.b = lerpChannel(tint.b, kFrostTint.b, k);
    }
    if (flash_ > 0.0f) {
        // Éclat vers le blanc chaud quand l'ennemi est touché.
        const float k = flash_ / kFlashDuration;
        tint.r = lerpChannel(tint.r, 255, k);
        tint.g = lerpChannel(tint.g, 230, k);
        tint.b = lerpChannel(tint.b, 200, k);
    }
    if (stats().ethereal && state_ == State::Dying) {
        tint.a = static_cast<unsigned char>(tint.a * std::max(0.0f, 1.0f - deathTimer_ / kGhostFadeDuration));
        if (tint.a == 0) return;
    }
    model_->draw(anim_, bodyTransform(), tint);
}
