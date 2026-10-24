#include "game/Crates.hpp"

#include "game/Scale.hpp"
#include "game/World.hpp"
#include "game/Zones.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
// Modèle, échelle, PV et teinte de chaque sorte de caisse, dans l'ordre de CrateKind.
struct KindDef {
    const char* model;
    float scale;
    float health;
    Color tint;
    int weight; // proportion en pourcentage
};
constexpr KindDef kKindDefs[] = {
    {"survival/box.glb", 1.4f, 25.0f, WHITE, 45},
    {"survival/box-large.glb", 1.4f, 50.0f, WHITE, 25},
    {"blaster/crate-medium.glb", 0.7f, 30.0f, WHITE, 15},
    {"survival/barrel.glb", 1.4f, 35.0f, WHITE, 10},
    {"survival/barrel.glb", 1.4f, 15.0f, {255, 90, 70, 255}, 5},
};
static_assert(sizeof(kKindDefs) / sizeof(kKindDefs[0]) == static_cast<size_t>(CrateKind::Count),
              "une définition par sorte de caisse");

constexpr float kDropHeight = 3.0f;
constexpr float kGravity = 22.0f;
constexpr float kAreaMargin = 1.5f;      // distance minimale aux murs d'une zone
constexpr float kMinPlayerDistance = 4.0f;
constexpr float kMinSpawnDistance = 3.0f;
constexpr float kMinCrateDistance = 2.0f;
constexpr float kFreeRadius = 0.9f;      // la caisse ne doit toucher aucun décor
constexpr int kPlacementTries = 25;
constexpr float kHitMargin = 0.15f;      // une balle arrêtée tout près de la caisse la touche
constexpr float kHurtTime = 0.15f;
constexpr float kHurtShake = 0.06f;
constexpr Color kHurtTint = {255, 220, 200, 255};

float randomUnit()
{
    return static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f;
}
}

Crates::Crates(World& world)
    : world_(world)
{
    for (size_t i = 0; i < kinds_.size(); ++i) {
        const KindDef& def = kKindDefs[i];
        kinds_[i] = {&world_.model(def.model), def.scale, def.health, def.tint};
    }
    for (Crate& crate : crates_) {
        crate.blocker = world_.addBlocker({});
        world_.setBlockerActive(crate.blocker, false);
    }
}

void Crates::clear()
{
    for (Crate& crate : crates_) {
        crate.alive = false;
        world_.setBlockerActive(crate.blocker, false);
    }
    blasts_.clear();
}

CrateKind Crates::randomKind()
{
    int roll = GetRandomValue(0, 99);
    for (size_t i = 0; i < static_cast<size_t>(CrateKind::Count); ++i) {
        roll -= kKindDefs[i].weight;
        if (roll < 0) return static_cast<CrateKind>(i);
    }
    return CrateKind::Wood;
}

bool Crates::placeFree(Vector3 point, const std::vector<Vector3>& spawns, Vector3 player) const
{
    if (Vector3Distance(point, player) < kMinPlayerDistance) return false;
    for (const Vector3& s : spawns) {
        if (Vector3Distance(point, s) < kMinSpawnDistance) return false;
    }
    for (const Crate& c : crates_) {
        if (c.alive && Vector3Distance(point, c.position) < kMinCrateDistance) return false;
    }
    // resolveCircle ne déplace pas un cercle qui ne touche aucun obstacle.
    const Vector3 resolved = world_.resolveCircle(point, kFreeRadius);
    return Vector3Distance(resolved, point) < 0.01f;
}

void Crates::spawnWave(int count, const Zones& zones, Vector3 player)
{
    const std::vector<Rectangle> areas = zones.openAreas();
    if (areas.empty()) return;
    const std::vector<Vector3> spawns = zones.openSpawnPoints();
    for (int n = 0; n < count; ++n) {
        auto slot = std::find_if(crates_.begin(), crates_.end(), [](const Crate& c) { return !c.alive; });
        if (slot == crates_.end()) return;
        for (int attempt = 0; attempt < kPlacementTries; ++attempt) {
            const Rectangle& area = areas[static_cast<size_t>(GetRandomValue(0, static_cast<int>(areas.size()) - 1))];
            const Vector3 point = {area.x + kAreaMargin + randomUnit() * (area.width - 2.0f * kAreaMargin), 0.0f,
                                   area.y + kAreaMargin + randomUnit() * (area.height - 2.0f * kAreaMargin)};
            if (!placeFree(point, spawns, player)) continue;
            Crate& c = *slot;
            c.kind = randomKind();
            const Kind& kind = kinds_[static_cast<size_t>(c.kind)];
            c.alive = true;
            c.landed = false;
            c.position = point;
            c.yaw = static_cast<float>(GetRandomValue(0, 3) * 90 + GetRandomValue(-12, 12));
            c.drawPosition = World::centeredPosition(*kind.model, point, c.yaw, {kind.scale, kind.scale, kind.scale});
            c.box = World::placedBounds(*kind.model, c.drawPosition, c.yaw, kind.scale);
            c.height = kDropHeight;
            c.fallSpeed = 0.0f;
            c.health = kind.health;
            c.hurt = 0.0f;
            world_.moveBlocker(c.blocker, c.box);
            break;
        }
    }
}

bool Crates::hitAt(Vector3 impact, float damage)
{
    for (Crate& c : crates_) {
        if (!c.alive || !c.landed) continue;
        if (impact.x < c.box.min.x - kHitMargin || impact.x > c.box.max.x + kHitMargin ||
            impact.z < c.box.min.z - kHitMargin || impact.z > c.box.max.z + kHitMargin) {
            continue;
        }
        c.health -= damage;
        c.hurt = kHurtTime;
        return true;
    }
    return false;
}

void Crates::queueBlast(Vector3 center, float radius, float damage)
{
    blasts_.push_back({center, radius, damage});
}

std::vector<Crates::Broken> Crates::update(float dt, EventQueue& events)
{
    for (const Blast& blast : blasts_) {
        for (Crate& c : crates_) {
            if (!c.alive || !c.landed) continue;
            const float d = Vector2Distance({c.position.x, c.position.z}, {blast.center.x, blast.center.z});
            if (d <= blast.radius + kFreeRadius) {
                c.health -= blast.damage;
                c.hurt = kHurtTime;
            }
        }
    }
    blasts_.clear();

    std::vector<Broken> broken;
    for (Crate& c : crates_) {
        if (!c.alive) continue;
        c.hurt = std::max(0.0f, c.hurt - dt);
        if (!c.landed) {
            c.fallSpeed += kGravity * dt;
            c.height -= c.fallSpeed * dt;
            if (c.height <= 0.0f) {
                c.height = 0.0f;
                c.landed = true;
                world_.setBlockerActive(c.blocker, true);
                events.push_back({EventType::CrateLanded, c.position});
            }
            continue;
        }
        if (c.health > 0.0f) continue;
        c.alive = false;
        world_.setBlockerActive(c.blocker, false);
        events.push_back({EventType::CrateBroken, c.position, static_cast<float>(c.kind)});
        broken.push_back({c.position, c.kind});
    }
    return broken;
}

void Crates::draw() const
{
    for (const Crate& c : crates_) {
        if (!c.alive) continue;
        const Kind& kind = kinds_[static_cast<size_t>(c.kind)];
        Vector3 at = {c.drawPosition.x, c.height, c.drawPosition.z};
        if (c.hurt > 0.0f) {
            const float shake = kHurtShake * c.hurt / kHurtTime;
            at.x += shake * static_cast<float>(GetRandomValue(-1, 1));
            at.z += shake * static_cast<float>(GetRandomValue(-1, 1));
        }
        const float s = kind.scale * kKenneyScale;
        DrawModelEx(*kind.model, at, {0.0f, 1.0f, 0.0f}, c.yaw, {s, s, s}, c.hurt > 0.0f ? kHurtTint : kind.tint);
    }
}
