#pragma once

#include "raylib.h"

#include <array>
#include <cstddef>
#include <vector>

// Particules texturées (flashs de tir, sang, étincelles, fumée, poussière) et
// taches de sang au sol. Tout est préalloué : aucune allocation pendant le jeu.
class Particles {
public:
    Particles();
    ~Particles();
    Particles(const Particles&) = delete;
    Particles& operator=(const Particles&) = delete;

    void clear();
    void update(float dt);
    // À appeler dans BeginMode3D, après les objets opaques.
    void draw(const Camera3D& camera) const;

    // Flash de bouche, étincelles et fumée. strength ≈ 1 pour un pistolet.
    void muzzleFlash(Vector3 muzzle, Vector3 forward, float strength);
    // Giclée de sang dans la direction de la balle, gouttes qui tombent.
    void bloodHit(Vector3 position, Vector3 direction);
    // Grosse giclée et flaque de sang au sol.
    void bloodKill(Vector3 position, Vector3 direction);
    // Balle qui frappe le décor : étincelles et poussière. normal : face touchée.
    void impact(Vector3 position, Vector3 normal);
    // Poussière sous les pieds pendant un sprint (émission continue, à appeler à chaque image).
    void sprintDust(Vector3 feet, Vector3 direction, float dt);

private:
    enum Tex { kMuzzle1, kMuzzle2, kMuzzle3, kMuzzle4, kGlow, kFlare, kSmoke, kSplat1, kSplat2, kSplash, kTexCount };
    enum class Shape { Billboard, Stretched, Flat };
    // Ce que fait une particule soumise à la gravité en touchant le sol.
    enum class Landing { Bounce, Vanish, Stain };

    struct Particle {
        Vector3 position;
        Vector3 velocity;
        float gravity;
        float drag;      // frottement (fraction de vitesse perdue par seconde)
        float age;
        float life;
        float sizeStart;
        float sizeEnd;
        float stretch;   // Stretched : allongement par unité de vitesse
        float rotation;
        float spin;
        Color colorStart;
        Color colorEnd;
        Tex texture;
        Shape shape;
        bool additive;
        Landing landing;
    };

    struct Decal {
        Vector3 position;
        float rotation;
        float size;
        float age;
        float life;
        Color color;
        Tex texture;
    };

    Particle& spawn();
    void addDecal(Vector3 position, float size, float life, Color color, Tex texture);
    void bloodSpray(Vector3 position, Vector3 direction, int drops, float force);
    void drawParticles(const Camera3D& camera, bool additive) const;
    void drawDecals() const;

    std::array<Texture2D, kTexCount> textures_{};
    std::vector<Particle> particles_; // capacité réservée une fois pour toutes
    std::vector<Decal> decals_;       // taille fixe
    size_t nextOverwrite_ = 0;
    float dustTimer_ = 0.0f;
};
