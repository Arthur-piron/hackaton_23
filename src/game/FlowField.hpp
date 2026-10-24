#pragma once

#include "raylib.h"

#include <vector>

class Structures;
class World;
class Zones;

// Navigation des morts (docs/GAME.md §7.4) : champ de distances (Dijkstra) sur une grille
// de 1 m, calculé depuis la case du joueur. Le décor et les grilles fermées sont
// infranchissables ; une structure coûte 1 + PV / 25 : un long détour coûte plus cher
// que de la casser.
class FlowField {
public:
    // Recalcule le champ vers `target`.
    void rebuild(const World& world, const Zones& zones, const Structures& structures, Vector3 target);
    // Point vers lequel marcher depuis `from` : deux cases plus loin sur la pente du champ,
    // ou la cible elle-même quand elle est proche ou injoignable.
    Vector3 waypoint(Vector3 from, Vector3 target) const;
    bool isBuilt() const { return !distance_.empty(); }

private:
    int cellIndex(Vector3 point) const; // -1 hors de la grille
    Vector3 cellCenter(int index) const;
    // Voisin de plus petite distance (pas de diagonale qui coupe un coin bloqué) ; -1 s'il n'y en a pas.
    int downhill(int index) const;

    float originX_ = 0.0f;
    float originZ_ = 0.0f;
    int width_ = 0;
    int depth_ = 0;
    std::vector<float> cost_;     // coût d'entrée dans la case ; négatif : infranchissable
    std::vector<float> distance_;
};
