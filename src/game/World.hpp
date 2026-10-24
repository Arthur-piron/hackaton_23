#pragma once

#include "game/MapFile.hpp"
#include "raylib.h"

#include <string>
#include <unordered_map>
#include <vector>

// La carte du cimetière : sol, décor Kenney et collisions du décor solide.
// Les modèles sont chargés une seule fois par fichier. À créer après
// InitWindow et à détruire avant CloseWindow.
class World {
public:
    explicit World(const MapData& map);
    ~World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void draw(const Camera3D& camera) const;

    // Repousse un cercle (plan XZ) hors du décor solide : il glisse le long des obstacles.
    Vector3 resolveCircle(Vector3 position, float radius) const;
    // Vrai si le segment [a, b] traverse le décor solide ; hit = premier point touché.
    bool segmentBlocked(Vector3 a, Vector3 b, Vector3& hit) const;

    Vector3 playerSpawn() const { return playerSpawn_; }

    // Modèle relatif à assets/models/, chargé au premier appel puis partagé.
    const Model& model(const std::string& file);
    // Boîte englobante d'un modèle posé (échelle = multiplicateur de kKenneyScale).
    static BoundingBox placedBounds(const Model& model, Vector3 position, float yaw, float scale);
    // Position à donner au modèle pour que le centre de sa boîte tombe sur `center` (plan XZ).
    // scale : multiplicateurs de kKenneyScale par axe.
    static Vector3 centeredPosition(const Model& model, Vector3 center, float yaw, Vector3 scale);

    // Obstacles ajoutés par d'autres systèmes (grilles...), activables à la volée.
    int addBlocker(BoundingBox box);
    void setBlockerActive(int id, bool active);

private:
    struct Placed {
        const Model* model;
        Vector3 position;
        float yaw;
        float scale;
    };
    struct Blocker {
        BoundingBox box;
        bool active;
    };

    void addProp(const PropDef& prop);
    void addWall(const WallDef& wall);
    void createGround();

    std::unordered_map<std::string, Model> models_;
    std::unordered_map<std::string, Texture2D> textures_; // texture partagée par kit
    std::vector<Placed> props_;
    std::vector<Blocker> blockers_;
    Vector3 playerSpawn_ = {0.0f, 0.0f, 0.0f};
    Texture2D groundTexture_{};
    Model ground_{};
};
