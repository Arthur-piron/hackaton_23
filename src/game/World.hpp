#pragma once

#include "raylib.h"

// Le terrain de jeu : pour l'instant un sol carré borné.
class World {
public:
    static constexpr float kHalfSize = 30.0f;

    void draw() const;

    // Ramène une position à l'intérieur des limites du terrain.
    Vector3 clampToBounds(Vector3 position, float radius) const;
};
