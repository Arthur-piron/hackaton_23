#include "game/Flashlight.hpp"

#include "game/World.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
// Batterie : 25 s d'éclairage continu, la dynamo rend 2 % par seconde jusqu'à 50 %.
constexpr float kBaseCapacity = 25.0f;
constexpr float kDynamoRate = 0.02f;
constexpr float kDynamoCap = 0.5f;
constexpr float kDynamoDelay = 1.0f;
constexpr float kMinChargeToLight = 0.05f;
// Batterie faible : le faisceau baisse et hoquette de plus en plus souvent.
constexpr float kLowThreshold = 0.2f;
constexpr float kLowMinIntensity = 0.5f;
constexpr float kDropoutIntensity = 0.12f;
constexpr float kFlickerSoundInterval = 0.6f;

// Effet sur les morts.
constexpr float kSlowFraction = 0.5f;
constexpr float kSlowLinger = 0.25f;     // le ralenti survit un instant à la sortie du cône
constexpr float kBurnPerSecond = 8.0f;
constexpr float kTickInterval = 0.25f;
constexpr float kMinTickDamage = 1.0f;

constexpr float kBatteryDropChance = 0.06f;
constexpr float kBruteBatteryDropChance = 0.25f;
// Les squelettes, sans chair à brûler, ignorent la lumière.
constexpr float kSkeletonLightSensitivity = 0.0f;

float randomRange(float min, float max)
{
    return min + (max - min) * static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f;
}
}

float Flashlight::lightSensitivity(EnemyType type)
{
    return type == EnemyType::Skeleton ? kSkeletonLightSensitivity : 1.0f;
}

float Flashlight::batteryDropChance(EnemyType type)
{
    return type == EnemyType::Brute ? kBruteBatteryDropChance : kBatteryDropChance;
}

void Flashlight::reset()
{
    charge_ = 1.0f;
    on_ = false;
    needsRelease_ = false;
    idleTime_ = 0.0f;
    flickerTimer_ = 0.0f;
    dropout_ = 0.0f;
    flickerSoundCooldown_ = 0.0f;
    tickTimer_ = 0.0f;
    pending_.clear();
}

void Flashlight::setModifiers(float capacityMultiplier, float bonusDamagePerSecond)
{
    capacityMultiplier_ = std::max(0.1f, capacityMultiplier);
    bonusDamage_ = bonusDamagePerSecond;
}

void Flashlight::recharge(float fraction)
{
    charge_ = std::min(1.0f, charge_ + fraction);
}

bool Flashlight::isLow() const
{
    return charge_ < kLowThreshold;
}

float Flashlight::intensity() const
{
    if (!on_) return 0.0f;
    const float base = isLow() ? Lerp(kLowMinIntensity, 1.0f, charge_ / kLowThreshold) : 1.0f;
    return dropout_ > 0.0f ? base * kDropoutIntensity : base;
}

void Flashlight::update(float dt, bool held, Vector3 position, EventQueue& events)
{
    if (!held) needsRelease_ = false;
    const bool wanted = held && !needsRelease_;
    if (wanted && !on_) {
        if (charge_ >= kMinChargeToLight) {
            on_ = true;
            events.push_back({EventType::FlashlightSwitched, position, 1.0f});
        } else {
            needsRelease_ = true;
            events.push_back({EventType::FlashlightSwitched, position, -1.0f});
        }
    } else if (!wanted && on_) {
        on_ = false;
        events.push_back({EventType::FlashlightSwitched, position, 0.0f});
    }

    flickerSoundCooldown_ = std::max(0.0f, flickerSoundCooldown_ - dt);
    if (!on_) {
        dropout_ = 0.0f;
        flickerTimer_ = 0.0f;
        idleTime_ += dt;
        if (idleTime_ >= kDynamoDelay && charge_ < kDynamoCap) {
            charge_ = std::min(kDynamoCap, charge_ + kDynamoRate * dt);
        }
        return;
    }

    idleTime_ = 0.0f;
    charge_ -= dt / (kBaseCapacity * capacityMultiplier_);
    if (charge_ <= 0.0f) {
        // Batterie à plat : la lampe s'éteint en grésillant.
        charge_ = 0.0f;
        on_ = false;
        needsRelease_ = true;
        events.push_back({EventType::FlashlightFlicker, position});
        events.push_back({EventType::FlashlightSwitched, position, 0.0f});
        return;
    }
    updateFlicker(dt, position, events);
}

void Flashlight::updateFlicker(float dt, Vector3 position, EventQueue& events)
{
    dropout_ = std::max(0.0f, dropout_ - dt);
    if (!isLow()) {
        flickerTimer_ = 0.0f;
        return;
    }
    flickerTimer_ -= dt;
    if (flickerTimer_ > 0.0f) return;
    // Plus la batterie est vide, plus les hoquets sont rapprochés.
    const float k = charge_ / kLowThreshold;
    flickerTimer_ = randomRange(0.12f, 0.35f + 1.3f * k);
    dropout_ = randomRange(0.04f, 0.12f);
    if (flickerSoundCooldown_ <= 0.0f) {
        flickerSoundCooldown_ = kFlickerSoundInterval;
        events.push_back({EventType::FlashlightFlicker, position});
    }
}

std::vector<Flashlight::LightTick> Flashlight::illuminate(float dt, Vector3 origin, Vector3 direction,
                                                          std::vector<Enemy>& enemies, const World& world)
{
    const Vector3 flat = {direction.x, 0.0f, direction.z};
    if (isLit() && Vector3Length(flat) > 1e-4f) {
        const Vector3 axis = Vector3Normalize(flat);
        const float halfAngle = kHalfAngleDegrees * DEG2RAD;
        const float damagePerSecond = kBurnPerSecond + bonusDamage_;
        for (Enemy& e : enemies) {
            const float sensitivity = lightSensitivity(e.type());
            if (!e.isAlive() || sensitivity <= 0.0f) continue;
            const Vector3 to = {e.position().x - origin.x, 0.0f, e.position().z - origin.z};
            const float distance = Vector3Length(to);
            if (distance > kRange + e.radius()) continue;
            if (distance > e.radius()) {
                // Le cône touche le corps entier, pas seulement son centre.
                const float angle = std::acos(std::clamp(Vector3DotProduct(Vector3Scale(to, 1.0f / distance), axis),
                                                         -1.0f, 1.0f));
                if (angle > halfAngle + std::asin(e.radius() / distance)) continue;
            }
            const Vector3 target = e.hitCenter();
            Vector3 hit;
            if (world.segmentBlocked({origin.x, target.y, origin.z}, target, hit)) continue;
            e.slow(kSlowFraction, kSlowLinger);
            pending_[e.id()] += damagePerSecond * sensitivity * dt;
        }
    }

    std::vector<LightTick> ticks;
    tickTimer_ -= dt;
    if (tickTimer_ > 0.0f) return ticks;
    tickTimer_ = kTickInterval;
    if (pending_.empty()) return ticks;
    // Une brûlure trop faible pour s'afficher attend la morsure suivante ; les morts l'oublient.
    std::unordered_map<unsigned, float> carried;
    for (Enemy& e : enemies) {
        const auto it = pending_.find(e.id());
        if (it == pending_.end() || !e.isAlive()) continue;
        if (it->second >= kMinTickDamage) {
            ticks.push_back({&e, it->second});
        } else {
            carried.emplace(it->first, it->second);
        }
    }
    pending_ = std::move(carried);
    return ticks;
}
