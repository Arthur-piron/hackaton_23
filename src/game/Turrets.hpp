#pragma once

#include "game/Enemy.hpp"
#include "game/Events.hpp"
#include "game/Structures.hpp"
#include "raylib.h"

#include <vector>

class Generator;
class World;

// Tourelles posées (docs/GAME.md §7.3) : l'arbalète (sans courant, carreaux qui percent)
// et la mitrailleuse (électrique, rafales dispersées). Elles visent le mort le plus proche
// à portée et en vue.
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
                            const Generator& generator, EventQueue& events);
    // Têtes des tourelles et carreaux en vol (dans BeginMode3D).
    void draw(const Structures& structures) const;

private:
    struct Bolt {
        Vector3 from;
        Vector3 to;
        float age;
        Color color;
    };
    // Modèles d'une sorte de tourelle.
    struct Look {
        const Model* head;
        float headScale;
        float baseTop; // hauteur du socle, où se pose la tête
    };

    static bool isTurret(StructureKind kind);
    const Look& look(StructureKind kind) const;

    Look crossbow_;
    Look machineGun_;
    std::vector<Bolt> bolts_;
};
