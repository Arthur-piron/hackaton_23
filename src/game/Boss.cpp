#include "game/Boss.hpp"

#include "core/AssetPath.hpp"
#include "core/Fonts.hpp"
#include "game/Scale.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace {
// Lancer : il s'arrête, arme son bras, puis la pierre part en cloche vers le joueur.
constexpr float kFirstThrowDelay = 2.5f;
constexpr float kThrowCooldown = 4.0f;
constexpr float kEnragedThrowCooldown = 2.6f;
constexpr float kEnragedHealth = 0.5f;         // fraction de vie sous laquelle il enrage
constexpr float kThrowWindup = 0.45f;          // de l'élan au lâcher
constexpr float kThrowAnimSpeed = 0.8f;
constexpr float kMinThrowDistance = 4.0f;
constexpr float kMaxThrowDistance = 18.0f;
constexpr float kReleaseHeight = 3.2f;
constexpr float kFlightTime = 1.15f;
constexpr float kArcHeight = 4.5f;
constexpr float kEnragedSpread = 2.6f;         // enragé : deux pierres de plus, de part et d'autre
constexpr float kImpactRadius = 2.2f;
constexpr float kImpactDamage = 30.0f;
constexpr float kTombstoneScale = 1.4f;        // × l'échelle Kenney
constexpr float kMinSpin = 280.0f;
constexpr float kMaxSpin = 520.0f;
// Débris laissés par l'impact : ils restent puis s'enfoncent.
constexpr float kRubbleLife = 5.0f;
constexpr float kRubbleSink = 1.0f;
constexpr float kRubbleDepth = 0.6f;
// Zone d'impact : disque qui se remplit et anneau qui se resserre.
constexpr Color kWarningColor = {255, 60, 40, 255};
constexpr float kWarningHeight = 0.04f;
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

const Enemy* livingBoss(const std::vector<Enemy>& enemies)
{
    for (const Enemy& e : enemies) {
        if (e.isBoss() && e.isAlive()) return &e;
    }
    return nullptr;
}
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
    throwTimer_ = kFirstThrowDelay;
    windup_ = -1.0f;
    healthTrail_ = 1.0f;
}

void Bosses::setShader(const Shader& shader)
{
    for (Model* model : {&tombstoneModel_, &rubbleModel_}) {
        for (int i = 0; i < model->materialCount; ++i) model->materials[i].shader = shader;
    }
}

void Bosses::release(const Enemy& boss, Vector3 playerPosition)
{
    const Vector3 from = Vector3Add(boss.position(), {0.0f, kReleaseHeight, 0.0f});
    const Vector3 target = {playerPosition.x, 0.0f, playerPosition.z};
    flying_.push_back({from, target, 0.0f, randomRange(kMinSpin, kMaxSpin)});
    if (boss.health() > boss.maxHealth() * kEnragedHealth) return;
    // Enragé : il couvre aussi les côtés, pour couper la fuite.
    const Vector3 toTarget = Vector3Normalize({target.x - from.x, 0.0f, target.z - from.z});
    const Vector3 side = {-toTarget.z, 0.0f, toTarget.x};
    for (float sign : {-1.0f, 1.0f}) {
        flying_.push_back({from, Vector3Add(target, Vector3Scale(side, sign * kEnragedSpread)), 0.0f,
                           randomRange(kMinSpin, kMaxSpin)});
    }
}

float Bosses::update(float dt, std::vector<Enemy>& enemies, Vector3 playerPosition, EventQueue& events)
{
    Enemy* boss = nullptr;
    for (Enemy& e : enemies) {
        if (e.isBoss() && e.isAlive()) boss = &e;
    }

    if (boss) {
        const float fraction = boss->health() / boss->maxHealth();
        healthTrail_ = std::max(fraction, healthTrail_ - kTrailSpeed * dt);
        const bool enraged = fraction <= kEnragedHealth;
        const float distance = Vector3Distance(boss->position(), playerPosition);
        throwTimer_ -= dt;
        if (windup_ >= 0.0f) {
            windup_ -= dt;
            if (windup_ < 0.0f) release(*boss, playerPosition);
        } else if (throwTimer_ <= 0.0f && !boss->isGesturing() && distance >= kMinThrowDistance &&
                   distance <= kMaxThrowDistance) {
            boss->gesture("attack-melee-right", kThrowAnimSpeed);
            windup_ = kThrowWindup;
            throwTimer_ = enraged ? kEnragedThrowCooldown : kThrowCooldown;
        }
    } else {
        windup_ = -1.0f;
        throwTimer_ = kFirstThrowDelay;
        healthTrail_ = 1.0f;
    }

    float damage = 0.0f;
    for (Tombstone& t : flying_) {
        t.progress += dt / kFlightTime;
        if (t.progress < 1.0f) continue;
        const float dx = playerPosition.x - t.to.x;
        const float dz = playerPosition.z - t.to.z;
        if (dx * dx + dz * dz <= kImpactRadius * kImpactRadius) damage += kImpactDamage;
        events.push_back({EventType::TombstoneImpact, t.to, kImpactRadius});
        rubble_.push_back({t.to, randomRange(0.0f, 360.0f), 0.0f});
    }
    flying_.erase(std::remove_if(flying_.begin(), flying_.end(), [](const Tombstone& t) { return t.progress >= 1.0f; }),
                  flying_.end());
    for (Rubble& r : rubble_) r.age += dt;
    rubble_.erase(std::remove_if(rubble_.begin(), rubble_.end(),
                                 [](const Rubble& r) { return r.age >= kRubbleLife + kRubbleSink; }),
                  rubble_.end());
    return damage;
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
        // Zone d'impact : le disque se remplit et l'anneau se resserre jusqu'à l'impact.
        const Vector3 ground = {t.to.x, kWarningHeight, t.to.z};
        DrawCylinder(ground, kImpactRadius, kImpactRadius, 0.0f, 32, ColorAlpha(kWarningColor, 0.1f + 0.3f * t.progress));
        DrawCircle3D(ground, kImpactRadius, {1.0f, 0.0f, 0.0f}, 90.0f, kWarningColor);
        DrawCircle3D(ground, kImpactRadius * (1.0f - t.progress), {1.0f, 0.0f, 0.0f}, 90.0f, kWarningColor);
        // Trajectoire en cloche.
        Vector3 at = Vector3Lerp(t.from, t.to, t.progress);
        at.y += kArcHeight * 4.0f * t.progress * (1.0f - t.progress);
        DrawModelEx(tombstoneModel_, at, {1.0f, 0.0f, 0.3f}, t.spin * t.progress, {scale, scale, scale}, WHITE);
    }
}

void Bosses::drawHealthBar(const std::vector<Enemy>& enemies) const
{
    const Enemy* boss = livingBoss(enemies);
    if (!boss) return;
    const int x = (GetScreenWidth() - kBarWidth) / 2;
    const int y = GetScreenHeight() - kBarBottomMargin;
    const float fraction = std::clamp(boss->health() / boss->maxHealth(), 0.0f, 1.0f);
    const char* name = boss->stats().name;
    fonts::drawText(name, (GetScreenWidth() - fonts::measureText(name, kBarNameSize)) / 2, y - kBarNameSize - 6,
                    kBarNameSize, kBarName);
    DrawRectangle(x, y, kBarWidth, kBarHeight, kBarBackground);
    DrawRectangle(x, y, static_cast<int>(kBarWidth * healthTrail_), kBarHeight, kBarTrail);
    DrawRectangle(x, y, static_cast<int>(kBarWidth * fraction), kBarHeight, kBarHealth);
    DrawRectangleLines(x - 1, y - 1, kBarWidth + 2, kBarHeight + 2, kBarBorder);
}
