#pragma once

#include "core/AnimatedModel.hpp"
#include "game/Events.hpp"
#include "raylib.h"

enum class EnemyType { Walker, Runner, Skeleton, Brute, Count };

struct EnemyStats {
    const char* name;
    const char* model;       // fichier dans assets/models/graveyard/
    float health;
    float speed;
    float damage;
    float attackRange;
    float attackCooldown;
    float scale;             // multiplicateur de l'échelle Kenney
    Color tint;
    const char* moveClip;    // « walk » ou « sprint »
    float animReferenceSpeed;
    int score;
    float knockbackResistance; // 0 = subit tout le recul, 1 = insensible
};

const EnemyStats& enemyStats(EnemyType type);

// Un mort-vivant : poursuit le joueur, attaque au contact, meurt en animation.
class Enemy {
public:
    Enemy(unsigned id, EnemyType type, const AnimatedModel& model, Vector3 position);

    // Renvoie les dégâts infligés au joueur pendant cette image.
    float update(float dt, Vector3 target, EventQueue& events);
    // Inflige des dégâts ; renvoie vrai si l'ennemi vient de mourir.
    bool hit(float damage, Vector3 direction, float knockback, EventQueue& events);
    void push(Vector3 offset);
    void draw() const;

    bool isAlive() const { return state_ != State::Dying; }
    bool isRemovable() const { return removable_; }
    Vector3 position() const { return position_; }
    // Centre de la zone touchable (torse).
    Vector3 hitCenter() const;
    float radius() const;
    float health() const { return health_; }
    EnemyType type() const { return type_; }
    const EnemyStats& stats() const { return enemyStats(type_); }
    // Identifiant unique (stable même si le tableau des ennemis est réorganisé).
    unsigned id() const { return id_; }

private:
    enum class State { Chasing, Attacking, Dying };

    unsigned id_;
    EnemyType type_;
    const AnimatedModel* model_;
    AnimationState anim_;
    State state_ = State::Chasing;

    Vector3 position_;
    Vector3 knockback_ = {0.0f, 0.0f, 0.0f};
    float yaw_ = 0.0f;
    float health_;
    float flash_ = 0.0f;          // éclat blanc quand l'ennemi est touché
    float attackCooldown_ = 0.0f;
    bool attackDealt_ = false;
    float deathTimer_ = 0.0f;
    float groanTimer_;
    float speedJitter_;           // chaque zombie a sa propre allure
    bool removable_ = false;

    int clipMove_;
    int clipAttack_;
    int clipDie_;
};
