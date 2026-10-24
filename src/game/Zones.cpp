#include "game/Zones.hpp"

#include "core/Fonts.hpp"
#include "game/Economy.hpp"
#include "game/Scale.hpp"
#include "game/World.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <queue>

namespace {
// Une grille = un portail de pierre (iron-fence-border-gate, élargi pour un passage de 3 m)
// fermé par un battant de barreaux (iron-fence) qui s'enfonce dans le sol à l'ouverture.
constexpr const char* kFrameModel = "graveyard/iron-fence-border-gate.glb";
constexpr const char* kLeafModel = "graveyard/iron-fence.glb";
constexpr Vector3 kFrameScale = {2.7f, 1.5f, 2.0f}; // multiplicateurs de kKenneyScale
constexpr Vector3 kLeafScale = {1.5f, 1.0f, 1.0f};
constexpr float kOpeningHalfWidth = 1.5f;
constexpr float kPostCenter = 2.1f;    // distance du centre aux poteaux, le long de la grille
constexpr float kPostHalfWidth = 0.6f;
constexpr float kGateThickness = 0.6f;
constexpr float kPostThickness = 0.9f;
constexpr float kGateHeight = 2.0f;
constexpr float kInteractDistance = 2.0f;
constexpr float kSignDistance = 12.0f;
// Les panneaux lointains ne s'affichent pas sous le titre du HUD ni au ras des bords de l'écran.
constexpr float kSignTopMargin = 110.0f;
constexpr float kSignEdgeMargin = 40.0f;
// À l'ouverture, la grille tremble et s'enfonce dans le sol.
constexpr float kOpenDuration = 1.4f;
constexpr float kSinkDepth = 1.9f;
constexpr float kShakeAmplitude = 0.04f;
constexpr float kShakeFrequency = 55.0f;
// Point de passage visé au-delà de la grille, pour la franchir franchement.
constexpr float kWaypointOvershoot = 3.0f;
constexpr float kSideProbe = 1.5f;

constexpr Color kPromptColor = {255, 215, 120, 255};
constexpr Color kPromptDeniedColor = {235, 90, 80, 255};
constexpr Color kSignColor = {230, 200, 120, 200};
constexpr Color kPromptShadow = {0, 0, 0, 200};

float smoothstep(float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// Boîte alignée de demi-largeur halfWidth le long de axis et d'épaisseur thickness.
BoundingBox gateBox(Vector3 center, Vector3 axis, Vector3 normal, float halfWidth, float thickness)
{
    const Vector3 half = Vector3Add(Vector3Scale(axis, halfWidth), Vector3Scale(normal, thickness * 0.5f));
    const Vector3 a = Vector3Subtract(center, half);
    const Vector3 b = Vector3Add(center, half);
    return {{std::min(a.x, b.x), 0.0f, std::min(a.z, b.z)}, {std::max(a.x, b.x), kGateHeight, std::max(a.z, b.z)}};
}

void drawCenteredText(const char* text, Vector2 at, int size, Color color)
{
    const int x = static_cast<int>(at.x) - fonts::measureText(text, size) / 2;
    const int y = static_cast<int>(at.y);
    fonts::drawText(text, x + 2, y + 2, size, kPromptShadow);
    fonts::drawText(text, x, y, size, color);
}
}

Zones::Zones(const MapData& map, World& world)
    : world_(world)
    , frameModel_(&world.model(kFrameModel))
    , leafModel_(&world.model(kLeafModel))
{
    for (const ZoneDef& def : map.zones) zones_.push_back({def, def.price == 0, {}});
    for (const SpawnDef& spawn : map.spawns) {
        const int zone = zoneIndex(spawn.zone);
        if (zone >= 0) zones_[static_cast<size_t>(zone)].spawns.push_back(spawn.position);
    }

    for (const GateDef& def : map.gates) {
        Gate gate;
        gate.zone = zoneIndex(def.zone);
        if (gate.zone < 0) continue;
        gate.center = def.position;
        gate.yaw = def.yaw;
        const float yaw = def.yaw * DEG2RAD;
        gate.axis = {std::cos(yaw), 0.0f, -std::sin(yaw)};
        gate.normal = {gate.axis.z, 0.0f, -gate.axis.x};
        if (zoneAt(Vector3Add(gate.center, Vector3Scale(gate.normal, kSideProbe))) != gate.zone) {
            gate.normal = Vector3Negate(gate.normal);
        }
        gate.otherZone = zoneAt(Vector3Subtract(gate.center, Vector3Scale(gate.normal, kSideProbe)));

        gate.framePosition = World::centeredPosition(*frameModel_, gate.center, gate.yaw, kFrameScale);
        gate.leafPosition = World::centeredPosition(*leafModel_, gate.center, gate.yaw, kLeafScale);

        // Poteaux du portail : toujours solides. Battant : solide tant que la grille est fermée.
        for (float side = -1.0f; side <= 1.0f; side += 2.0f) {
            const Vector3 post = Vector3Add(gate.center, Vector3Scale(gate.axis, side * kPostCenter));
            world_.addBlocker(gateBox(post, gate.axis, gate.normal, kPostHalfWidth, kPostThickness));
        }
        gate.blocker = world_.addBlocker(gateBox(gate.center, gate.axis, gate.normal, kOpeningHalfWidth, kGateThickness));
        gates_.push_back(gate);
    }
    buildRoutes();
    reset();
}

int Zones::zoneIndex(const std::string& id) const
{
    for (size_t i = 0; i < zones_.size(); ++i) {
        if (zones_[i].def.id == id) return static_cast<int>(i);
    }
    TraceLog(LOG_WARNING, "ZONES: zone inconnue '%s'", id.c_str());
    return -1;
}

void Zones::buildRoutes()
{
    // Parcours en largeur depuis chaque zone : on retient la première grille du chemin.
    const size_t count = zones_.size();
    routes_.assign(count, std::vector<int>(count, -1));
    for (size_t start = 0; start < count; ++start) {
        std::vector<bool> seen(count, false);
        std::queue<size_t> frontier;
        seen[start] = true;
        frontier.push(start);
        while (!frontier.empty()) {
            const size_t current = frontier.front();
            frontier.pop();
            for (size_t g = 0; g < gates_.size(); ++g) {
                const Gate& gate = gates_[g];
                if (gate.otherZone < 0) continue;
                int next = -1;
                if (static_cast<size_t>(gate.zone) == current) next = gate.otherZone;
                else if (static_cast<size_t>(gate.otherZone) == current) next = gate.zone;
                if (next < 0 || seen[static_cast<size_t>(next)]) continue;
                seen[static_cast<size_t>(next)] = true;
                routes_[start][static_cast<size_t>(next)] =
                    current == start ? static_cast<int>(g) : routes_[start][current];
                frontier.push(static_cast<size_t>(next));
            }
        }
    }
}

void Zones::reset()
{
    for (Zone& zone : zones_) zone.open = zone.def.price == 0;
    for (Gate& gate : gates_) {
        gate.open = false;
        gate.openTime = 0.0f;
        world_.setBlockerActive(gate.blocker, true);
    }
}

void Zones::update(float dt)
{
    for (Gate& gate : gates_) {
        if (gate.open) gate.openTime += dt;
    }
}

int Zones::zoneAt(Vector3 position) const
{
    for (size_t i = 0; i < zones_.size(); ++i) {
        const ZoneDef& z = zones_[i].def;
        if (position.x >= z.min.x && position.x <= z.max.x && position.z >= z.min.y && position.z <= z.max.y) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int Zones::nearestClosedGate(Vector3 player) const
{
    int best = -1;
    float bestDistance = kInteractDistance;
    for (size_t i = 0; i < gates_.size(); ++i) {
        const Gate& gate = gates_[i];
        if (gate.open) continue;
        // Distance au segment du battant (3 m de large).
        const Vector3 offset = Vector3Subtract({player.x, 0.0f, player.z}, gate.center);
        const float along = std::clamp(Vector3DotProduct(offset, gate.axis), -kOpeningHalfWidth, kOpeningHalfWidth);
        const float distance = Vector3Length(Vector3Subtract(offset, Vector3Scale(gate.axis, along)));
        if (distance < bestDistance) {
            bestDistance = distance;
            best = static_cast<int>(i);
        }
    }
    return best;
}

bool Zones::tryOpenNearest(Vector3 player, Economy& economy, EventQueue& events)
{
    const int index = nearestClosedGate(player);
    if (index < 0) return false;
    Gate& gate = gates_[static_cast<size_t>(index)];
    if (!economy.spend(zones_[static_cast<size_t>(gate.zone)].def.price)) return false;

    gate.open = true;
    gate.openTime = 0.0f;
    world_.setBlockerActive(gate.blocker, false);
    zones_[static_cast<size_t>(gate.zone)].open = true;
    events.push_back({EventType::GateOpened, gate.center, static_cast<float>(gate.zone)});
    return true;
}

std::vector<Vector3> Zones::openSpawnPoints() const
{
    std::vector<Vector3> points;
    for (const Zone& zone : zones_) {
        if (zone.open) points.insert(points.end(), zone.spawns.begin(), zone.spawns.end());
    }
    return points;
}

Vector3 Zones::steer(Vector3 from, Vector3 target) const
{
    const int a = zoneAt(from);
    const int b = zoneAt(target);
    if (a < 0 || b < 0 || a == b) return target;
    const int route = routes_[static_cast<size_t>(a)][static_cast<size_t>(b)];
    if (route < 0) return target;
    const Gate& gate = gates_[static_cast<size_t>(route)];
    if (!gate.open) return target;
    // La normale va de otherZone vers zone : on vise un point au-delà de la grille.
    const float side = a == gate.otherZone ? 1.0f : -1.0f;
    return Vector3Add(gate.center, Vector3Scale(gate.normal, side * kWaypointOvershoot));
}

void Zones::draw() const
{
    const Vector3 frameScale = Vector3Scale(kFrameScale, kKenneyScale);
    const Vector3 leafScale = Vector3Scale(kLeafScale, kKenneyScale);
    for (const Gate& gate : gates_) {
        DrawModelEx(*frameModel_, gate.framePosition, {0.0f, 1.0f, 0.0f}, gate.yaw, frameScale, WHITE);
        if (gate.open && gate.openTime >= kOpenDuration) continue;
        // À l'ouverture, le battant tremble puis s'enfonce dans le sol.
        const float t = gate.open ? gate.openTime / kOpenDuration : 0.0f;
        const float shake = gate.open ? std::sin(gate.openTime * kShakeFrequency) * kShakeAmplitude : 0.0f;
        Vector3 position = Vector3Add(gate.leafPosition, Vector3Scale(gate.axis, shake));
        position.y -= kSinkDepth * smoothstep(t);
        DrawModelEx(*leafModel_, position, {0.0f, 1.0f, 0.0f}, gate.yaw, leafScale, WHITE);
    }
}

void Zones::drawPrompt(const Camera3D& camera, Vector3 player, int gold) const
{
    const int nearest = nearestClosedGate(player);
    for (size_t i = 0; i < gates_.size(); ++i) {
        const Gate& gate = gates_[i];
        if (gate.open || Vector3Distance(gate.center, player) > kSignDistance) continue;
        const ZoneDef& zone = zones_[static_cast<size_t>(gate.zone)].def;
        const Vector2 screen = GetWorldToScreen(Vector3Add(gate.center, {0.0f, kGateHeight + 0.4f, 0.0f}), camera);
        if (static_cast<int>(i) != nearest) {
            const bool onScreen = screen.y > kSignTopMargin && screen.y < GetScreenHeight() - kSignEdgeMargin &&
                                  screen.x > kSignEdgeMargin && screen.x < GetScreenWidth() - kSignEdgeMargin;
            if (onScreen) {
                drawCenteredText(TextFormat("%s - %d", zone.name.c_str(), zone.price), screen, 18, kSignColor);
            }
            continue;
        }
        const bool affordable = gold >= zone.price;
        drawCenteredText(TextFormat("E : ouvrir la grille (%d)", zone.price), {screen.x, screen.y - 30.0f}, 24,
                         affordable ? kPromptColor : kPromptDeniedColor);
        drawCenteredText(affordable ? zone.name.c_str() : "Pas assez d'or", screen, 18,
                         affordable ? kSignColor : kPromptDeniedColor);
    }
}
