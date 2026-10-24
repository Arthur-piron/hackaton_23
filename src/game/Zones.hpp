#pragma once

#include "game/Events.hpp"
#include "game/MapFile.hpp"
#include "raylib.h"

#include <string>
#include <vector>

class Economy;
class World;

// Les zones du cimetière, leurs grilles payantes et leurs points d'apparition.
// Une zone s'ouvre quand on paie sa grille ; ses points d'apparition deviennent
// alors actifs. Sert aussi de navigation simple aux morts : pour rejoindre une
// autre zone, ils passent par la grille qui y mène.
class Zones {
public:
    // world doit survivre à Zones : les grilles y enregistrent leur collision.
    Zones(const MapData& map, World& world);
    Zones(const Zones&) = delete;
    Zones& operator=(const Zones&) = delete;

    // Referme toutes les grilles (nouvelle partie).
    void reset();
    void update(float dt);
    void draw() const;
    // Invite « E : ouvrir la grille » au-dessus de la grille la plus proche.
    void drawPrompt(const Camera3D& camera, Vector3 player, int gold) const;

    // Ouvre la grille fermée la plus proche (à portée) si l'or suffit, et débite le prix.
    bool tryOpenNearest(Vector3 player, Economy& economy, EventQueue& events);

    // Points d'apparition des zones ouvertes.
    std::vector<Vector3> openSpawnPoints() const;
    // Point vers lequel marcher depuis `from` pour rejoindre `target` :
    // la cible elle-même, ou la grille de passage si elle est dans une autre zone.
    Vector3 steer(Vector3 from, Vector3 target) const;
    // Zone contenant le point (-1 : hors carte).
    int zoneAt(Vector3 position) const;
    const std::string& zoneName(int zone) const { return zones_[static_cast<size_t>(zone)].def.name; }

private:
    struct Zone {
        ZoneDef def;
        bool open = false;
        std::vector<Vector3> spawns;
    };
    struct Gate {
        int zone = -1;        // zone débloquée par la grille
        int otherZone = -1;   // zone de l'autre côté
        Vector3 center = {0.0f, 0.0f, 0.0f};
        Vector3 axis = {1.0f, 0.0f, 0.0f};   // direction de la grille
        Vector3 normal = {0.0f, 0.0f, 1.0f}; // de otherZone vers zone
        float yaw = 0.0f;
        Vector3 framePosition = {0.0f, 0.0f, 0.0f};
        Vector3 leafPosition = {0.0f, 0.0f, 0.0f};
        int blocker = -1;      // collision du battant
        bool open = false;
        float openTime = 0.0f; // temps écoulé depuis l'ouverture (animation)
    };

    int zoneIndex(const std::string& id) const;
    int nearestClosedGate(Vector3 player) const;
    void buildRoutes();

    World& world_;
    const Model* frameModel_;
    const Model* leafModel_;
    std::vector<Zone> zones_;
    std::vector<Gate> gates_;
    // routes_[a][b] : grille à franchir pour aller de la zone a vers la zone b (-1 : aucune).
    std::vector<std::vector<int>> routes_;
};
