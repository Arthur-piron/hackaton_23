#pragma once

#include "core/AnimatedModel.hpp"
#include "raylib.h"

class World;

// Le gardien du cimetière : se déplace au clavier et regarde vers la souris.
class Player {
public:
    static constexpr float kRadius = 0.4f;

    explicit Player(const AnimatedModel& model);

    void reset(Vector3 spawn);
    // aimPoint : point du sol visé par la souris.
    void update(float dt, const World& world, Vector3 aimPoint);
    void draw() const;

    Vector3 position() const { return position_; }
    float yaw() const { return yaw_; }
    // Direction horizontale du regard (normalisée).
    Vector3 forward() const;
    bool isSprinting() const { return sprinting_; }

private:
    void updateAnimation(float dt, float speed);

    const AnimatedModel& model_;
    AnimationState anim_;
    int clipIdle_ = -1;
    int clipWalk_ = -1;
    int clipSprint_ = -1;

    Vector3 position_ = {0.0f, 0.0f, 0.0f};
    Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
    float yaw_ = 0.0f;
    bool sprinting_ = false;
};
