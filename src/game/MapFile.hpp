#pragma once

#include "raylib.h"

#include <string>
#include <vector>

// Description d'une carte, lue depuis un fichier texte (assets/maps/*.txt).
// Le format est documenté en tête de assets/maps/graveyard.txt.

enum class PropCollision { None, Solid, Trunk };

// Un élément de décor posé sur la carte.
struct PropDef {
    std::string model;     // relatif à assets/models/
    Vector3 position = {0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;      // degrés, autour de l'axe vertical
    float scale = 1.0f;    // multiplicateur de kKenneyScale
    PropCollision collision = PropCollision::None;
};

// Une ligne de murs : les pièces sont alignées et centrées sur le segment.
struct WallDef {
    std::string model;
    Vector2 from = {0.0f, 0.0f}; // (x, z)
    Vector2 to = {0.0f, 0.0f};
    float scale = 1.0f;
};

struct ZoneDef {
    std::string id;
    std::string name;      // affiché en jeu (police par défaut : sans accents)
    int price = 0;         // 0 : ouverte dès le départ
    Vector2 min = {0.0f, 0.0f}; // (x, z)
    Vector2 max = {0.0f, 0.0f};
};

// Grille payante : son ouverture débloque la zone `zone`.
struct GateDef {
    std::string zone;
    Vector3 position = {0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;      // 0 : grille le long de X, 90 : le long de Z
};

struct SpawnDef {
    std::string zone;
    Vector3 position = {0.0f, 0.0f, 0.0f};
};

struct MapData {
    Vector3 playerSpawn = {0.0f, 0.0f, 0.0f};
    std::vector<PropDef> props;
    std::vector<WallDef> walls;
    std::vector<ZoneDef> zones;
    std::vector<GateDef> gates;
    std::vector<SpawnDef> spawns;
};

// Lit une carte ; les lignes invalides sont signalées dans la console et ignorées.
MapData loadMapFile(const std::string& path);
