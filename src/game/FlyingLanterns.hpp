#pragma once

#include "fx/PointLight.hpp"
#include "game/Enemy.hpp"
#include "raylib.h"

#include <vector>

// Carte « Lanterne volante » : des lanternes tournent autour du joueur, éclairent
// la nuit et brûlent les morts qu'elles frôlent (sauf les squelettes). Une par rang.
class FlyingLanterns {
public:
    struct Burn {
        Enemy* enemy;
        float damage;
    };

    void reset();
    void setCount(int count);
    void update(float dt, Vector3 center);
    // Brûlures de cette image, regroupées en morsures régulières.
    std::vector<Burn> burn(float dt, std::vector<Enemy>& enemies);
    // Lumières à envoyer au shader de nuit (une par lanterne, avec halo).
    std::vector<PointLight> lights() const;
    void draw(const Model& model) const;

private:
    int count_ = 0;
    float angle_ = 0.0f;
    float time_ = 0.0f;
    float tickTimer_ = 0.0f;
    std::vector<Vector3> positions_;
};
