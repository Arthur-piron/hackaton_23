#pragma once

#include "raylib.h"

#include <vector>

class World;

// Catégories du mode construction (docs/GAME.md §7.2), dans l'ordre des touches 1 à 4.
enum class BuildCategory { Defenses, Turrets, Traps, Utilities, Count };

enum class StructureKind { Palisade, Barricade, Door, Count };

// Description d'une structure du catalogue (GAME.md §7.3).
struct StructureInfo {
    const char* name;
    BuildCategory category;
    int cellsX;        // empreinte en cases de 1 m, sans rotation
    int cellsZ;
    int wood;
    int scrap;
    int gold;
    float health;
    unsigned mask;     // ce que la structure arrête (BlockMask)
    const char* model; // relatif à assets/models/
};

// Les structures posées par le joueur : collision dans World, rendu, porte qui s'ouvre
// devant le joueur. Les dégâts des morts viendront avec le FlowField (B2).
class Structures {
public:
    static const StructureInfo& info(StructureKind kind);
    // Emprise au sol d'une structure centrée sur `center` (rotated : tournée de 90°).
    static BoundingBox footprint(StructureKind kind, Vector3 center, bool rotated);

    // world doit survivre aux structures : elles y enregistrent leurs collisions et leurs modèles.
    explicit Structures(World& world);
    Structures(const Structures&) = delete;
    Structures& operator=(const Structures&) = delete;

    void clear();
    // Pose une structure (la validité est vérifiée par BuildMode). Renvoie son identifiant.
    int place(StructureKind kind, Vector3 center, bool rotated);
    void remove(int id);
    // Structure dont l'emprise contient le point (-1 : aucune).
    int at(Vector3 point) const;
    bool exists(int id) const;
    StructureKind kind(int id) const { return items_[static_cast<size_t>(id)].kind; }
    float healthFraction(int id) const;
    BoundingBox bounds(int id) const;

    // Les portes s'ouvrent quand le joueur approche.
    void update(float dt, Vector3 player);
    void draw() const;
    // Dessine le modèle d'une structure à un endroit donné (fantôme du mode construction).
    void drawModel(StructureKind kind, Vector3 center, bool rotated, Color tint) const;

private:
    struct Item {
        StructureKind kind = StructureKind::Palisade;
        bool alive = false;
        Vector3 center = {0.0f, 0.0f, 0.0f};
        bool rotated = false;
        float health = 0.0f;
        int blocker = -1;
        float open = 0.0f; // porte : 0 fermée, 1 ouverte
    };

    World& world_;
    std::vector<const Model*> models_;
    std::vector<float> modelScales_; // l'empreinte la plus longue mesure exactement son nombre de cases
    std::vector<Item> items_;        // les identifiants sont des indices ; les emplacements libres sont réutilisés
};
