#pragma once

#include "game/Enemy.hpp"
#include "raylib.h"

#include <unordered_map>
#include <vector>

// Effets persistants des cartes : ennemis en feu (dégâts sur la durée, flammes
// visibles) et ondes lumineuses (explosion nécrotique, Soleil de poche, Dernier rempart).
// Les dégâts sont renvoyés à Game, qui les applique et récompense les morts.
class CardEffects {
public:
    static constexpr float kBurnDuration = 3.0f;
    static constexpr float kBurnDamagePerSecond = 6.0f;

    struct BurnTick {
        Enemy* enemy;
        float damage;
    };

    void clear();
    // Enflamme un ennemi (ou relance sa brûlure).
    void ignite(const Enemy& enemy);
    bool isBurning(const Enemy& enemy) const { return burns_.count(enemy.id()) > 0; }
    // Fait brûler les ennemis : renvoie les dégâts à infliger pendant cette image.
    std::vector<BurnTick> update(float dt, std::vector<Enemy>& enemies);
    // Onde visuelle qui s'étend jusqu'à radius puis s'efface.
    void addBlast(Vector3 center, float radius, Color color);
    void draw(const std::vector<Enemy>& enemies) const;

private:
    struct Burn {
        float timeLeft = 0.0f;
        float tick = 0.0f;  // temps avant la prochaine morsure du feu
        bool seen = false;  // l'ennemi existe encore (nettoyage)
    };
    struct Blast {
        Vector3 center;
        float radius;
        Color color;
        float age;
    };

    void drawFlame(const Enemy& enemy) const;

    std::unordered_map<unsigned, Burn> burns_;
    std::vector<Blast> blasts_;
    float time_ = 0.0f;
};
