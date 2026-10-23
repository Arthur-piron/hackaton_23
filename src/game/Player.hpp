#pragma once

#include "raylib.h"

class World;

// Le gardien du cimetière, contrôlé au clavier.
class Player {
public:
    static constexpr float kRadius = 0.4f;

    void reset(Vector3 spawn);
    void update(float dt, const World& world);
    void draw() const;

    Vector3 position() const { return position_; }

private:
    Vector3 position_ = {0.0f, 0.0f, 0.0f};
    Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
};
