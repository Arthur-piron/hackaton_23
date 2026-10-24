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
    static constexpr float kBaseHealth = 100.0f;

    explicit Player(const AnimatedModel& model);
    // Change de personnage (modèle partagé, possédé par Game) ; à appeler avant reset().
    void setModel(const AnimatedModel& model);

    void reset(Vector3 spawn);
    // aimPoint : point du sol visé ; move : direction voulue (normalisée ou nulle).
    void update(float dt, const World& world, Vector3 aimPoint, Vector3 move, bool sprint);
    // Renvoie vrai si ce coup tue le joueur.
    bool takeDamage(float amount, Vector3 from, EventQueue& events);
    void heal(float amount);
    // Bonus des cartes.
    void setSpeedMultiplier(float multiplier) { speedMultiplier_ = multiplier; }
    void setStaminaMultipliers(float capacity, float regen)
    {
        staminaCapacity_ = capacity;
        staminaRegen_ = regen;
    }
    void setMaxHealthBonus(float bonus);
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
    // Vrai juste après un coup : les dégâts suivants sont ignorés.
    bool isInvulnerable() const { return invulnerable_ > 0.0f; }
    // Vrai quand l'animation de mort est terminée.
    bool deathFinished() const;
    // 1 juste après un coup, puis décroît vers 0 (flash rouge de l'écran).
    float hurtFlash() const { return hurtFlash_; }

private:
    void updateAnimation(float dt, float speed);
    Matrix bodyTransform() const;

    const AnimatedModel* model_ = nullptr;
    AnimationState anim_;
    int clipIdle_ = -1;
    int clipWalk_ = -1;
    int clipSprint_ = -1;
    int clipDie_ = -1;
    int clipHold_ = -1;
    int clipHoldShoot_ = -1;
    int armRight_ = -1;
    unsigned armsMask_ = 0;

    Vector3 position_ = {0.0f, 0.0f, 0.0f};
    Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
    Vector3 recoil_ = {0.0f, 0.0f, 0.0f};
    Vector3 hand_ = {0.0f, 1.0f, 0.0f};
    float yaw_ = 0.0f;
    bool sprinting_ = false;
    float stamina_ = 1.0f;
    bool exhausted_ = false;

    float speedMultiplier_ = 1.0f;
    float staminaCapacity_ = 1.0f;
    float staminaRegen_ = 1.0f;

    float maxHealth_ = kBaseHealth;
    float health_ = kBaseHealth;
    float invulnerable_ = 0.0f;
    float hurtFlash_ = 0.0f;
    float shootPose_ = 0.0f;
    bool dead_ = false;
};
