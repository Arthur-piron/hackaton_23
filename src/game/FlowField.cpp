#include "game/FlowField.hpp"

#include "game/Structures.hpp"
#include "game/World.hpp"
#include "game/Zones.hpp"
#include "raymath.h"

#include <cmath>
#include <functional>
#include <limits>
#include <queue>

namespace {
constexpr float kMargin = 2.0f;
// Les obstacles sont élargis du rayon d'un mort : une case dont le centre y tombe est bloquée.
constexpr float kClearance = 0.3f;
constexpr float kWall = -1.0f;
constexpr float kHealthPerCost = 25.0f;
constexpr float kDiagonal = 1.41421356f;
constexpr float kDirectDistance = 2.0f; // en dessous, on va droit sur la cible
constexpr int kLookAhead = 2;
constexpr float kInfinity = std::numeric_limits<float>::max();

constexpr int kNeighbors[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
}

int FlowField::cellIndex(Vector3 point) const
{
    const int x = static_cast<int>(std::floor(point.x - originX_));
    const int z = static_cast<int>(std::floor(point.z - originZ_));
    if (x < 0 || z < 0 || x >= width_ || z >= depth_) return -1;
    return z * width_ + x;
}

Vector3 FlowField::cellCenter(int index) const
{
    return {originX_ + static_cast<float>(index % width_) + 0.5f, 0.0f,
            originZ_ + static_cast<float>(index / width_) + 0.5f};
}

void FlowField::rebuild(const World& world, const Zones& zones, const Structures& structures, Vector3 target)
{
    const Rectangle area = zones.bounds();
    originX_ = std::floor(area.x - kMargin);
    originZ_ = std::floor(area.y - kMargin);
    width_ = static_cast<int>(std::ceil(area.width + 2.0f * kMargin));
    depth_ = static_cast<int>(std::ceil(area.height + 2.0f * kMargin));
    cost_.assign(static_cast<size_t>(width_ * depth_), 1.0f);

    // Décor, grilles fermées, caisses et structures : on remplit les cases couvertes.
    const auto fill = [&](const BoundingBox& box, float margin, float value) {
        const int x0 = std::max(0, static_cast<int>(std::floor(box.min.x - margin - originX_)));
        const int x1 = std::min(width_ - 1, static_cast<int>(std::floor(box.max.x + margin - originX_)));
        const int z0 = std::max(0, static_cast<int>(std::floor(box.min.z - margin - originZ_)));
        const int z1 = std::min(depth_ - 1, static_cast<int>(std::floor(box.max.z + margin - originZ_)));
        for (int z = z0; z <= z1; ++z) {
            for (int x = x0; x <= x1; ++x) {
                const Vector3 c = cellCenter(z * width_ + x);
                if (c.x > box.min.x - margin && c.x < box.max.x + margin && c.z > box.min.z - margin &&
                    c.z < box.max.z + margin) {
                    cost_[static_cast<size_t>(z * width_ + x)] = value;
                }
            }
        }
    };
    for (const BoundingBox& box : world.blockerBoxes(kBlocksEnemies)) fill(box, kClearance, kWall);
    // Les structures restent franchissables… en les cassant.
    for (int id : structures.ids()) {
        if (Structures::info(structures.kind(id)).mask & kBlocksEnemies) {
            fill(structures.bounds(id), 0.0f, 1.0f + structures.health(id) / kHealthPerCost);
        }
    }

    distance_.assign(cost_.size(), kInfinity);
    const int start = cellIndex(target);
    if (start < 0) return;
    using Node = std::pair<float, int>;
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
    distance_[static_cast<size_t>(start)] = 0.0f;
    open.push({0.0f, start});
    while (!open.empty()) {
        const auto [d, index] = open.top();
        open.pop();
        if (d > distance_[static_cast<size_t>(index)]) continue;
        const int x = index % width_;
        const int z = index / width_;
        for (const auto& n : kNeighbors) {
            const int nx = x + n[0];
            const int nz = z + n[1];
            if (nx < 0 || nz < 0 || nx >= width_ || nz >= depth_) continue;
            const int next = nz * width_ + nx;
            const float cost = cost_[static_cast<size_t>(next)];
            if (cost < 0.0f) continue;
            const bool diagonal = n[0] != 0 && n[1] != 0;
            // Pas de diagonale qui coupe le coin d'une case bloquée.
            if (diagonal && (cost_[static_cast<size_t>(z * width_ + nx)] < 0.0f ||
                             cost_[static_cast<size_t>(nz * width_ + x)] < 0.0f)) {
                continue;
            }
            const float candidate = d + cost * (diagonal ? kDiagonal : 1.0f);
            if (candidate < distance_[static_cast<size_t>(next)]) {
                distance_[static_cast<size_t>(next)] = candidate;
                open.push({candidate, next});
            }
        }
    }
}

int FlowField::downhill(int index) const
{
    const int x = index % width_;
    const int z = index / width_;
    int best = -1;
    float bestDistance = distance_[static_cast<size_t>(index)];
    for (const auto& n : kNeighbors) {
        const int nx = x + n[0];
        const int nz = z + n[1];
        if (nx < 0 || nz < 0 || nx >= width_ || nz >= depth_) continue;
        if (n[0] != 0 && n[1] != 0 &&
            (cost_[static_cast<size_t>(z * width_ + nx)] < 0.0f || cost_[static_cast<size_t>(nz * width_ + x)] < 0.0f)) {
            continue;
        }
        const int next = nz * width_ + nx;
        if (distance_[static_cast<size_t>(next)] < bestDistance) {
            bestDistance = distance_[static_cast<size_t>(next)];
            best = next;
        }
    }
    return best;
}

Vector3 FlowField::waypoint(Vector3 from, Vector3 target) const
{
    if (!isBuilt() || Vector2Distance({from.x, from.z}, {target.x, target.z}) < kDirectDistance) return target;
    // Un mort collé à un mur peut être dans une case bloquée : ses voisins le guident quand même.
    const int index = cellIndex(from);
    if (index < 0) return target;
    int ahead = index;
    for (int step = 0; step < kLookAhead; ++step) {
        const int next = downhill(ahead);
        if (next < 0) break;
        ahead = next;
        // Une structure sur le chemin : on va droit dessus pour la casser.
        if (cost_[static_cast<size_t>(next)] > 1.0f) break;
    }
    return ahead == index ? target : cellCenter(ahead);
}
