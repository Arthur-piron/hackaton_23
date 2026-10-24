#pragma once

#include "fx/PointLight.hpp"
#include "game/Enemy.hpp"
#include "game/Events.hpp"
#include "game/Structures.hpp"
#include "raylib.h"

#include <vector>

// Effet des pièges posés (docs/GAME.md §7.3) : pics, mine, flaque d'huile, piège à ours.
// Les pièges sont des structures que les morts traversent ; Traps lit et met à jour leur
// état et renvoie à Game les coups et les explosions à appliquer (toute mort passe par
// Game::damageEnemy).
class Traps {
public:
    static constexpr int kMaxTraps = 12;

    struct Hit {
        Enemy* enemy;
        float damage;
        bool ignite; // la flaque en feu embrase le mort
    };
    struct Blast {
        Vector3 center;
        float radius;
        float damage;
        float knockback;
    };
    struct Result {
        std::vector<Hit> hits;
        std::vector<Blast> blasts;
    };

    void clear() { ignitions_.clear(); }
    Result update(float dt, Structures& structures, std::vector<Enemy>& enemies, EventQueue& events);
    // Une explosion enflamme les flaques d'huile qu'elle touche (à la prochaine mise à jour).
    void igniteAt(Vector3 center, float radius) { ignitions_.push_back({center, radius}); }
    // E près d'une flaque : on y met le feu. Vrai si une flaque était à portée.
    bool igniteNear(Vector3 player, const Structures& structures);
    // Flaques en feu, piège à ours, voyant de la mine (dans BeginMode3D).
    void draw(const Structures& structures) const;
    // Lueur des flaques en feu, à ajouter à chaque image.
    std::vector<PointLight> lights(const Structures& structures) const;

private:
    struct Ignition {
        Vector3 center;
        float radius;
    };

    std::vector<Ignition> ignitions_;
    float time_ = 0.0f;
};
