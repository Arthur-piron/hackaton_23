#include "game/BuildMode.hpp"

#include "core/Fonts.hpp"
#include "game/Traps.hpp"
#include "game/Turrets.hpp"
#include "game/World.hpp"
#include "game/Zones.hpp"
#include "raymath.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kGridRadius = 6.0f;
constexpr float kGridHeight = 0.03f;
constexpr Color kGridColor = {255, 255, 255, 40};
constexpr Color kValidTint = {90, 255, 120, 150};
constexpr Color kInvalidTint = {255, 80, 80, 150};
constexpr Color kHoverColor = {255, 255, 255, 200};

// Distances minimales de GAME.md §7.2.
constexpr float kGateClearance = 1.5f;
constexpr float kSpawnClearance = 3.0f;
constexpr float kKeepClearDistance = 2.0f;
constexpr float kPlayerClearance = 0.45f;
constexpr float kSellRefund = 0.5f;

constexpr Color kCostColor = {235, 225, 200, 255};
constexpr Color kRefusalColor = {255, 110, 100, 255};
constexpr Color kShadow = {0, 0, 0, 200};

float distanceToBox(Vector3 point, const BoundingBox& box)
{
    const float dx = point.x - std::clamp(point.x, box.min.x, box.max.x);
    const float dz = point.z - std::clamp(point.z, box.min.z, box.max.z);
    return std::sqrt(dx * dx + dz * dz);
}

void drawCenteredText(const char* text, Vector2 at, int size, Color color)
{
    const int x = static_cast<int>(at.x) - fonts::measureText(text, size) / 2;
    const int y = static_cast<int>(at.y);
    fonts::drawText(text, x + 2, y + 2, size, kShadow);
    fonts::drawText(text, x, y, size, color);
}

void drawFootprint(const BoundingBox& b, Color color)
{
    const float y = kGridHeight * 2.0f;
    DrawLine3D({b.min.x, y, b.min.z}, {b.max.x, y, b.min.z}, color);
    DrawLine3D({b.max.x, y, b.min.z}, {b.max.x, y, b.max.z}, color);
    DrawLine3D({b.max.x, y, b.max.z}, {b.min.x, y, b.max.z}, color);
    DrawLine3D({b.min.x, y, b.max.z}, {b.min.x, y, b.min.z}, color);
}
}

void BuildMode::reset()
{
    active_ = false;
    dragging_ = false;
    hovered_ = -1;
    placedThisPause_.clear();
}

std::vector<StructureKind> BuildMode::items() const
{
    std::vector<StructureKind> list;
    for (int i = 0; i < static_cast<int>(StructureKind::Count); ++i) {
        const StructureKind kind = static_cast<StructureKind>(i);
        if (Structures::info(kind).category == category_) list.push_back(kind);
    }
    return list;
}

void BuildMode::selectCategory(BuildCategory category)
{
    category_ = category;
    const std::vector<StructureKind> list = items();
    if (!list.empty()) selected_ = list.front();
}

Vector3 BuildMode::snap(Vector3 cursor) const
{
    const StructureInfo& s = Structures::info(selected_);
    const int cellsX = rotated_ ? s.cellsZ : s.cellsX;
    const int cellsZ = rotated_ ? s.cellsX : s.cellsZ;
    // Nombre pair de cases : centre sur une ligne de la grille ; impair : au milieu d'une case.
    const auto axis = [](float value, int cells) {
        return cells % 2 == 0 ? std::round(value) : std::floor(value) + 0.5f;
    };
    return {axis(cursor.x, cellsX), 0.0f, axis(cursor.z, cellsZ)};
}

std::string BuildMode::checkPlacement(Vector3 center, const Surroundings& around, const Economy& economy) const
{
    const BoundingBox box = Structures::footprint(selected_, center, rotated_);
    // Chaque case de l'emprise doit être dans une zone ouverte.
    for (float x = std::floor(box.min.x) + 0.5f; x < box.max.x; x += 1.0f) {
        for (float z = std::floor(box.min.z) + 0.5f; z < box.max.z; z += 1.0f) {
            if (!around.zones.isOpen(around.zones.zoneAt({x, 0.0f, z}))) return "Zone fermée";
        }
    }
    if (around.world.areaBlocked(box)) return "Place occupée";
    for (const Vector3& gate : around.zones.gateCenters()) {
        if (distanceToBox(gate, box) < kGateClearance) return "Trop près d'une grille";
    }
    for (const Vector3& spawn : around.zones.openSpawnPoints()) {
        if (distanceToBox(spawn, box) < kSpawnClearance) return "Trop près d'une entrée des morts";
    }
    for (const Vector3& point : around.keepClear) {
        if (distanceToBox(point, box) < kKeepClearDistance) return "Gardez le passage dégagé";
    }
    const StructureInfo& s = Structures::info(selected_);
    // Les pièges se posent même sous le joueur : on les traverse.
    if (s.mask != 0u && distanceToBox(around.player, box) < kPlayerClearance) return "Pas sous tes pieds";
    if (economy.resource(Resource::Wood) < s.wood) return "Pas assez de bois";
    if (economy.resource(Resource::Scrap) < s.scrap) return "Pas assez de ferraille";
    if (economy.gold() < s.gold) return "Pas assez d'or";
    return {};
}

void BuildMode::update(Vector3 cursor, bool intermission, const Surroundings& around, Structures& structures,
                       Economy& economy, EventQueue& events)
{
    if (IsKeyPressed(KEY_B)) {
        if (active_) {
            active_ = false;
        } else if (intermission) {
            active_ = true;
            dragging_ = false;
        } else {
            events.push_back({EventType::BuildDenied, around.player});
        }
    }
    if (active_ && (!intermission || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))) active_ = false;
    if (!active_) return;

    for (int i = 0; i < static_cast<int>(BuildCategory::Count); ++i) {
        if (IsKeyPressed(KEY_ONE + i)) selectCategory(static_cast<BuildCategory>(i));
    }
    const std::vector<StructureKind> list = items();
    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && !list.empty()) {
        const auto current = std::find(list.begin(), list.end(), selected_);
        const int index = static_cast<int>(current - list.begin());
        const int count = static_cast<int>(list.size());
        selected_ = list[static_cast<size_t>((index + (wheel < 0.0f ? 1 : count - 1)) % count)];
    }
    if (IsKeyPressed(KEY_T)) rotated_ = !rotated_;

    cursor_ = cursor;
    ghost_ = snap(cursor);
    hovered_ = structures.at(cursor);
    refusal_ = list.empty() ? std::string("Bientôt disponible") : checkPlacement(ghost_, around, economy);
    if (refusal_.empty() && category_ == BuildCategory::Traps && structures.countIn(BuildCategory::Traps) >= Traps::kMaxTraps) {
        refusal_ = TextFormat("Au plus %d pièges", Traps::kMaxTraps);
    }
    const int turretLimit = Turrets::limit(static_cast<int>(around.zones.openAreas().size()));
    if (refusal_.empty() && category_ == BuildCategory::Turrets && structures.countIn(BuildCategory::Turrets) >= turretLimit) {
        refusal_ = TextFormat("Au plus %d tourelles (ouvre d'autres zones)", turretLimit);
    }

    if (IsKeyPressed(KEY_X)) sell(structures, economy, events);
    if (IsKeyPressed(KEY_BACKSPACE)) undo(structures, economy, events);

    // Clic : une pose ; bouton maintenu : une pose à chaque nouvelle case valide (ligne de palissades).
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        dragging_ = true;
        if (refusal_.empty()) {
            place(structures, economy, events);
        } else {
            events.push_back({EventType::BuildDenied, ghost_});
        }
    } else if (dragging_ && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (refusal_.empty() && Vector3Distance(ghost_, lastPlaced_) > 0.5f) place(structures, economy, events);
    } else {
        dragging_ = false;
    }
}

void BuildMode::place(Structures& structures, Economy& economy, EventQueue& events)
{
    const StructureInfo& s = Structures::info(selected_);
    economy.spendResource(Resource::Wood, s.wood);
    economy.spendResource(Resource::Scrap, s.scrap);
    economy.spend(s.gold);
    const int id = structures.place(selected_, ghost_, rotated_);
    placedThisPause_.push_back({id, selected_});
    lastPlaced_ = ghost_;
    refusal_ = "Place occupée";
    events.push_back({EventType::StructurePlaced, ghost_, static_cast<float>(selected_)});
}

void BuildMode::sell(Structures& structures, Economy& economy, EventQueue& events)
{
    if (!structures.exists(hovered_)) return;
    const StructureInfo& s = Structures::info(structures.kind(hovered_));
    // 50 % de chaque ressource, au prorata des PV restants, arrondi en dessous.
    const float share = kSellRefund * structures.healthFraction(hovered_);
    economy.addResource(Resource::Wood, static_cast<int>(static_cast<float>(s.wood) * share));
    economy.addResource(Resource::Scrap, static_cast<int>(static_cast<float>(s.scrap) * share));
    economy.addGold(static_cast<int>(static_cast<float>(s.gold) * share));
    structures.remove(hovered_);
    placedThisPause_.erase(std::remove_if(placedThisPause_.begin(), placedThisPause_.end(),
                                          [&](const Placement& p) { return p.id == hovered_; }),
                           placedThisPause_.end());
    events.push_back({EventType::StructureRemoved, cursor_});
    hovered_ = -1;
}

void BuildMode::undo(Structures& structures, Economy& economy, EventQueue& events)
{
    while (!placedThisPause_.empty()) {
        const Placement last = placedThisPause_.back();
        placedThisPause_.pop_back();
        if (!structures.exists(last.id) || structures.kind(last.id) != last.kind) continue;
        // Annulation : remboursement complet.
        const StructureInfo& s = Structures::info(last.kind);
        economy.addResource(Resource::Wood, s.wood);
        economy.addResource(Resource::Scrap, s.scrap);
        economy.addGold(s.gold);
        structures.remove(last.id);
        events.push_back({EventType::StructureRemoved, cursor_});
        return;
    }
}

void BuildMode::drawWorld(const Structures& structures) const
{
    if (!active_) return;
    // Lignes de la grille autour du curseur.
    const float cx = std::round(cursor_.x);
    const float cz = std::round(cursor_.z);
    for (float d = -kGridRadius; d <= kGridRadius; d += 1.0f) {
        const float half = std::sqrt(std::max(0.0f, kGridRadius * kGridRadius - d * d));
        DrawLine3D({cx + d, kGridHeight, cz - half}, {cx + d, kGridHeight, cz + half}, kGridColor);
        DrawLine3D({cx - half, kGridHeight, cz + d}, {cx + half, kGridHeight, cz + d}, kGridColor);
    }
    if (structures.exists(hovered_)) {
        drawFootprint(structures.bounds(hovered_), kHoverColor);
    }
    if (items().empty()) return;
    // Fantôme transparent, sans écrire la profondeur.
    const Color tint = refusal_.empty() ? kValidTint : kInvalidTint;
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    structures.drawModel(selected_, ghost_, rotated_, tint);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    drawFootprint(Structures::footprint(selected_, ghost_, rotated_), tint);
}

void BuildMode::drawPrompt(const Camera3D& camera) const
{
    if (!active_ || items().empty()) return;
    const Vector2 screen = GetWorldToScreen(Vector3Add(ghost_, {0.0f, 2.2f, 0.0f}), camera);
    if (!refusal_.empty()) {
        drawCenteredText(refusal_.c_str(), screen, 20, kRefusalColor);
        return;
    }
    const StructureInfo& s = Structures::info(selected_);
    const char* cost = TextFormat("%s : %d bois  %d ferraille  %d or", s.name, s.wood, s.scrap, s.gold);
    drawCenteredText(cost, screen, 20, kCostColor);
}
