#pragma once

#include "game/Enemy.hpp"
#include "game/Events.hpp"
#include "game/Structures.hpp"
#include "raylib.h"

#include <vector>

class World;

// Tourelles posées (docs/GAME.md §7.3). Pour l'instant l'arbalète, sans courant :
// elle vise le mort le plus proche à portée et en vue, et son carreau en perce deux.
// Les coups sont renvoyés à Game (Game::damageEnemy, mort comptée « par une défense »).
class Turrets {
public:
    struct Hit {
        Enemy* enemy;
        float damage;
        Vector3 direction;
    };

    // Nombre de tourelles permis : 2, plus 1 par zone ouverte en plus de la cour, au plus 6.
    static int limit(int openZones);

    // world doit survivre aux tourelles (modèles partagés).
    explicit Turrets(World& world);

    void clear() { bolts_.clear(); }
    std::vector<Hit> update(float dt, Structures& structures, std::vector<Enemy>& enemies, const World& world,
                            EventQueue& events);
    // Têtes des tourelles et carreaux en vol (dans BeginMode3D).
    void draw(const Structures& structures) const;

private:
    struct Bolt {
        Vector3 from;
        Vector3 to;
        float age;
    };

    const Model* head_;
    float headScale_;
    float baseTop_; // hauteur du socle, où se pose la tête
    std::vector<Bolt> bolts_;
};
