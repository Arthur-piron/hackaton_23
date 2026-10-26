#include "game/Structures.hpp"

#include "game/Scale.hpp"
#include "game/World.hpp"
#include "ui/UiScale.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
// Catalogue, dans l'ordre de StructureKind. Coûts : bois / ferraille / or.
const StructureInfo kCatalog[] = {
    {"Palissade", BuildCategory::Defenses, 2, 1, 4, 0, 0, 150.0f, kBlocksAll, "graveyard/fence.glb", WHITE, 0, false},
    {"Barricade", BuildCategory::Defenses, 2, 1, 6, 2, 0, 300.0f, kBlocksPlayer | kBlocksEnemies,
     "survival/fence-fortified.glb", WHITE, 0, false},
    {"Porte", BuildCategory::Defenses, 2, 1, 5, 3, 0, 250.0f, kBlocksEnemies | kBlocksBullets,
     "graveyard/fence-gate.glb", WHITE, 0, false},
    {"Pics", BuildCategory::Traps, 1, 1, 3, 2, 0, 1.0f, 0u, "tower/detail-crystal.glb", {150, 160, 175, 255}, 25, false},
    {"Mine", BuildCategory::Traps, 1, 1, 0, 4, 50, 1.0f, 0u, "blaster/grenade-b.glb", WHITE, 1, false},
    {"Flaque d'huile", BuildCategory::Traps, 3, 3, 0, 2, 30, 1.0f, 0u, nullptr, WHITE, 1, false},
    {"Piège à ours", BuildCategory::Traps, 1, 1, 2, 3, 0, 1.0f, 0u, nullptr, WHITE, 10, false},
    {"Arbalète", BuildCategory::Turrets, 1, 1, 8, 6, 150, 300.0f, kBlocksAll, "tower/tower-square-bottom-a.glb", WHITE,
     0, false},
    {"Mitrailleuse", BuildCategory::Turrets, 1, 1, 4, 12, 300, 250.0f, kBlocksAll, "tower/tower-square-bottom-b.glb",
     WHITE, 0, true},
    {"Générateur", BuildCategory::Utilities, 2, 1, 6, 10, 200, 400.0f, kBlocksAll, "urban/detail-dumpster-closed.glb",
     WHITE, 0, false},
    {"Lampadaire", BuildCategory::Utilities, 1, 1, 2, 3, 100, 150.0f, kBlocksPlayer | kBlocksEnemies,
     "graveyard/lightpost-single.glb", WHITE, 0, true},
    {"Lance-flammes", BuildCategory::Turrets, 1, 1, 2, 10, 400, 280.0f, kBlocksAll, "tower/tower-round-bottom-a.glb",
     WHITE, 0, true},
    {"Projecteur", BuildCategory::Utilities, 1, 1, 3, 4, 120, 150.0f, kBlocksPlayer | kBlocksEnemies,
     "urban/detail-light-double.glb", WHITE, 0, true},
    {"Établi", BuildCategory::Utilities, 2, 1, 10, 5, 100, 300.0f, kBlocksAll, "survival/workbench.glb", WHITE, 0,
     false},
};
static_assert(sizeof(kCatalog) / sizeof(kCatalog[0]) == static_cast<size_t>(StructureKind::Count),
              "une entrée de catalogue par structure");

constexpr float kHeight = 1.6f;          // hauteur de la collision
constexpr float kThickness = 0.5f;       // épaisseur des murs 2×1, plus fine que la case
constexpr float kDoorOpenDistance = 1.5f;
constexpr float kDoorSpeed = 4.0f;       // ouverture complète en 0,25 s
constexpr float kDoorSink = 1.2f;        // la porte ouverte s'abaisse sous le sol
constexpr float kFlatHeight = 0.03f;     // aperçu au sol des pièges sans modèle
// Améliorations, par niveau (1 à 3) : PV ×2 puis ×3,5 ; dégâts +30 % puis +70 %.
constexpr float kLevelHealth[] = {1.0f, 2.0f, 3.5f};
constexpr float kLevelDamage[] = {1.0f, 1.3f, 1.7f};
constexpr const char* kWorkbenchII = "survival/workbench-anvil.glb";
constexpr float kPipSize = 6.0f;
constexpr Color kPipColor = {255, 205, 90, 255};

// Dégâts par seconde aux structures, dans l'ordre d'EnemyType (GAME.md §7.4).
constexpr float kDamagePerSecond[] = {10.0f, 9.0f, 10.0f, 50.0f, 12.0f, 0.0f, 80.0f, 120.0f};
static_assert(sizeof(kDamagePerSecond) / sizeof(kDamagePerSecond[0]) == static_cast<size_t>(EnemyType::Count),
              "des dégâts par type d'ennemi");

constexpr float kHitEventInterval = 0.5f;
constexpr float kShakeTime = 0.1f;
constexpr float kShakeAmplitude = 0.04f;
constexpr float kCrackedFraction = 0.66f;
constexpr float kRuinedFraction = 0.33f;
constexpr Color kCrackedTint = {204, 204, 204, 255};
constexpr float kRuinTilt = 6.0f;          // degrés
constexpr float kBarDistance = 12.0f;
constexpr float kBarWidth = 46.0f;
constexpr float kBarHeight = 6.0f;
constexpr float kBarLift = 1.9f;
constexpr Color kBarBack = {20, 20, 25, 200};
constexpr Color kBarHealthy = {120, 220, 120, 255};
constexpr Color kBarRuined = {235, 70, 60, 255};
}

const StructureInfo& Structures::info(StructureKind kind)
{
    return kCatalog[static_cast<size_t>(kind)];
}

float Structures::damageMultiplier(int level)
{
    return kLevelDamage[std::clamp(level, 1, kMaxLevel) - 1];
}

float Structures::damagePerSecond(EnemyType type)
{
    return kDamagePerSecond[static_cast<size_t>(type)];
}

BoundingBox Structures::footprint(StructureKind kind, Vector3 center, bool rotated)
{
    const StructureInfo& s = info(kind);
    float halfX = static_cast<float>(s.cellsX) * 0.5f;
    float halfZ = static_cast<float>(s.cellsZ) * 0.5f;
    // Les murs d'une case d'épaisseur ne remplissent pas toute la case.
    if (s.cellsX > 1 && s.cellsZ == 1) halfZ = std::min(halfZ, kThickness * 0.5f);
    if (rotated) std::swap(halfX, halfZ);
    return {{center.x - halfX, 0.0f, center.z - halfZ}, {center.x + halfX, kHeight, center.z + halfZ}};
}

Structures::Structures(World& world)
    : world_(world)
{
    for (const StructureInfo& s : kCatalog) {
        if (!s.model) {
            models_.push_back(nullptr);
            modelScales_.push_back(1.0f);
            continue;
        }
        const Model& model = world_.model(s.model);
        const BoundingBox b = GetModelBoundingBox(model);
        const float longest = std::max(b.max.x - b.min.x, b.max.z - b.min.z);
        models_.push_back(&model);
        modelScales_.push_back(static_cast<float>(std::max(s.cellsX, s.cellsZ)) / std::max(longest, 0.01f));
    }
    workbenchII_ = &world_.model(kWorkbenchII);
    const BoundingBox b = GetModelBoundingBox(*workbenchII_);
    workbenchIIScale_ = 2.0f / std::max(0.01f, std::max(b.max.x - b.min.x, b.max.z - b.min.z));
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
    item.trap = TrapState{};
    item.turret = TurretState{};
    item.trap.uses = info(kind).uses;
    item.level = 1;
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
    return item.health / maxHealth(id);
}

float Structures::maxHealth(int id) const
{
    const Item& item = items_[static_cast<size_t>(id)];
    return info(item.kind).health * kLevelHealth[item.level - 1];
}

void Structures::upgrade(int id)
{
    if (!exists(id) || level(id) >= kMaxLevel) return;
    Item& item = items_[static_cast<size_t>(id)];
    const float before = maxHealth(id);
    ++item.level;
    item.health += maxHealth(id) - before;
    // Pièges : utilisations ×2 au niveau 2, ×3 au niveau 3.
    item.trap.uses += info(item.kind).uses;
}

const Model* Structures::modelOf(const Item& item) const
{
    if (item.kind == StructureKind::Workbench && item.level >= 2) return workbenchII_;
    return models_[static_cast<size_t>(item.kind)];
}

BoundingBox Structures::bounds(int id) const
{
    const Item& item = items_[static_cast<size_t>(id)];
    return footprint(item.kind, item.center, item.rotated);
}

std::vector<int> Structures::ids() const
{
    std::vector<int> list;
    for (size_t i = 0; i < items_.size(); ++i) {
        if (items_[i].alive) list.push_back(static_cast<int>(i));
    }
    return list;
}

int Structures::countIn(BuildCategory category) const
{
    return static_cast<int>(std::count_if(items_.begin(), items_.end(), [&](const Item& i) {
        return i.alive && info(i.kind).category == category;
    }));
}

int Structures::countOf(StructureKind kind) const
{
    return static_cast<int>(
        std::count_if(items_.begin(), items_.end(), [&](const Item& i) { return i.alive && i.kind == kind; }));
}

int Structures::blockingNear(Vector3 point, float reach) const
{
    for (size_t i = 0; i < items_.size(); ++i) {
        const Item& item = items_[i];
        if (!item.alive || (info(item.kind).mask & kBlocksEnemies) == 0) continue;
        const BoundingBox b = bounds(static_cast<int>(i));
        const float dx = point.x - std::clamp(point.x, b.min.x, b.max.x);
        const float dz = point.z - std::clamp(point.z, b.min.z, b.max.z);
        if (dx * dx + dz * dz <= reach * reach) return static_cast<int>(i);
    }
    return -1;
}

bool Structures::damage(int id, float amount, EventQueue& events)
{
    if (!exists(id) || amount <= 0.0f) return false;
    Item& item = items_[static_cast<size_t>(id)];
    item.health -= amount;
    item.shake = kShakeTime;
    if (item.health <= 0.0f) {
        remove(id);
        events.push_back({EventType::StructureDestroyed, item.center, static_cast<float>(item.kind)});
        return true;
    }
    if (item.hitCooldown <= 0.0f) {
        item.hitCooldown = kHitEventInterval;
        events.push_back({EventType::StructureDamaged, item.center, static_cast<float>(item.kind)});
    }
    return false;
}

void Structures::update(float dt, Vector3 player)
{
    for (Item& item : items_) {
        if (!item.alive) continue;
        item.hitCooldown = std::max(0.0f, item.hitCooldown - dt);
        item.shake = std::max(0.0f, item.shake - dt);
        if (item.kind != StructureKind::Door) continue;
        const bool near = Vector2Distance({player.x, player.z}, {item.center.x, item.center.z}) < kDoorOpenDistance;
        item.open = std::clamp(item.open + (near ? kDoorSpeed : -kDoorSpeed) * dt, 0.0f, 1.0f);
    }
}

void Structures::drawModel(StructureKind kind, Vector3 center, bool rotated, Color tint, float tilt) const
{
    if (!models_[static_cast<size_t>(kind)]) {
        // Pas de modèle (flaque, piège à ours) : un aplat de l'emprise, pour l'aperçu.
        const BoundingBox b = footprint(kind, center, rotated);
        DrawPlane({center.x, kFlatHeight, center.z}, {b.max.x - b.min.x, b.max.z - b.min.z}, tint);
        return;
    }
    drawWith(*models_[static_cast<size_t>(kind)], modelScales_[static_cast<size_t>(kind)], center, rotated, tint, tilt);
}

void Structures::drawWith(const Model& model, float scale, Vector3 center, bool rotated, Color tint, float tilt) const
{
    const float yaw = rotated ? 90.0f : 0.0f;
    // World::centeredPosition attend des multiplicateurs de kKenneyScale.
    const float relative = scale / kKenneyScale;
    const Vector3 at = World::centeredPosition(model, center, yaw, {relative, relative, relative});
    // Une structure en ruine penche vers l'avant.
    const Matrix transform = MatrixMultiply(
        MatrixMultiply(MatrixMultiply(MatrixScale(scale, scale, scale), MatrixRotateY(yaw * DEG2RAD)),
                       MatrixRotateX(tilt * DEG2RAD)),
        MatrixTranslate(at.x, at.y, at.z));
    for (int m = 0; m < model.meshCount; ++m) {
        Material material = model.materials[model.meshMaterial[m]];
        const Color base = material.maps[MATERIAL_MAP_DIFFUSE].color;
        material.maps[MATERIAL_MAP_DIFFUSE].color = ColorTint(base, tint);
        DrawMesh(model.meshes[m], material, transform);
        material.maps[MATERIAL_MAP_DIFFUSE].color = base;
    }
}

void Structures::draw() const
{
    for (const Item& item : items_) {
        const Model* model = item.alive ? modelOf(item) : nullptr;
        if (!model) continue;
        Vector3 center = item.center;
        center.y -= kDoorSink * item.open;
        if (item.shake > 0.0f) center.x += kShakeAmplitude * std::sin(item.shake * 120.0f);
        const float fraction = healthFraction(static_cast<int>(&item - items_.data()));
        const Color tint = fraction < kCrackedFraction ? ColorTint(info(item.kind).tint, kCrackedTint) : info(item.kind).tint;
        const float scale = model == workbenchII_ ? workbenchIIScale_ : modelScales_[static_cast<size_t>(item.kind)];
        drawWith(*model, scale, center, item.rotated, tint, fraction < kRuinedFraction ? kRuinTilt : 0.0f);
    }
}

void Structures::drawHealthBars(const Camera3D& camera, Vector3 player) const
{
    for (const Item& item : items_) {
        if (!item.alive || Vector3Distance(item.center, player) > kBarDistance) continue;
        const Vector2 at = ui::fromScreen(GetWorldToScreen(Vector3Add(item.center, {0.0f, kBarLift, 0.0f}), camera));
        // Niveau : un losange doré par niveau au-delà du premier.
        for (int l = 1; l < item.level; ++l) {
            const Vector2 pip = {at.x + (static_cast<float>(l) - static_cast<float>(item.level) * 0.5f) * kPipSize * 2.2f,
                                 at.y - kPipSize * 1.6f};
            DrawPoly(pip, 4, kPipSize, 0.0f, kPipColor);
        }
        const float fraction = healthFraction(static_cast<int>(&item - items_.data()));
        if (fraction >= 1.0f) continue;
        const Rectangle back = {at.x - kBarWidth / 2.0f, at.y, kBarWidth, kBarHeight};
        DrawRectangleRec(back, kBarBack);
        // En ruine, la barre clignote en rouge.
        const bool ruined = fraction < kRuinedFraction;
        const bool blink = ruined && std::fmod(static_cast<float>(GetTime()), 0.4f) < 0.2f;
        DrawRectangleRec({back.x + 1.0f, back.y + 1.0f, (kBarWidth - 2.0f) * fraction, kBarHeight - 2.0f},
                         ruined ? (blink ? WHITE : kBarRuined) : kBarHealthy);
    }
}
