#include "game/Turrets.hpp"

#include "game/Scale.hpp"
#include "game/World.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr const char* kBaseModel = "tower/tower-square-bottom-a.glb";
constexpr const char* kHeadModel = "tower/weapon-ballista.glb";
constexpr float kHeadSize = 0.9f;      // la tête tient dans 0,9 m

// Arbalète : portée 12 m, un carreau toutes les 1,2 s, 45 dégâts, perce 2 morts.
constexpr float kRange = 12.0f;
constexpr float kInterval = 1.2f;
constexpr float kDamage = 45.0f;
constexpr int kPierce = 2;
constexpr float kTurnSpeed = 8.0f;
constexpr float kAimTolerance = 0.15f; // radians : tire quand l'arme est presque alignée
constexpr float kMuzzleHeight = 0.25f; // au-dessus du socle
constexpr float kBoltLife = 0.18f;
constexpr Color kBoltColor = {235, 210, 160, 255};
constexpr int kMaxTurrets = 6;

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
    : head_(&world.model(kHeadModel))
    , headScale_(fitScale(*head_, kHeadSize))
{
    // Même mise à l'échelle que Structures : l'emprise d'une case mesure 1 m.
    const Model& base = world.model(kBaseModel);
    baseTop_ = GetModelBoundingBox(base).max.y * fitScale(base, 1.0f);
}

std::vector<Turrets::Hit> Turrets::update(float dt, Structures& structures, std::vector<Enemy>& enemies,
                                          const World& world, EventQueue& events)
{
    for (Bolt& b : bolts_) b.age += dt;
    bolts_.erase(std::remove_if(bolts_.begin(), bolts_.end(), [](const Bolt& b) { return b.age >= kBoltLife; }),
                 bolts_.end());

    std::vector<Hit> hits;
    for (int id : structures.ids()) {
        if (structures.kind(id) != StructureKind::Crossbow) continue;
        TurretState& turret = structures.turret(id);
        turret.cooldown = std::max(0.0f, turret.cooldown - dt);
        const Vector3 base = structures.center(id);
        const Vector3 muzzle = {base.x, baseTop_ + kMuzzleHeight, base.z};

        // Le mort le plus proche, à portée et en vue.
        Enemy* target = nullptr;
        float best = kRange;
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
        turret.cooldown = kInterval;

        // Le carreau file tout droit jusqu'à la portée ou au décor, et perce kPierce morts.
        const Vector3 direction = Vector3Normalize({aim.x - muzzle.x, 0.0f, aim.z - muzzle.z});
        Vector3 end = Vector3Add(muzzle, Vector3Scale(direction, kRange));
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
        const size_t count = std::min(along.size(), static_cast<size_t>(kPierce + 1));
        for (size_t i = 0; i < count; ++i) hits.push_back({along[i].second, kDamage, direction});
        if (count > static_cast<size_t>(kPierce)) end = Vector3Add(muzzle, Vector3Scale(direction, along[count - 1].first));
        bolts_.push_back({muzzle, {end.x, muzzle.y, end.z}, 0.0f});
        events.push_back({EventType::TurretFired, muzzle, static_cast<float>(StructureKind::Crossbow), direction});
    }
    return hits;
}

void Turrets::draw(const Structures& structures) const
{
    for (int id : structures.ids()) {
        if (structures.kind(id) != StructureKind::Crossbow) continue;
        const Vector3 base = structures.center(id);
        const float yaw = structures.turret(id).yaw;
        const Vector3 at = World::centeredPosition(*head_, {base.x, baseTop_, base.z}, yaw * RAD2DEG,
                                                   Vector3Scale({1.0f, 1.0f, 1.0f}, headScale_ / kKenneyScale));
        DrawModelEx(*head_, at, {0.0f, 1.0f, 0.0f}, yaw * RAD2DEG, {headScale_, headScale_, headScale_}, WHITE);
    }
    for (const Bolt& b : bolts_) {
        // Le carreau : un trait qui file et s'estompe.
        const float t = b.age / kBoltLife;
        const Vector3 tip = Vector3Lerp(b.from, b.to, std::min(1.0f, t * 1.6f));
        const Vector3 tail = Vector3Lerp(b.from, tip, 0.6f);
        DrawCylinderEx(tail, tip, 0.03f, 0.03f, 6, Fade(kBoltColor, 1.0f - t));
    }
}
