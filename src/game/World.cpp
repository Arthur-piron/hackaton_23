#include "game/World.hpp"

#include <algorithm>

namespace {
constexpr int kTileCount = 20;
constexpr Color kTileDark = {34, 44, 34, 255};
constexpr Color kTileLight = {40, 52, 40, 255};
constexpr Color kBorderColor = {90, 80, 70, 255};
constexpr float kBorderHeight = 1.2f;
constexpr float kBorderThickness = 0.5f;
}

void World::draw() const
{
    const float tile = (kHalfSize * 2.0f) / kTileCount;
    for (int z = 0; z < kTileCount; ++z) {
        for (int x = 0; x < kTileCount; ++x) {
            const Vector3 center = {-kHalfSize + tile * (x + 0.5f), -0.05f, -kHalfSize + tile * (z + 0.5f)};
            DrawCube(center, tile, 0.1f, tile, ((x + z) % 2 == 0) ? kTileDark : kTileLight);
        }
    }

    const float full = kHalfSize * 2.0f + kBorderThickness * 2.0f;
    const float y = kBorderHeight * 0.5f;
    const float edge = kHalfSize + kBorderThickness * 0.5f;
    DrawCube({0.0f, y, -edge}, full, kBorderHeight, kBorderThickness, kBorderColor);
    DrawCube({0.0f, y, edge}, full, kBorderHeight, kBorderThickness, kBorderColor);
    DrawCube({-edge, y, 0.0f}, kBorderThickness, kBorderHeight, full, kBorderColor);
    DrawCube({edge, y, 0.0f}, kBorderThickness, kBorderHeight, full, kBorderColor);
}

Vector3 World::clampToBounds(Vector3 position, float radius) const
{
    const float limit = kHalfSize - radius;
    position.x = std::clamp(position.x, -limit, limit);
    position.z = std::clamp(position.z, -limit, limit);
    return position;
}
