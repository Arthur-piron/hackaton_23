#pragma once

#include "core/AnimatedModel.hpp"
#include "game/Events.hpp"
#include "raylib.h"

class World;

// Le gardien du cimetière : se déplace au clavier, regarde vers la souris,
// tient son arme en main droite.
class Player {
public:
    static constexpr float kRadius = 0.4f;

    explicit Player(const AnimatedModel& model);

    void reset(Vector3 spawn);
    // aimPoint : point du sol visé ; move : direction voulue (normalisée ou nulle).
    void update(float dt, const World& world, Vector3 aimPoint, Vector3 move, bool sprint);
    // Renvoie vrai si ce coup tue le joueur.
    bool takeDamage(float amount, Vector3 from, EventQueue& events);
    void heal(float amount);
    void applyRecoil(Vector3 impulse);
    void notifyShot();
    // weapon : modèle de l'arme tenue (peut être nul).
    void draw(const Model* weapon) const;

    Vector3 position() const { return position_; }
    float yaw() const { return yaw_; }
    Vector3 forward() const;
    // Bout du canon de l'arme tenue.
    Vector3 muzzle() const;
    float health() const { return health_; }
    float maxHealth() const { return maxHealth_; }
    float stamina() const { return stamina_; }
    bool isDead() const { return dead_; }
    // Vrai quand l'animation de mort est terminée.
    bool deathFinished() const;
    // 1 juste après un coup, puis décroît vers 0 (flash rouge de l'écran).
    float hurtFlash() const { return hurtFlash_; }

private:
    void updateAnimation(float dt, float speed);
    Matrix bodyTransform() const;

    const AnimatedModel& model_;
    AnimationState anim_;
    int clipIdle_;
    int clipWalk_;
    int clipSprint_;
    int clipDie_;
    int clipHold_;
    int clipHoldShoot_;
    int armRight_;
    unsigned armsMask_;

    Vector3 position_ = {0.0f, 0.0f, 0.0f};
    Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
    Vector3 recoil_ = {0.0f, 0.0f, 0.0f};
    Vector3 hand_ = {0.0f, 1.0f, 0.0f};
    float yaw_ = 0.0f;
    bool sprinting_ = false;
    float stamina_ = 1.0f;
    bool exhausted_ = false;

    float maxHealth_ = 100.0f;
    float health_ = 100.0f;
    float invulnerable_ = 0.0f;
    float hurtFlash_ = 0.0f;
    float shootPose_ = 0.0f;
    bool dead_ = false;
};
