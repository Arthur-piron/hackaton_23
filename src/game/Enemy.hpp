#pragma once

#include "core/AnimatedModel.hpp"
#include "game/Elites.hpp"
#include "game/Events.hpp"
#include "raylib.h"

enum class EnemyType { Walker, Runner, Skeleton, Brute, Vampire, Ghost, Count };

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
    bool fearsLight;         // fuit la lampe torche et les lumières du décor, attaque de dos
    bool ethereal;           // traverse le décor, flotte, semi-transparent
};

const EnemyStats& enemyStats(EnemyType type);

// Ce qu'un mort perçoit pendant une image (rempli par WaveManager).
struct EnemySenses {
    Vector3 goal;            // où il va : le joueur, ou la grille qui mène à sa zone
    Vector3 player;          // position du joueur (portée d'attaque, contournement)
    Vector3 playerForward;   // vers où le joueur regarde et éclaire
    Vector3 lightEscape;     // direction pour sortir de la lumière (nulle dans le noir)
    bool inFlashlight;       // pris dans le cône de la lampe torche
    float haste;             // multiplicateur de vitesse de la vague (Lune de sang)
};

// Un mort-vivant : poursuit le joueur, attaque au contact, meurt en animation.
class Enemy {
public:
    Enemy(unsigned id, EnemyType type, const AnimatedModel& model, Vector3 position);

    // Renvoie les dégâts infligés au joueur pendant cette image.
    float update(float dt, const EnemySenses& senses, EventQueue& events);
    // Inflige des dégâts ; renvoie vrai si l'ennemi vient de mourir.
    bool hit(float damage, Vector3 direction, float knockback, EventQueue& events);
    // En fait une élite (à l'apparition) : plus de vie, halo, effet du modificateur.
    void makeElite(EliteKind kind);
    // Ralentit de `fraction` (0..1) pendant `duration` secondes ; le plus fort ralentissement l'emporte.
    void slow(float fraction, float duration);
    void push(Vector3 offset);
    void draw() const;
    // Yeux qui luisent dans le noir : position monde de chaque œil.
    void eyePositions(Vector3& left, Vector3& right) const;
    // Éclat des yeux : 1 en vie, s'éteint pendant la mort.
    float eyeGlow() const;

    bool isAlive() const { return state_ != State::Dying; }
    bool isRemovable() const { return removable_; }
    bool isSlowed() const { return slowTimer_ > 0.0f; }
    // Vampire qui fuit la lumière.
    bool isFleeing() const { return fleeing_; }
    Vector3 position() const { return position_; }
    // Centre de la zone touchable (torse).
    Vector3 hitCenter() const;
    float radius() const;
    float health() const { return health_; }
    // Mode Cauchemar : multiplie la vie d'un mort qui vient d'apparaître.
    void scaleHealth(float factor);
    float maxHealth() const { return maxHealth_; }
    EliteKind elite() const { return elite_; }
    bool isElite() const { return elite_ != EliteKind::None; }
    EnemyType type() const { return type_; }
    const EnemyStats& stats() const { return enemyStats(type_); }
    // Identifiant unique (stable même si le tableau des ennemis est réorganisé).
    unsigned id() const { return id_; }

private:
    enum class State { Chasing, Attacking, Dying };

    // Point visé par un vampire : il tourne autour du joueur pour le prendre de dos.
    Vector3 flankPoint(const EnemySenses& senses) const;
    float hoverHeight() const;
    Matrix bodyTransform() const;

    unsigned id_;
    EnemyType type_;
    const AnimatedModel* model_;
    AnimationState anim_;
    State state_ = State::Chasing;

    Vector3 position_;
    Vector3 knockback_ = {0.0f, 0.0f, 0.0f};
    float yaw_ = 0.0f;
    float health_;
    float maxHealth_;
    EliteKind elite_ = EliteKind::None;
    float sinceHit_ = 0.0f;       // secondes depuis le dernier coup (régénération)
    float flash_ = 0.0f;          // éclat blanc quand l'ennemi est touché
    float attackCooldown_ = 0.0f;
    bool attackDealt_ = false;
    float deathTimer_ = 0.0f;
    float groanTimer_;
    float speedJitter_;           // chaque zombie a sa propre allure
    float slowFraction_ = 0.0f;
    float slowTimer_ = 0.0f;
    float age_ = 0.0f;            // secondes depuis l'apparition (flottement des spectres)
    bool fleeing_ = false;
    bool removable_ = false;

    int clipMove_;
    int clipFlee_;
    int clipAttack_;
    int clipDie_;
    int headNode_;
};
