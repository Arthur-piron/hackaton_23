#include "game/Boss.hpp"

#include "core/AssetPath.hpp"
#include "core/Fonts.hpp"
#include "game/Scale.hpp"
#include "ui/UiScale.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace {
// Le Fossoyeur : il s'arrête, arme son bras, puis la pierre part en cloche vers le joueur.
constexpr float kFirstThrowDelay = 2.5f;
constexpr float kThrowCooldown = 4.0f;
constexpr float kEnragedThrowCooldown = 2.6f;
constexpr float kGravediggerRage = 0.5f;       // fraction de vie sous laquelle il enrage
constexpr float kThrowWindup = 0.45f;          // de l'élan au lâcher
constexpr float kThrowAnimSpeed = 0.8f;
constexpr float kMinThrowDistance = 4.0f;
constexpr float kMaxThrowDistance = 18.0f;
constexpr float kReleaseHeight = 3.2f;
constexpr float kFlightTime = 1.15f;
constexpr float kArcHeight = 4.5f;
constexpr float kEnragedSpread = 2.6f;         // enragé : deux pierres de plus, de part et d'autre
constexpr float kTombstoneRadius = 2.2f;
constexpr float kTombstoneDamage = 30.0f;
constexpr float kTombstoneScale = 1.4f;        // × l'échelle Kenney
constexpr float kMinSpin = 280.0f;
constexpr float kMaxSpin = 520.0f;
constexpr float kRubbleLife = 5.0f;
constexpr float kRubbleSink = 1.0f;
constexpr float kRubbleDepth = 0.6f;

// Le Rongé : une action toutes les quelques secondes, plus souvent quand il enrage.
constexpr float kFirstMoveDelay = 3.0f;
constexpr float kMoveCooldown = 4.5f;
constexpr float kEnragedMoveCooldown = 2.8f;
constexpr float kGnawedRage = 0.4f;
constexpr int kSummonEvery = 3;                // une action sur trois est une invocation
// Charge : il se ramasse, une bande rouge marque sa trajectoire, puis il fonce.
constexpr float kChargeMinDistance = 6.0f;
constexpr float kChargeTelegraph = 1.1f;
constexpr float kChargeSpeed = 15.0f;
constexpr float kChargeOvershoot = 4.0f;       // il ne s'arrête pas sur le joueur : il le dépasse
constexpr float kChargeMaxLength = 22.0f;
constexpr float kChargeWidth = 2.6f;
constexpr float kChargeDamage = 45.0f;
constexpr float kChargeAnimSpeed = 2.2f;
constexpr float kChargeBlockedFraction = 0.35f;  // il avance moins que ça : un obstacle le retient
constexpr float kChargeBlockedTime = 0.2f;        // retenu aussi longtemps : la charge s'arrête
constexpr float kTrampleInterval = 0.12f;
constexpr float kTrampleRadius = 1.8f;
// Frappe au sol : zone qui se remplit autour de lui, puis onde de choc.
constexpr float kSlamTelegraph = 1.0f;
constexpr float kSlamRadius = 6.5f;
constexpr float kSlamDamage = 35.0f;
constexpr float kSlamAnimSpeed = 0.55f;
// Invocation : des marcheurs surgissent autour de lui.
constexpr float kSummonDelay = 0.8f;
constexpr int kSummonCount = 5;
constexpr int kEnragedSummonCount = 8;
constexpr float kSummonWaveRadius = 4.0f;
// Fin d'une action en boucle : on relâche la pose aussitôt.
constexpr float kReleasePose = 0.01f;

// Effets au sol.
constexpr Color kWarningColor = {255, 60, 40, 255};
constexpr Color kShockwaveColor = {255, 170, 120, 255};
constexpr Color kSummonColor = {170, 90, 255, 255};
constexpr float kWarningHeight = 0.04f;
constexpr float kShockwaveLife = 0.5f;
constexpr int kShockwaveRings = 3;

// Barre de vie.
constexpr int kBarWidth = 520;
constexpr int kBarHeight = 16;
constexpr int kBarBottomMargin = 78;
constexpr int kBarNameSize = 22;
constexpr Color kBarBackground = {20, 10, 12, 220};
constexpr Color kBarHealth = {215, 45, 40, 255};
constexpr Color kBarTrail = {120, 30, 30, 255};
constexpr Color kBarBorder = {255, 220, 200, 180};
constexpr Color kBarName = {255, 215, 190, 255};
constexpr float kTrailSpeed = 0.35f;           // fraction de vie rattrapée par seconde

float randomRange(float min, float max)
{
    return min + (max - min) * static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f;
}

float flatDistance(Vector3 a, Vector3 b)
{
    return std::hypot(a.x - b.x, a.z - b.z);
}

const Enemy* livingBoss(const std::vector<Enemy>& enemies)
{
    for (const Enemy& e : enemies) {
        if (e.isBoss() && e.isAlive()) return &e;
    }
    return nullptr;
}
}

bool boss::waveBoss(int wave, EnemyType& type)
{
    const int inCycle = (wave - 1) % kBossCycle + 1;
    if (inCycle == kGravediggerWave) {
        type = EnemyType::Gravedigger;
    } else if (inCycle == kGnawedWave) {
        type = EnemyType::Gnawed;
    } else {
        return false;
    }
    return true;
}

const char* boss::tagline(EnemyType type)
{
    return type == EnemyType::Gnawed ? "La faim l'a rendu énorme..." : "Il vient creuser vos tombes...";
}

Bosses::Bosses()
    : tombstoneModel_(LoadModel(assetPath("models/graveyard/gravestone-round.glb").c_str()))
    , rubbleModel_(LoadModel(assetPath("models/graveyard/gravestone-debris.glb").c_str()))
{
}

Bosses::~Bosses()
{
    UnloadModel(tombstoneModel_);
    UnloadModel(rubbleModel_);
}

void Bosses::reset()
{
    flying_.clear();
    rubble_.clear();
    shockwaves_.clear();
    healthTrail_ = 1.0f;
    throwTimer_ = kFirstThrowDelay;
    windup_ = -1.0f;
    move_ = Move::None;
    moveTimer_ = kFirstMoveDelay;
    moveCount_ = 0;
}

void Bosses::setShader(const Shader& shader)
{
    for (Model* model : {&tombstoneModel_, &rubbleModel_}) {
        for (int i = 0; i < model->materialCount; ++i) model->materials[i].shader = shader;
    }
}

BossUpdate Bosses::update(float dt, std::vector<Enemy>& enemies, Vector3 playerPosition, EventQueue& events)
{
    BossUpdate result;
    Enemy* boss = nullptr;
    for (Enemy& e : enemies) {
        if (e.isBoss() && e.isAlive()) boss = &e;
    }

    if (boss) {
        healthTrail_ = std::max(boss->health() / boss->maxHealth(), healthTrail_ - kTrailSpeed * dt);
        if (boss->type() == EnemyType::Gravedigger) {
            updateGravedigger(dt, *boss, playerPosition);
        } else {
            updateGnawed(dt, *boss, playerPosition, events, result);
        }
    } else {
        // Plus de boss en vie : ses minuteries repartent de zéro pour le prochain.
        healthTrail_ = 1.0f;
        windup_ = -1.0f;
        throwTimer_ = kFirstThrowDelay;
        move_ = Move::None;
        moveTimer_ = kFirstMoveDelay;
        moveCount_ = 0;
    }

    for (Tombstone& t : flying_) {
        t.progress += dt / kFlightTime;
        if (t.progress < 1.0f) continue;
        if (flatDistance(playerPosition, t.to) <= kTombstoneRadius) result.damage += kTombstoneDamage;
        events.push_back({EventType::BossImpact, t.to, kTombstoneRadius});
        rubble_.push_back({t.to, randomRange(0.0f, 360.0f), 0.0f});
    }
    flying_.erase(std::remove_if(flying_.begin(), flying_.end(), [](const Tombstone& t) { return t.progress >= 1.0f; }),
                  flying_.end());
    for (Rubble& r : rubble_) r.age += dt;
    rubble_.erase(std::remove_if(rubble_.begin(), rubble_.end(),
                                 [](const Rubble& r) { return r.age >= kRubbleLife + kRubbleSink; }),
                  rubble_.end());
    for (Shockwave& s : shockwaves_) s.age += dt;
    shockwaves_.erase(std::remove_if(shockwaves_.begin(), shockwaves_.end(),
                                     [](const Shockwave& s) { return s.age >= kShockwaveLife; }),
                      shockwaves_.end());
    return result;
}

void Bosses::updateGravedigger(float dt, Enemy& boss, Vector3 playerPosition)
{
    const bool enraged = boss.health() <= boss.maxHealth() * kGravediggerRage;
    const float distance = flatDistance(boss.position(), playerPosition);
    throwTimer_ -= dt;
    if (windup_ >= 0.0f) {
        windup_ -= dt;
        if (windup_ < 0.0f) throwTombstones(boss, playerPosition);
    } else if (throwTimer_ <= 0.0f && !boss.isGesturing() && distance >= kMinThrowDistance &&
               distance <= kMaxThrowDistance) {
        boss.gesture("attack-melee-right", kThrowAnimSpeed);
        windup_ = kThrowWindup;
        throwTimer_ = enraged ? kEnragedThrowCooldown : kThrowCooldown;
    }
}

void Bosses::throwTombstones(const Enemy& boss, Vector3 playerPosition)
{
    const Vector3 from = Vector3Add(boss.position(), {0.0f, kReleaseHeight, 0.0f});
    const Vector3 target = {playerPosition.x, 0.0f, playerPosition.z};
    flying_.push_back({from, target, 0.0f, randomRange(kMinSpin, kMaxSpin)});
    if (boss.health() > boss.maxHealth() * kGravediggerRage) return;
    // Enragé : il couvre aussi les côtés, pour couper la fuite.
    const Vector3 toTarget = Vector3Normalize({target.x - from.x, 0.0f, target.z - from.z});
    const Vector3 side = {-toTarget.z, 0.0f, toTarget.x};
    for (float sign : {-1.0f, 1.0f}) {
        flying_.push_back({from, Vector3Add(target, Vector3Scale(side, sign * kEnragedSpread)), 0.0f,
                           randomRange(kMinSpin, kMaxSpin)});
    }
}

void Bosses::startMove(Enemy& boss, Vector3 playerPosition)
{
    const float distance = flatDistance(boss.position(), playerPosition);
    ++moveCount_;
    moveActive_ = false;
    moveTime_ = 0.0f;
    lastPosition_ = {boss.position().x, 0.0f, boss.position().z};
    if (moveCount_ % kSummonEvery == 0) {
        move_ = Move::Summon;
        boss.gesture("interact-right", 1.0f);
    } else if (distance >= kChargeMinDistance) {
        // La direction est figée à l'annonce : le joueur a le temps de sortir de la bande.
        move_ = Move::Charge;
        chargeOrigin_ = lastPosition_;
        chargeDirection_ =
            Vector3Normalize({playerPosition.x - chargeOrigin_.x, 0.0f, playerPosition.z - chargeOrigin_.z});
        chargeLength_ = std::min(distance + kChargeOvershoot, kChargeMaxLength);
        boss.face(chargeDirection_);
        boss.gesture("crouch", 1.0f, kChargeTelegraph);
    } else {
        move_ = Move::Slam;
        boss.gesture("attack-melee-left", kSlamAnimSpeed);
    }
}

void Bosses::endMove(const Enemy& boss)
{
    move_ = Move::None;
    const bool enraged = boss.health() <= boss.maxHealth() * kGnawedRage;
    moveTimer_ = enraged ? kEnragedMoveCooldown : kMoveCooldown;
}

void Bosses::updateGnawed(float dt, Enemy& boss, Vector3 playerPosition, EventQueue& events, BossUpdate& result)
{
    if (move_ == Move::None) {
        moveTimer_ -= dt;
        if (moveTimer_ <= 0.0f && !boss.isGesturing()) startMove(boss, playerPosition);
        return;
    }
    moveTime_ += dt;
    const Vector3 at = {boss.position().x, 0.0f, boss.position().z};

    switch (move_) {
    case Move::Charge:
        if (!moveActive_) {
            if (moveTime_ < kChargeTelegraph) break;
            moveActive_ = true;
            moveTime_ = 0.0f;
            chargeTravelled_ = 0.0f;
            trampleTimer_ = 0.0f;
            blockedTime_ = 0.0f;
            chargeHit_ = false;
            lastPosition_ = at;
            boss.face(chargeDirection_);
            boss.gesture("sprint", kChargeAnimSpeed, kChargeMaxLength / kChargeSpeed);
            break;
        }
        // Retenu par le décor ou une structure : il la piétine aussitôt ; s'il ne passe toujours pas,
        // la charge s'arrête là.
        if (moveTime_ > dt && flatDistance(at, lastPosition_) < kChargeSpeed * dt * kChargeBlockedFraction) {
            if (blockedTime_ <= 0.0f) trampleTimer_ = 0.0f;
            blockedTime_ += dt;
            if (blockedTime_ >= kChargeBlockedTime) {
                boss.gesture("idle", 1.0f, kReleasePose);
                endMove(boss);
                break;
            }
        } else {
            blockedTime_ = 0.0f;
        }
        lastPosition_ = at;
        {
            const float step = std::min(kChargeSpeed * dt, chargeLength_ - chargeTravelled_);
            boss.push(Vector3Scale(chargeDirection_, step));
            chargeTravelled_ += step;
        }
        trampleTimer_ -= dt;
        if (trampleTimer_ <= 0.0f) {
            trampleTimer_ = kTrampleInterval;
            events.push_back({EventType::BossTrample, Vector3Add(at, Vector3Scale(chargeDirection_, boss.radius())),
                              kTrampleRadius, chargeDirection_});
        }
        if (!chargeHit_ && flatDistance(at, playerPosition) <= boss.radius() + kChargeWidth * 0.5f) {
            chargeHit_ = true;
            result.damage += kChargeDamage;
        }
        if (chargeTravelled_ >= chargeLength_) {
            boss.gesture("idle", 1.0f, kReleasePose);
            endMove(boss);
        }
        break;
    case Move::Slam:
        if (moveTime_ < kSlamTelegraph) break;
        if (flatDistance(at, playerPosition) <= kSlamRadius) result.damage += kSlamDamage;
        events.push_back({EventType::BossImpact, at, kSlamRadius});
        shockwaves_.push_back({at, kSlamRadius, kShockwaveColor, 0.0f});
        endMove(boss);
        break;
    case Move::Summon:
        if (moveTime_ < kSummonDelay) break;
        result.summons = boss.health() <= boss.maxHealth() * kGnawedRage ? kEnragedSummonCount : kSummonCount;
        result.summonCenter = at;
        shockwaves_.push_back({at, kSummonWaveRadius, kSummonColor, 0.0f});
        endMove(boss);
        break;
    case Move::None:
        break;
    }
}

void Bosses::drawGroundWarning(Vector3 center, float radius, float progress) const
{
    // Le disque se remplit et l'anneau se resserre jusqu'à l'impact.
    const Vector3 ground = {center.x, kWarningHeight, center.z};
    DrawCylinder(ground, radius, radius, 0.0f, 40, ColorAlpha(kWarningColor, 0.1f + 0.3f * progress));
    DrawCircle3D(ground, radius, {1.0f, 0.0f, 0.0f}, 90.0f, kWarningColor);
    DrawCircle3D(ground, radius * (1.0f - progress), {1.0f, 0.0f, 0.0f}, 90.0f, kWarningColor);
}

void Bosses::drawChargeLine() const
{
    const float progress = moveActive_ ? 1.0f : std::min(1.0f, moveTime_ / kChargeTelegraph);
    const Vector3 side = Vector3Scale({-chargeDirection_.z, 0.0f, chargeDirection_.x}, kChargeWidth * 0.5f);
    const Vector3 start = {chargeOrigin_.x, kWarningHeight, chargeOrigin_.z};
    const Vector3 end = Vector3Add(start, Vector3Scale(chargeDirection_, chargeLength_));
    // La bande se remplit depuis le Rongé jusqu'au bout de la charge.
    const Vector3 filled = Vector3Lerp(start, end, progress);
    const Vector3 a = Vector3Subtract(start, side);
    const Vector3 b = Vector3Add(start, side);
    const Vector3 c = Vector3Add(filled, side);
    const Vector3 d = Vector3Subtract(filled, side);
    const Color fill = ColorAlpha(kWarningColor, moveActive_ ? 0.15f : 0.35f);
    // Les deux sens d'enroulement : la bande est visible quelle que soit son orientation.
    DrawTriangle3D(a, b, c, fill);
    DrawTriangle3D(a, c, d, fill);
    DrawTriangle3D(a, c, b, fill);
    DrawTriangle3D(a, d, c, fill);
    DrawLine3D(Vector3Subtract(start, side), Vector3Subtract(end, side), kWarningColor);
    DrawLine3D(Vector3Add(start, side), Vector3Add(end, side), kWarningColor);
    DrawLine3D(Vector3Subtract(end, side), Vector3Add(end, side), kWarningColor);
}

void Bosses::draw() const
{
    const float scale = kKenneyScale * kTombstoneScale;
    for (const Rubble& r : rubble_) {
        Vector3 at = r.position;
        if (r.age > kRubbleLife) at.y -= kRubbleDepth * (r.age - kRubbleLife) / kRubbleSink;
        DrawModelEx(rubbleModel_, at, {0.0f, 1.0f, 0.0f}, r.yaw, {scale, scale, scale}, WHITE);
    }
    for (const Tombstone& t : flying_) {
        drawGroundWarning(t.to, kTombstoneRadius, t.progress);
        // Trajectoire en cloche.
        Vector3 at = Vector3Lerp(t.from, t.to, t.progress);
        at.y += kArcHeight * 4.0f * t.progress * (1.0f - t.progress);
        DrawModelEx(tombstoneModel_, at, {1.0f, 0.0f, 0.3f}, t.spin * t.progress, {scale, scale, scale}, WHITE);
    }
    if (move_ == Move::Charge) drawChargeLine();
    if (move_ == Move::Slam) drawGroundWarning(lastPosition_, kSlamRadius, std::min(1.0f, moveTime_ / kSlamTelegraph));
    for (const Shockwave& s : shockwaves_) {
        const float k = s.age / kShockwaveLife;
        for (int ring = 0; ring < kShockwaveRings; ++ring) {
            const float r = s.radius * std::min(1.0f, k + 0.12f * static_cast<float>(ring));
            DrawCircle3D({s.center.x, kWarningHeight + 0.02f * static_cast<float>(ring), s.center.z}, r,
                         {1.0f, 0.0f, 0.0f}, 90.0f, ColorAlpha(s.color, 1.0f - k));
        }
    }
}

void Bosses::drawHealthBar(const std::vector<Enemy>& enemies) const
{
    const Enemy* boss = livingBoss(enemies);
    if (!boss) return;
    const int x = (ui::screenWidth() - kBarWidth) / 2;
    const int y = ui::screenHeight() - kBarBottomMargin;
    const float fraction = std::clamp(boss->health() / boss->maxHealth(), 0.0f, 1.0f);
    const char* name = boss->stats().name;
    fonts::drawText(name, (ui::screenWidth() - fonts::measureText(name, kBarNameSize)) / 2, y - kBarNameSize - 6,
                    kBarNameSize, kBarName);
    DrawRectangle(x, y, kBarWidth, kBarHeight, kBarBackground);
    DrawRectangle(x, y, static_cast<int>(kBarWidth * healthTrail_), kBarHeight, kBarTrail);
    DrawRectangle(x, y, static_cast<int>(kBarWidth * fraction), kBarHeight, kBarHealth);
    DrawRectangleLines(x - 1, y - 1, kBarWidth + 2, kBarHeight + 2, kBarBorder);
}
