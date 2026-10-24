#include "game/Turrets.hpp"

#include "game/Generator.hpp"
#include "game/Scale.hpp"
#include "game/World.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
// Statistiques d'une tourelle (GAME.md §7.3).
struct TurretStats {
    float range;
    float interval;  // secondes entre deux tirs
    float damage;
    int pierce;      // morts traversés en plus du premier
    float spread;    // degrés
    Color color;     // trait du projectile
};
constexpr TurretStats kCrossbow = {12.0f, 1.2f, 45.0f, 2, 0.0f, {235, 210, 160, 255}};
constexpr TurretStats kMachineGun = {10.0f, 0.125f, 9.0f, 0, 4.0f, {255, 220, 120, 255}};

constexpr float kHeadSize = 0.9f;      // la tête tient dans 0,9 m
constexpr float kTurnSpeed = 8.0f;
constexpr float kAimTolerance = 0.15f; // radians : tire quand l'arme est presque alignée
constexpr float kMuzzleHeight = 0.25f; // au-dessus du socle
constexpr float kBoltLife = 0.18f;
constexpr int kMaxTurrets = 6;

const TurretStats& statsOf(StructureKind kind)
{
    return kind == StructureKind::MachineGun ? kMachineGun : kCrossbow;
}

float angleDelta(float a, float b)
{
    return std::remainder(b - a, 2.0f * PI);
}

float fitScale(const Model& model, float size)
{
    const BoundingBox b = GetModelBoundingBox(model);
    return size / std::max(0.01f, std::max(b.max.x - b.min.x, b.max.z - b.min.z));
}
}

int Turrets::limit(int openZones)
{
    return std::min(kMaxTurrets, 2 + std::max(0, openZones - 1));
}

Turrets::Turrets(World& world)
{
    // Même mise à l'échelle que Structures : l'emprise d'une case mesure 1 m.
    const auto make = [&](const char* base, const char* head) {
        const Model& baseModel = world.model(base);
        const Model& headModel = world.model(head);
        return Look{&headModel, fitScale(headModel, kHeadSize),
                    GetModelBoundingBox(baseModel).max.y * fitScale(baseModel, 1.0f)};
    };
    crossbow_ = make("tower/tower-square-bottom-a.glb", "tower/weapon-ballista.glb");
    machineGun_ = make("tower/tower-square-bottom-b.glb", "tower/weapon-turret.glb");
}

bool Turrets::isTurret(StructureKind kind)
{
    return kind == StructureKind::Crossbow || kind == StructureKind::MachineGun;
}

const Turrets::Look& Turrets::look(StructureKind kind) const
{
    return kind == StructureKind::MachineGun ? machineGun_ : crossbow_;
}

std::vector<Turrets::Hit> Turrets::update(float dt, Structures& structures, std::vector<Enemy>& enemies,
                                          const World& world, const Generator& generator, EventQueue& events)
{
    for (Bolt& b : bolts_) b.age += dt;
    bolts_.erase(std::remove_if(bolts_.begin(), bolts_.end(), [](const Bolt& b) { return b.age >= kBoltLife; }),
                 bolts_.end());

    std::vector<Hit> hits;
    for (int id : structures.ids()) {
        const StructureKind kind = structures.kind(id);
        if (!isTurret(kind)) continue;
        const TurretStats& stats = statsOf(kind);
        TurretState& turret = structures.turret(id);
        turret.cooldown = std::max(0.0f, turret.cooldown - dt);
        const Vector3 base = structures.center(id);
        // Sans courant, une tourelle électrique ne fait rien.
        if (Structures::info(kind).electric && !generator.powered(structures, base)) continue;
        const Vector3 muzzle = {base.x, look(kind).baseTop + kMuzzleHeight, base.z};

        // Le mort le plus proche, à portée et en vue.
        Enemy* target = nullptr;
        float best = stats.range;
        for (Enemy& e : enemies) {
            if (!e.isAlive()) continue;
            const float d = Vector2Distance({e.position().x, e.position().z}, {base.x, base.z});
            Vector3 blocked;
            if (d >= best || world.segmentBlocked(muzzle, e.hitCenter(), blocked)) continue;
            best = d;
            target = &e;
        }
        if (!target) continue;

        const Vector3 aim = target->hitCenter();
        const float wanted = std::atan2(aim.x - base.x, aim.z - base.z);
        turret.yaw += angleDelta(turret.yaw, wanted) * std::min(1.0f, kTurnSpeed * dt);
        if (turret.cooldown > 0.0f || std::fabs(angleDelta(turret.yaw, wanted)) > kAimTolerance) continue;
        turret.cooldown = stats.interval;

        // Le projectile file tout droit (avec sa dispersion) jusqu'à la portée ou au décor.
        const float spread = stats.spread * DEG2RAD * static_cast<float>(GetRandomValue(-100, 100)) / 100.0f;
        const float angle = std::atan2(aim.x - muzzle.x, aim.z - muzzle.z) + spread;
        const Vector3 direction = {std::sin(angle), 0.0f, std::cos(angle)};
        Vector3 end = Vector3Add(muzzle, Vector3Scale(direction, stats.range));
        Vector3 wall;
        if (world.segmentBlocked(muzzle, {end.x, aim.y, end.z}, wall)) end = {wall.x, muzzle.y, wall.z};
        const float length = Vector2Distance({muzzle.x, muzzle.z}, {end.x, end.z});
        std::vector<std::pair<float, Enemy*>> along;
        for (Enemy& e : enemies) {
            if (!e.isAlive()) continue;
            const Vector3 to = Vector3Subtract(e.position(), muzzle);
            const float t = to.x * direction.x + to.z * direction.z;
            if (t < 0.0f || t > length) continue;
            const float side = std::fabs(to.x * direction.z - to.z * direction.x);
            if (side <= e.radius()) along.push_back({t, &e});
        }
        std::sort(along.begin(), along.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        const size_t count = std::min(along.size(), static_cast<size_t>(stats.pierce + 1));
        for (size_t i = 0; i < count; ++i) hits.push_back({along[i].second, stats.damage, direction});
        if (count > static_cast<size_t>(stats.pierce)) {
            end = Vector3Add(muzzle, Vector3Scale(direction, along[count - 1].first));
        }
        bolts_.push_back({muzzle, {end.x, muzzle.y, end.z}, 0.0f, stats.color});
        events.push_back({EventType::TurretFired, muzzle, static_cast<float>(kind), direction});
    }
    return hits;
}

void Turrets::draw(const Structures& structures) const
{
    for (int id : structures.ids()) {
        const StructureKind kind = structures.kind(id);
        if (!isTurret(kind)) continue;
        const Look& l = look(kind);
        const Vector3 base = structures.center(id);
        const float yaw = structures.turret(id).yaw;
        const Vector3 at = World::centeredPosition(*l.head, {base.x, l.baseTop, base.z}, yaw * RAD2DEG,
                                                   Vector3Scale({1.0f, 1.0f, 1.0f}, l.headScale / kKenneyScale));
        DrawModelEx(*l.head, at, {0.0f, 1.0f, 0.0f}, yaw * RAD2DEG, {l.headScale, l.headScale, l.headScale}, WHITE);
    }
    for (const Bolt& b : bolts_) {
        // Le carreau : un trait qui file et s'estompe.
        const float t = b.age / kBoltLife;
        const Vector3 tip = Vector3Lerp(b.from, b.to, std::min(1.0f, t * 1.6f));
        const Vector3 tail = Vector3Lerp(b.from, tip, 0.6f);
        DrawCylinderEx(tail, tip, 0.03f, 0.03f, 6, Fade(b.color, 1.0f - t));
    }
}
