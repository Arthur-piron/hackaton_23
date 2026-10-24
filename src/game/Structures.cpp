#include "game/Structures.hpp"

#include "game/Scale.hpp"
#include "game/World.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
// Catalogue, dans l'ordre de StructureKind. Coûts : bois / ferraille / or.
const StructureInfo kCatalog[] = {
    {"Palissade", BuildCategory::Defenses, 2, 1, 4, 0, 0, 150.0f, kBlocksAll, "graveyard/fence.glb"},
    {"Barricade", BuildCategory::Defenses, 2, 1, 6, 2, 0, 300.0f, kBlocksPlayer | kBlocksEnemies,
     "survival/fence-fortified.glb"},
    {"Porte", BuildCategory::Defenses, 2, 1, 5, 3, 0, 250.0f, kBlocksEnemies | kBlocksBullets,
     "graveyard/fence-gate.glb"},
};
static_assert(sizeof(kCatalog) / sizeof(kCatalog[0]) == static_cast<size_t>(StructureKind::Count),
              "une entrée de catalogue par structure");

constexpr float kHeight = 1.6f;          // hauteur de la collision
constexpr float kThickness = 0.5f;       // épaisseur des murs 2×1, plus fine que la case
constexpr float kDoorOpenDistance = 1.5f;
constexpr float kDoorSpeed = 4.0f;       // ouverture complète en 0,25 s
constexpr float kDoorSink = 1.2f;        // la porte ouverte s'abaisse sous le sol
}

const StructureInfo& Structures::info(StructureKind kind)
{
    return kCatalog[static_cast<size_t>(kind)];
}

BoundingBox Structures::footprint(StructureKind kind, Vector3 center, bool rotated)
{
    const StructureInfo& s = info(kind);
    float halfX = static_cast<float>(s.cellsX) * 0.5f;
    float halfZ = static_cast<float>(s.cellsZ) * 0.5f;
    // Les murs d'une case d'épaisseur ne remplissent pas toute la case.
    if (s.cellsZ == 1) halfZ = std::min(halfZ, kThickness * 0.5f);
    if (rotated) std::swap(halfX, halfZ);
    return {{center.x - halfX, 0.0f, center.z - halfZ}, {center.x + halfX, kHeight, center.z + halfZ}};
}

Structures::Structures(World& world)
    : world_(world)
{
    for (const StructureInfo& s : kCatalog) {
        const Model& model = world_.model(s.model);
        const BoundingBox b = GetModelBoundingBox(model);
        const float longest = std::max(b.max.x - b.min.x, b.max.z - b.min.z);
        models_.push_back(&model);
        modelScales_.push_back(static_cast<float>(std::max(s.cellsX, s.cellsZ)) / std::max(longest, 0.01f));
    }
}

void Structures::clear()
{
    for (Item& item : items_) {
        if (item.alive) world_.setBlockerActive(item.blocker, false);
        item.alive = false;
    }
}

int Structures::place(StructureKind kind, Vector3 center, bool rotated)
{
    auto slot = std::find_if(items_.begin(), items_.end(), [](const Item& i) { return !i.alive; });
    if (slot == items_.end()) {
        items_.push_back(Item{});
        slot = items_.end() - 1;
    }
    Item& item = *slot;
    item.kind = kind;
    item.alive = true;
    item.center = center;
    item.rotated = rotated;
    item.health = info(kind).health;
    item.open = 0.0f;
    const BoundingBox box = footprint(kind, center, rotated);
    if (item.blocker < 0) {
        item.blocker = world_.addBlocker(box, info(kind).mask);
    } else {
        world_.moveBlocker(item.blocker, box);
        world_.setBlockerMask(item.blocker, info(kind).mask);
        world_.setBlockerActive(item.blocker, true);
    }
    return static_cast<int>(slot - items_.begin());
}

void Structures::remove(int id)
{
    if (!exists(id)) return;
    Item& item = items_[static_cast<size_t>(id)];
    item.alive = false;
    world_.setBlockerActive(item.blocker, false);
}

int Structures::at(Vector3 point) const
{
    for (size_t i = 0; i < items_.size(); ++i) {
        const Item& item = items_[i];
        if (!item.alive) continue;
        const BoundingBox b = bounds(static_cast<int>(i));
        if (point.x >= b.min.x && point.x <= b.max.x && point.z >= b.min.z && point.z <= b.max.z) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool Structures::exists(int id) const
{
    return id >= 0 && id < static_cast<int>(items_.size()) && items_[static_cast<size_t>(id)].alive;
}

float Structures::healthFraction(int id) const
{
    const Item& item = items_[static_cast<size_t>(id)];
    return item.health / info(item.kind).health;
}

BoundingBox Structures::bounds(int id) const
{
    const Item& item = items_[static_cast<size_t>(id)];
    return footprint(item.kind, item.center, item.rotated);
}

void Structures::update(float dt, Vector3 player)
{
    for (Item& item : items_) {
        if (!item.alive || item.kind != StructureKind::Door) continue;
        const bool near = Vector2Distance({player.x, player.z}, {item.center.x, item.center.z}) < kDoorOpenDistance;
        item.open = std::clamp(item.open + (near ? kDoorSpeed : -kDoorSpeed) * dt, 0.0f, 1.0f);
    }
}

void Structures::drawModel(StructureKind kind, Vector3 center, bool rotated, Color tint) const
{
    const Model& model = *models_[static_cast<size_t>(kind)];
    const float scale = modelScales_[static_cast<size_t>(kind)];
    const float yaw = rotated ? 90.0f : 0.0f;
    // World::centeredPosition attend des multiplicateurs de kKenneyScale.
    const float relative = scale / kKenneyScale;
    const Vector3 at = World::centeredPosition(model, center, yaw, {relative, relative, relative});
    DrawModelEx(model, at, {0.0f, 1.0f, 0.0f}, yaw, {scale, scale, scale}, tint);
}

void Structures::draw() const
{
    for (const Item& item : items_) {
        if (!item.alive) continue;
        Vector3 center = item.center;
        center.y -= kDoorSink * item.open;
        drawModel(item.kind, center, item.rotated, WHITE);
    }
}
