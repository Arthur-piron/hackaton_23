#pragma once

#include "raylib.h"

#include <vector>

// Balle visible (traçante). On garde la position précédente pour tester la
// collision sur tout le segment parcouru pendant l'image : à 45 u/s, une balle
// avance de plus d'un rayon de zombie par image.
struct Projectile {
    Vector3 position = {0.0f, 0.0f, 0.0f};
    Vector3 previous = {0.0f, 0.0f, 0.0f};
    Vector3 velocity = {0.0f, 0.0f, 0.0f};
    float damage = 0.0f;
    float knockback = 0.0f;
    float life = 0.0f;
    int pierce = 0; // nombre d'ennemis supplémentaires traversés
    unsigned lastHit = 0; // id du dernier ennemi touché (0 : aucun)
    bool alive = true;
};

class ProjectileSystem {
public:
    void spawn(const Projectile& projectile) { projectiles_.push_back(projectile); }
    void update(float dt);
    void draw() const;
    void clear() { projectiles_.clear(); }

    std::vector<Projectile>& projectiles() { return projectiles_; }

private:
    std::vector<Projectile> projectiles_;
};

// Vrai si le segment [a, b] passe à moins de radius de center ; t = position sur le segment (0..1).
bool segmentHitsSphere(Vector3 a, Vector3 b, Vector3 center, float radius, float& t);
