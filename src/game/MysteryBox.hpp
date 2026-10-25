#pragma once

#include "fx/Lighting.hpp"
#include "fx/PointLight.hpp"
#include "game/Events.hpp"
#include "game/MapFile.hpp"
#include "raylib.h"

#include <cstddef>
#include <vector>

class Economy;
class World;
class Zones;

// La boîte mystère (docs/GAME.md §4) : contre de l'or, elle tire une arme au hasard,
// parfois une variante légendaire dorée. L'arme tirée flotte au-dessus d'elle quelques
// secondes : E pour la prendre. Après kUsesBeforeMove tirages, elle s'envole vers un
// autre emplacement d'une zone ouverte. À créer après InitWindow, à détruire avant CloseWindow.
class MysteryBox {
public:
    static constexpr int kPrice = 950;
    static constexpr int kUsesBeforeMove = 4;
    static constexpr float kInteractDistance = 2.4f;

    struct Prize {
        int weapon = -1; // indice dans Arsenal::catalog ; -1 : rien de pris
        bool legendary = false;
        bool refused = false; // pas assez d'or pour un tirage
    };

    // world doit survivre à la boîte : chaque emplacement y enregistre sa collision.
    MysteryBox(const std::vector<MysteryBoxDef>& spots, World& world);
    ~MysteryBox();
    MysteryBox(const MysteryBox&) = delete;
    MysteryBox& operator=(const MysteryBox&) = delete;

    // Fait passer la boîte et les armes dorées par l'éclairage de nuit.
    void applyLighting(const Lighting& lighting);
    // Nouvelle partie : la boîte revient à l'emplacement `spot`, compteur remis à zéro.
    void reset(int spot = 0);
    void update(float dt, const Zones& zones, EventQueue& events);
    // E à portée : paie et lance un tirage, ou prend l'arme qui flotte au-dessus.
    Prize interact(Vector3 player, const Zones& zones, Economy& economy, EventQueue& events);

    bool inReach(Vector3 player) const;
    Vector3 position() const { return spots_[static_cast<size_t>(spot_)].position; }
    // Emplacement le plus proche d'un point (test --smoke).
    int nearestSpot(Vector3 point) const;

    // weapons : modèles normaux des armes, dans l'ordre d'Arsenal::catalog.
    void draw(const std::vector<Model>& weapons) const;
    // Enseigne au loin, invite et prix à portée.
    void drawPrompt(const Camera3D& camera, Vector3 player, int gold, const Zones& zones) const;
    // Lumière dorée (et son halo) de la boîte, à ajouter à chaque image.
    PointLight light() const;
    // Modèle doré d'une arme, pour la dessiner dans la main du joueur.
    const Model& goldenWeapon(int weapon) const { return goldenWeapons_[static_cast<size_t>(weapon)]; }

private:
    enum class State { Idle, Rolling, Offering, Leaving, Arriving };

    struct Spot {
        Vector3 position;
        float yaw;     // radians
        int blocker;   // collision dans World
    };

    bool usable(const Zones& zones) const;
    void startRoll(EventQueue& events);
    // Après l'offre : la boîte reste, ou s'envole si elle a assez servi.
    void finishOffer(const Zones& zones);
    void moveToAnotherSpot(const Zones& zones);
    // Élévation de l'arme au-dessus du couvercle (0 : dans la boîte).
    float weaponLift() const;

    World& world_;
    Model chest_{};
    float chestHeight_ = 0.0f; // hauteur du couvercle, à l'échelle
    Vector3 chestCenter_ = {0.0f, 0.0f, 0.0f}; // centre de l'emprise au sol du modèle, à l'échelle
    std::vector<Model> goldenWeapons_;
    std::vector<Spot> spots_;
    int spot_ = 0;
    int uses_ = 0;
    State state_ = State::Idle;
    float stateTime_ = 0.0f;
    float tickTimer_ = 0.0f;
    int shownWeapon_ = 0;   // arme affichée pendant le tirage
    Prize prize_;
    float time_ = 0.0f;
};
