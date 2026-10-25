#pragma once

#include "game/Enemy.hpp"
#include "game/Events.hpp"
#include "raylib.h"

#include <vector>

namespace boss {
// Le Fossoyeur à la vague 5, le Rongé à la vague 10 ; en Sans fin, le cycle recommence toutes les 10 vagues.
constexpr int kBossCycle = 10;
constexpr int kGravediggerWave = 5;
constexpr int kGnawedWave = 10;
// Dégâts aux structures (GAME.md §7.4) : pierre tombale et frappe au sol touchent la structure la plus
// proche de l'impact ; la charge du Rongé détruit les niveaux 1-2 et fait 200 au niveau 3.
constexpr float kImpactStructureDamage = 150.0f;
constexpr int kTrampleDestroyMaxLevel = 2;
constexpr float kTrampleDestroyDamage = 1.0e6f;
constexpr float kTrampleDamage = 200.0f;

// Boss de la vague, s'il y en a un.
bool waveBoss(int wave, EnemyType& type);
// Sous-titre de l'annonce d'arrivée du boss.
const char* tagline(EnemyType type);
}

// Ce que les boss demandent au WaveManager pendant une image.
struct BossUpdate {
    float damage = 0.0f;          // dégâts au joueur
    int summons = 0;              // marcheurs à faire surgir autour de summonCenter
    Vector3 summonCenter = {0.0f, 0.0f, 0.0f};
};

// Comportements propres aux boss, en plus de l'IA commune d'Enemy :
// - le Fossoyeur lance des pierres tombales en cloche, avec une zone d'impact au sol ;
// - le Rongé charge (ligne au sol), frappe le sol (onde de choc) et invoque des marcheurs.
// Possède aussi la barre de vie du boss en bas de l'écran.
class Bosses {
public:
    Bosses();
    ~Bosses();
    Bosses(const Bosses&) = delete;
    Bosses& operator=(const Bosses&) = delete;

    void reset();
    void setShader(const Shader& shader);
    BossUpdate update(float dt, std::vector<Enemy>& enemies, Vector3 playerPosition, EventQueue& events);
    // Pierres en vol, zones d'impact, ligne de charge, ondes de choc et débris (dans BeginMode3D).
    void draw() const;
    // Barre de vie du boss vivant (2D, après la scène).
    void drawHealthBar(const std::vector<Enemy>& enemies) const;

private:
    enum class Move { None, Charge, Slam, Summon };
    struct Tombstone {
        Vector3 from;
        Vector3 to;
        float progress;  // 0 au lancer, 1 à l'impact
        float spin;      // vitesse de rotation (degrés par unité de progression)
    };
    struct Rubble {
        Vector3 position;
        float yaw;
        float age;
    };
    struct Shockwave {
        Vector3 center;
        float radius;
        Color color;
        float age;
    };

    void updateGravedigger(float dt, Enemy& boss, Vector3 playerPosition);
    void throwTombstones(const Enemy& boss, Vector3 playerPosition);
    void updateGnawed(float dt, Enemy& boss, Vector3 playerPosition, EventQueue& events, BossUpdate& result);
    void startMove(Enemy& boss, Vector3 playerPosition);
    void endMove(const Enemy& boss);
    void drawGroundWarning(Vector3 center, float radius, float progress) const;
    void drawChargeLine() const;

    Model tombstoneModel_{};
    Model rubbleModel_{};
    std::vector<Tombstone> flying_;
    std::vector<Rubble> rubble_;
    std::vector<Shockwave> shockwaves_;
    float healthTrail_ = 1.0f;  // la barre rouge foncé rattrape la vie réelle

    // Fossoyeur.
    float throwTimer_ = 0.0f;
    float windup_ = -1.0f;      // < 0 : pas de lancer en préparation

    // Rongé : une action à la fois, d'abord annoncée (telegraph), puis exécutée.
    Move move_ = Move::None;
    bool moveActive_ = false;
    float moveTime_ = 0.0f;
    float moveTimer_ = 0.0f;
    int moveCount_ = 0;
    Vector3 chargeOrigin_ = {0.0f, 0.0f, 0.0f};
    Vector3 chargeDirection_ = {0.0f, 0.0f, 1.0f};
    float chargeLength_ = 0.0f;
    float chargeTravelled_ = 0.0f;
    float trampleTimer_ = 0.0f;
    float blockedTime_ = 0.0f;  // temps passé sans avancer pendant la charge
    bool chargeHit_ = false;
    Vector3 lastPosition_ = {0.0f, 0.0f, 0.0f};
};
