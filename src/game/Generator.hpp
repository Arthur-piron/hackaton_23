#pragma once

#include "fx/PointLight.hpp"
#include "game/Economy.hpp"
#include "game/Enemy.hpp"
#include "game/Events.hpp"
#include "game/Structures.hpp"
#include "raylib.h"

#include <vector>

// Le réseau électrique de la base (docs/GAME.md §7.3) : chaque générateur alimente les
// structures électriques à ≤ 15 m tant qu'il a du carburant. Il en brûle pendant les vagues,
// d'autant plus qu'il alimente de structures ; E près de lui verse un bidon. Gère aussi
// les lampadaires, qui ralentissent les morts dans leur lumière, et les projecteurs, dont
// le cône fixe ralentit et brûle.
class Generator {
public:
    static constexpr float kRange = 15.0f;
    static constexpr float kInteractDistance = 2.0f;

    // Vrai si une structure électrique posée en `point` reçoit du courant.
    bool powered(const Structures& structures, Vector3 point) const;
    // Brûlure d'un projecteur, à appliquer par Game::damageEnemy (lumière, mort « par une défense »).
    struct Burn {
        Enemy* enemy;
        float damage;
    };

    // fighting : une vague est en cours (le générateur ne consomme rien pendant les pauses).
    std::vector<Burn> update(float dt, bool fighting, Structures& structures, std::vector<Enemy>& enemies,
                             EventQueue& events);
    // E à portée d'un générateur : verse un bidon s'il reste de la place. Vrai si un générateur était à portée.
    bool refuelNear(Vector3 player, Structures& structures, Economy& economy, EventQueue& events);

    // Voyant des générateurs et faisceau des projecteurs (dans BeginMode3D).
    void draw(const Structures& structures) const;
    // Réservoir au-dessus du générateur le plus proche, invite à portée (en 2D).
    void drawPrompt(const Camera3D& camera, Vector3 player, const Structures& structures, const Economy& economy) const;
    // Lumière des lampadaires (et lanterne faible sans courant) et des projecteurs, à ajouter à chaque image.
    std::vector<PointLight> lights(const Structures& structures) const;

private:
    float time_ = 0.0f;
};
