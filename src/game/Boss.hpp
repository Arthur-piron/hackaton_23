#pragma once

#include "game/Enemy.hpp"
#include "game/Events.hpp"
#include "raylib.h"

#include <vector>

namespace boss {
// Le Fossoyeur, mini-boss de la vague 5 (puis toutes les 10 vagues en Sans fin).
constexpr int kGravediggerWave = 5;
constexpr int kBossCycle = 10;
// Sa pierre tombale fait 150 aux structures dans sa zone d'impact (GAME.md §7.4).
constexpr float kTombstoneStructureDamage = 150.0f;
}

// Comportements propres aux boss, en plus de l'IA commune d'Enemy : le Fossoyeur
// lance des pierres tombales en cloche, avec une zone d'impact visible au sol.
// Possède aussi la barre de vie du boss en bas de l'écran.
class Bosses {
public:
    Bosses();
    ~Bosses();
    Bosses(const Bosses&) = delete;
    Bosses& operator=(const Bosses&) = delete;

    void reset();
    void setShader(const Shader& shader);
    // Fait agir les boss vivants ; renvoie les dégâts infligés au joueur pendant cette image.
    float update(float dt, std::vector<Enemy>& enemies, Vector3 playerPosition, EventQueue& events);
    // Pierres en vol, zones d'impact et débris (dans BeginMode3D).
    void draw() const;
    // Barre de vie du boss vivant (2D, après la scène).
    void drawHealthBar(const std::vector<Enemy>& enemies) const;

private:
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

    void release(const Enemy& boss, Vector3 playerPosition);

    Model tombstoneModel_{};
    Model rubbleModel_{};
    std::vector<Tombstone> flying_;
    std::vector<Rubble> rubble_;
    float throwTimer_ = 0.0f;
    float windup_ = -1.0f;      // < 0 : pas de lancer en préparation
    float healthTrail_ = 1.0f;  // la barre rouge foncé rattrape la vie réelle
};
