#include "game/Pickups.hpp"

#include "game/Economy.hpp"
#include "raymath.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kGravity = 18.0f;
constexpr float kBounce = 0.45f;
constexpr float kRestHeight = 0.35f;     // hauteur de flottement au-dessus du sol
constexpr float kBobAmplitude = 0.06f;
constexpr float kGroundFriction = 4.0f;
constexpr float kLifetime = 20.0f;
constexpr float kBlinkDuration = 3.0f;   // clignote pendant les dernières secondes
constexpr float kBlinkPeriod = 0.22f;
constexpr float kMagnetRadius = 3.0f;
constexpr float kMagnetDelay = 0.4f;     // laisse le temps de voir le jaillissement
constexpr float kMagnetAcceleration = 45.0f;
constexpr float kMagnetMaxSpeed = 22.0f;
constexpr float kPickupRadius = 0.7f;
// Pluie de pièces : anneau autour du joueur, hauteur de départ.
constexpr float kRainMinRadius = 1.5f;
constexpr float kRainMaxRadius = 6.0f;
constexpr float kRainMinHeight = 5.0f;
constexpr float kRainMaxHeight = 10.0f;
constexpr float kPlayerChestHeight = 0.8f;

constexpr int kGoldPerCoin = 5;
constexpr int kMaxCoins = 8;
constexpr int kXpPerOrb = 10;
constexpr int kMaxOrbs = 4;

constexpr float kCoinRadius = 0.2f;
constexpr float kCoinHalfThickness = 0.025f;
constexpr float kCoinSpin = 3.0f;        // radians par seconde
constexpr float kOrbRadius = 0.11f;

constexpr Color kCoinColor = {255, 196, 50, 255};
constexpr Color kCoinFace = {255, 228, 120, 255};
constexpr Color kCoinGlow = {255, 180, 50, 60};
constexpr Color kOrbColor = {150, 255, 225, 255};
constexpr Color kOrbGlow = {40, 220, 180, 60};

float randomRange(float min, float max)
{
    return min + (max - min) * static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f;
}

// Répartit amount en count parts presque égales et appelle emit(part) pour chacune.
template <typename Emit>
void split(int amount, int count, Emit emit)
{
    for (int i = 0; i < count; ++i) emit(amount / count + (i < amount % count ? 1 : 0));
}
}

void Pickups::spawn(Kind kind, int value, Vector3 position)
{
    const float angle = randomRange(0.0f, 2.0f * PI);
    const float speed = randomRange(1.2f, 3.2f);
    Item item;
    item.kind = kind;
    item.value = value;
    item.position = position;
    item.velocity = {std::cos(angle) * speed, randomRange(4.5f, 6.5f), std::sin(angle) * speed};
    item.phase = randomRange(0.0f, 2.0f * PI);
    items_.push_back(item);
}

void Pickups::spawnLoot(Vector3 position, int gold, int xp)
{
    if (gold > 0) {
        const int coins = std::clamp((gold + kGoldPerCoin - 1) / kGoldPerCoin, 1, kMaxCoins);
        split(gold, coins, [&](int value) { spawn(Kind::Gold, value, position); });
    }
    if (xp > 0) {
        const int orbs = std::clamp((xp + kXpPerOrb - 1) / kXpPerOrb, 1, kMaxOrbs);
        split(xp, orbs, [&](int value) { spawn(Kind::Xp, value, position); });
    }
}

void Pickups::spawnCoinRain(Vector3 center, int coins, int goldPerCoin)
{
    for (int i = 0; i < coins; ++i) {
        const float angle = randomRange(0.0f, 2.0f * PI);
        const float distance = randomRange(kRainMinRadius, kRainMaxRadius);
        Item item;
        item.value = goldPerCoin;
        item.position = {center.x + std::cos(angle) * distance, randomRange(kRainMinHeight, kRainMaxHeight),
                         center.z + std::sin(angle) * distance};
        item.velocity = {0.0f, -randomRange(0.0f, 4.0f), 0.0f};
        item.phase = randomRange(0.0f, 2.0f * PI);
        items_.push_back(item);
    }
}

void Pickups::update(float dt, Vector3 playerPosition, Economy& economy, EventQueue& events)
{
    const Vector3 target = {playerPosition.x, kPlayerChestHeight, playerPosition.z};
    for (Item& item : items_) {
        if (item.collected) continue;
        item.age += dt;
        const Vector3 flat = {target.x - item.position.x, 0.0f, target.z - item.position.z};
        if (!item.attracted && item.age > kMagnetDelay && Vector3Length(flat) < kMagnetRadius * magnetMultiplier_) {
            item.attracted = true;
        }

        if (item.attracted) {
            // Accélère vers le joueur ; la vitesse est réorientée à chaque image pour ne pas le rater.
            const Vector3 toTarget = Vector3Subtract(target, item.position);
            const float distance = Vector3Length(toTarget);
            const float speed = std::min(kMagnetMaxSpeed, Vector3Length(item.velocity) + kMagnetAcceleration * dt);
            if (distance > 0.001f) item.velocity = Vector3Scale(toTarget, speed / distance);
            item.position = Vector3Add(item.position, Vector3Scale(item.velocity, std::min(dt, distance / speed)));
        } else {
            item.velocity.y -= kGravity * dt;
            item.position = Vector3Add(item.position, Vector3Scale(item.velocity, dt));
            if (item.position.y < kRestHeight) {
                item.position.y = kRestHeight;
                item.velocity.y = item.velocity.y < -1.0f ? -item.velocity.y * kBounce : 0.0f;
                const float friction = std::exp(-kGroundFriction * dt);
                item.velocity.x *= friction;
                item.velocity.z *= friction;
            }
        }

        if (Vector3Distance(item.position, target) < kPickupRadius) {
            item.collected = true;
            if (item.kind == Kind::Gold) {
                economy.addGold(item.value);
                events.push_back({EventType::GoldPicked, item.position, static_cast<float>(item.value)});
            } else {
                events.push_back({EventType::XpPicked, item.position, static_cast<float>(item.value)});
                economy.addXp(item.value, events);
            }
        }
    }
    // Un objet attiré ne disparaît pas en plein vol.
    items_.erase(std::remove_if(items_.begin(), items_.end(),
                                [](const Item& item) {
                                    return item.collected || (!item.attracted && item.age >= kLifetime);
                                }),
                 items_.end());
}

void Pickups::draw() const
{
    struct Visible {
        Kind kind;
        Vector3 center;
        float age;
        float phase;
    };
    std::vector<Visible> visible;
    visible.reserve(items_.size());
    for (const Item& item : items_) {
        const float left = kLifetime - item.age;
        if (!item.attracted && left < kBlinkDuration && std::fmod(item.age, kBlinkPeriod) > kBlinkPeriod * 0.6f) {
            continue;
        }
        Vector3 center = item.position;
        if (!item.attracted) center.y += kBobAmplitude * std::sin(item.age * 3.0f + item.phase);
        visible.push_back({item.kind, center, item.age, item.phase});
    }

    for (const Visible& v : visible) {
        if (v.kind == Kind::Gold) {
            // Pièce qui tourne autour de l'axe X (de face puis de tranche vue de la caméra), face en relief.
            const float spin = v.age * kCoinSpin + v.phase;
            const Vector3 axis = {0.0f, std::cos(spin), std::sin(spin)};
            const Vector3 half = Vector3Scale(axis, kCoinHalfThickness);
            DrawCylinderEx(Vector3Subtract(v.center, half), Vector3Add(v.center, half), kCoinRadius, kCoinRadius, 16,
                           kCoinColor);
            const Vector3 relief = Vector3Scale(half, 1.4f);
            DrawCylinderEx(Vector3Subtract(v.center, relief), Vector3Add(v.center, relief), kCoinRadius * 0.62f,
                           kCoinRadius * 0.62f, 16, kCoinFace);
        } else {
            DrawSphereEx(v.center, kOrbRadius, 10, 10, kOrbColor);
        }
    }

    // Halos lumineux : mélange additif, sans écrire dans le tampon de profondeur.
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();
    for (const Visible& v : visible) {
        if (v.kind == Kind::Gold) {
            DrawSphereEx(v.center, kCoinRadius * 1.6f, 8, 8, kCoinGlow);
            DrawSphereEx(v.center, kCoinRadius * 2.6f, 8, 8, {kCoinGlow.r, kCoinGlow.g, kCoinGlow.b, 22});
        } else {
            const float pulse = 1.0f + 0.15f * std::sin(v.age * 6.0f + v.phase);
            DrawSphereEx(v.center, kOrbRadius * 2.2f * pulse, 8, 8, kOrbGlow);
            DrawSphereEx(v.center, kOrbRadius * 3.6f * pulse, 8, 8, {kOrbGlow.r, kOrbGlow.g, kOrbGlow.b, 25});
        }
    }
    EndBlendMode();
    rlEnableDepthMask();
}
