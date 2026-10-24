#include "game/Traps.hpp"

#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
// Pics : 15 dégâts et ralenti 30 % pendant 1 s, au plus une fois par mort et par 0,5 s.
constexpr float kSpikeDamage = 15.0f;
constexpr float kSpikeSlow = 0.3f;
constexpr float kSpikeSlowTime = 1.0f;
constexpr float kSpikeCooldown = 0.5f;
// Mine : un mort à moins de 0,9 m la déclenche ; bip 0,4 s puis explosion qui épargne le joueur.
constexpr float kMineTrigger = 0.9f;
constexpr float kMineFuse = 0.4f;
constexpr float kMineRadius = 3.0f;
constexpr float kMineDamage = 120.0f;
constexpr float kMineKnockback = 6.0f;
// Flaque : ralentit de 40 % ; en feu 8 s, 25 dégâts/s et embrasement.
constexpr float kOilSlow = 0.4f;
constexpr float kOilSlowRefresh = 0.2f;
constexpr float kOilBurnTime = 8.0f;
constexpr float kOilBurnDamage = 25.0f;
constexpr float kOilIgniteReach = 1.5f;
// Piège à ours : immobilise 3 s (brute 1,5 s) et 30 dégâts ; se réarme en 8 s.
constexpr float kBearHold = 3.0f;
constexpr float kBearHoldBrute = 1.5f;
constexpr float kBearDamage = 30.0f;
constexpr float kBearRearm = 8.0f;

constexpr Color kOilColor = {10, 8, 6, 200};
constexpr Color kFireCore = {255, 170, 60, 150};
constexpr Color kFireEdge = {255, 90, 30, 90};
constexpr Color kFireLight = {255, 140, 60, 255};
constexpr Color kSteel = {150, 155, 165, 255};
constexpr Color kSteelDark = {90, 92, 100, 255};
constexpr Color kMineLed = {255, 40, 30, 255};
constexpr int kBearTeeth = 10;

bool inside(Vector3 point, const BoundingBox& box, float margin)
{
    return point.x > box.min.x - margin && point.x < box.max.x + margin && point.z > box.min.z - margin &&
           point.z < box.max.z + margin;
}
}

bool Traps::igniteNear(Vector3 player, const Structures& structures)
{
    for (int id : structures.ids()) {
        if (structures.kind(id) != StructureKind::OilSlick || structures.trap(id).active) continue;
        const BoundingBox box = structures.bounds(id);
        if (inside(player, box, kOilIgniteReach)) {
            ignitions_.push_back({structures.center(id), 0.1f});
            return true;
        }
    }
    return false;
}

Traps::Result Traps::update(float dt, Structures& structures, std::vector<Enemy>& enemies, EventQueue& events)
{
    time_ += dt;
    Result result;
    for (int id : structures.ids()) {
        const StructureKind kind = structures.kind(id);
        if (Structures::info(kind).category != BuildCategory::Traps) continue;
        TrapState& trap = structures.trap(id);
        const BoundingBox box = structures.bounds(id);
        const Vector3 center = structures.center(id);
        trap.timer = std::max(0.0f, trap.timer - dt);

        switch (kind) {
        case StructureKind::Spikes:
            for (auto& c : trap.cooldowns) c.second -= dt;
            trap.cooldowns.erase(std::remove_if(trap.cooldowns.begin(), trap.cooldowns.end(),
                                                [](const auto& c) { return c.second <= 0.0f; }),
                                 trap.cooldowns.end());
            for (Enemy& e : enemies) {
                if (!e.isAlive() || e.stats().ethereal || !inside(e.position(), box, e.radius() * 0.5f)) continue;
                const bool cooling = std::any_of(trap.cooldowns.begin(), trap.cooldowns.end(),
                                                 [&](const auto& c) { return c.first == e.id(); });
                if (cooling) continue;
                trap.cooldowns.push_back({e.id(), kSpikeCooldown});
                e.slow(kSpikeSlow, kSpikeSlowTime);
                result.hits.push_back({&e, kSpikeDamage, false});
                events.push_back({EventType::TrapTriggered, center, static_cast<float>(kind)});
                if (--trap.uses <= 0) break;
            }
            break;
        case StructureKind::Mine:
            if (!trap.active) {
                for (const Enemy& e : enemies) {
                    if (e.isAlive() && !e.stats().ethereal &&
                        Vector2Distance({e.position().x, e.position().z}, {center.x, center.z}) < kMineTrigger) {
                        trap.active = true;
                        trap.timer = kMineFuse;
                        events.push_back({EventType::TrapTriggered, center, static_cast<float>(kind)});
                        break;
                    }
                }
            } else if (trap.timer <= 0.0f) {
                result.blasts.push_back({center, kMineRadius, kMineDamage, kMineKnockback});
                trap.uses = 0;
            }
            break;
        case StructureKind::OilSlick: {
            const bool ignited = std::any_of(ignitions_.begin(), ignitions_.end(), [&](const Ignition& i) {
                return inside(i.center, box, i.radius);
            });
            if (!trap.active && ignited) {
                trap.active = true;
                trap.timer = kOilBurnTime;
                events.push_back({EventType::TrapTriggered, center, static_cast<float>(kind)});
            }
            for (Enemy& e : enemies) {
                if (!e.isAlive() || e.stats().ethereal || !inside(e.position(), box, 0.0f)) continue;
                e.slow(kOilSlow, kOilSlowRefresh);
                if (trap.active) result.hits.push_back({&e, kOilBurnDamage * dt, true});
            }
            if (trap.active && trap.timer <= 0.0f) trap.uses = 0;
            break;
        }
        case StructureKind::BearTrap:
            // Mâchoires ouvertes quand le piège est armé.
            trap.active = trap.timer > 0.0f;
            if (trap.active) break;
            for (Enemy& e : enemies) {
                if (!e.isAlive() || e.stats().ethereal || !inside(e.position(), box, 0.0f)) continue;
                e.slow(1.0f, e.type() == EnemyType::Brute ? kBearHoldBrute : kBearHold);
                result.hits.push_back({&e, kBearDamage, false});
                trap.timer = kBearRearm;
                trap.active = true;
                --trap.uses;
                events.push_back({EventType::TrapTriggered, center, static_cast<float>(kind)});
                break;
            }
            break;
        default: break;
        }
        // Piège usé : il disparaît.
        if (trap.uses <= 0 && !(kind == StructureKind::BearTrap && trap.active)) structures.remove(id);
    }
    ignitions_.clear();
    return result;
}

void Traps::draw(const Structures& structures) const
{
    for (int id : structures.ids()) {
        const StructureKind kind = structures.kind(id);
        const TrapState& trap = structures.trap(id);
        const Vector3 c = structures.center(id);
        if (kind == StructureKind::OilSlick) {
            const BoundingBox b = structures.bounds(id);
            const float radius = (b.max.x - b.min.x) * 0.48f;
            DrawCylinder({c.x, 0.01f, c.z}, radius, radius, 0.02f, 24, kOilColor);
            if (trap.active) {
                // Flammes : deux cônes additifs qui ondulent.
                const float flicker = 0.85f + 0.15f * std::sin(time_ * 17.0f + c.x);
                BeginBlendMode(BLEND_ADDITIVE);
                DrawCylinder({c.x, 0.02f, c.z}, radius * 0.2f, radius * 0.95f, 0.9f * flicker, 18, kFireEdge);
                DrawCylinder({c.x, 0.02f, c.z}, 0.05f, radius * 0.7f, 1.4f * flicker, 14, kFireCore);
                EndBlendMode();
            }
        } else if (kind == StructureKind::BearTrap) {
            // Anneau d'acier et dents : couchées quand il est armé, dressées quand il s'est refermé.
            DrawCylinder({c.x, 0.02f, c.z}, 0.42f, 0.42f, 0.04f, 20, kSteelDark);
            DrawCylinder({c.x, 0.03f, c.z}, 0.16f, 0.16f, 0.05f, 12, kSteel);
            const float ring = trap.active ? 0.22f : 0.36f;
            const float height = trap.active ? 0.26f : 0.06f;
            for (int t = 0; t < kBearTeeth; ++t) {
                const float a = 2.0f * PI * static_cast<float>(t) / static_cast<float>(kBearTeeth);
                DrawCube({c.x + std::cos(a) * ring, height * 0.5f + 0.03f, c.z + std::sin(a) * ring}, 0.06f, height,
                         0.06f, kSteel);
            }
        } else if (kind == StructureKind::Mine) {
            // Voyant rouge : clignote lentement, puis très vite pendant la mèche.
            const float period = trap.active ? 0.08f : 0.8f;
            if (std::fmod(time_, period) < period * 0.5f) DrawSphere({c.x, 0.45f, c.z}, 0.06f, kMineLed);
        }
    }
}

std::vector<PointLight> Traps::lights(const Structures& structures) const
{
    std::vector<PointLight> list;
    for (int id : structures.ids()) {
        if (structures.kind(id) != StructureKind::OilSlick || !structures.trap(id).active) continue;
        PointLight light;
        light.position = Vector3Add(structures.center(id), {0.0f, 1.0f, 0.0f});
        light.color = kFireLight;
        light.range = 6.0f;
        light.intensity = 1.6f;
        light.flicker = 0.8f;
        light.halo = 2.0f;
        list.push_back(light);
    }
    return list;
}
