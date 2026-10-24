#pragma once

#include "game/Events.hpp"
#include "raylib.h"

#include <array>
#include <vector>

class World;
class Zones;

enum class CrateKind { Wood, Large, Ammo, Barrel, Powder, Count };

// Caisses destructibles (docs/GAME.md §7.1) : au début de chaque pause, elles tombent du ciel
// sur des cases libres des zones ouvertes. Elles bloquent le passage et les balles, se cassent
// aux balles et aux explosions, et lâchent leur contenu (géré par Game). Les morts les ignorent.
class Crates {
public:
    static constexpr int kMaxCrates = 10;

    struct Broken {
        Vector3 position;
        CrateKind kind;
    };

    // world doit survivre aux caisses : elles y enregistrent leurs collisions et leurs modèles.
    explicit Crates(World& world);
    Crates(const Crates&) = delete;
    Crates& operator=(const Crates&) = delete;

    void clear();
    // Fait tomber jusqu'à `count` caisses (au plus kMaxCrates au sol), loin du joueur et des apparitions.
    void spawnWave(int count, const Zones& zones, Vector3 player);
    // Balle arrêtée par le décor en `impact` : vrai si c'était une caisse (elle encaisse les dégâts).
    bool hitAt(Vector3 impact, float damage);
    // Explosion : touche les caisses du rayon à la prochaine mise à jour (réactions en chaîne).
    void queueBlast(Vector3 center, float radius, float damage);
    // Chute, coups et casse. Publie CrateLanded et CrateBroken ; renvoie les caisses cassées.
    std::vector<Broken> update(float dt, EventQueue& events);
    void draw() const;

private:
    struct Kind {
        const Model* model;
        float scale;    // multiplicateur de kKenneyScale
        float health;
        Color tint;
    };
    struct Crate {
        CrateKind kind = CrateKind::Wood;
        bool alive = false;
        bool landed = false;
        Vector3 position = {0.0f, 0.0f, 0.0f}; // centre au sol
        Vector3 drawPosition = {0.0f, 0.0f, 0.0f};
        BoundingBox box = {};
        float yaw = 0.0f;   // degrés
        float height = 0.0f; // altitude pendant la chute
        float fallSpeed = 0.0f;
        float health = 0.0f;
        float hurt = 0.0f;  // secousse et flash après un coup
        int blocker = -1;
    };
    struct Blast {
        Vector3 center;
        float radius;
        float damage;
    };

    static CrateKind randomKind();
    bool placeFree(Vector3 point, const std::vector<Vector3>& spawns, Vector3 player) const;

    World& world_;
    std::array<Kind, static_cast<size_t>(CrateKind::Count)> kinds_{};
    std::array<Crate, kMaxCrates> crates_{};
    std::vector<Blast> blasts_;
};
