#pragma once

#include "game/Economy.hpp"
#include "game/Events.hpp"
#include "game/Structures.hpp"
#include "raylib.h"

#include <string>
#include <vector>

class World;
class Zones;

// Mode construction (docs/GAME.md §7.2), pendant les pauses entre vagues :
// B entre et sort, 1-4 change de catégorie, molette change d'élément, T tourne,
// clic pose (maintenu : pose en ligne en glissant), X revend, Retour arrière annule,
// U améliore la structure visée (établi requis).
class BuildMode {
public:
    // Ce qui entoure la pose : décor, zones, joueur et points à garder dégagés (marchand, boîte, autel).
    struct Surroundings {
        const World& world;
        const Zones& zones;
        Vector3 player;
        std::vector<Vector3> keepClear;
    };

    bool isActive() const { return active_; }
    // Nouvelle partie, ou début de vague : on quitte le mode et la pile d'annulation est vidée.
    void reset();
    // Lit le clavier et la souris. cursor : point du sol visé.
    void update(Vector3 cursor, bool intermission, const Surroundings& around, Structures& structures,
                Economy& economy, EventQueue& events);

    // Grille, fantôme vert ou rouge et contour de la structure visée (dans BeginMode3D).
    void drawWorld(const Structures& structures) const;
    // E près de l'établi pendant une pause : il passe au niveau II (améliorations niv. 3).
    // Vrai si un établi était à portée.
    bool upgradeWorkbenchNear(Vector3 player, bool intermission, Structures& structures, Economy& economy,
                              EventQueue& events) const;

    // Coût ou raison du refus au-dessus du fantôme, et amélioration possible de la structure visée.
    void drawPrompt(const Camera3D& camera) const;

    BuildCategory category() const { return category_; }
    StructureKind selected() const { return selected_; }
    // Éléments de la catégorie courante, dans l'ordre du catalogue.
    std::vector<StructureKind> items() const;

private:
    struct Placement {
        int id;
        StructureKind kind;
    };

    void selectCategory(BuildCategory category);
    // Centre accroché à la grille : les éléments 2×1 se centrent sur l'arête entre deux cases.
    Vector3 snap(Vector3 cursor) const;
    // Vide si la pose est permise, sinon la raison du refus.
    std::string checkPlacement(Vector3 center, const Surroundings& around, const Economy& economy) const;
    void place(Vector3 player, Structures& structures, Economy& economy, EventQueue& events);
    void sell(Structures& structures, Economy& economy, EventQueue& events);
    void undo(Structures& structures, Economy& economy, EventQueue& events);
    // Vide si la structure visée peut monter d'un niveau, sinon la raison ; cost reçoit le prix.
    std::string checkUpgrade(const Structures& structures, const Economy& economy, int& wood, int& scrap,
                             int& gold) const;
    void upgrade(Structures& structures, Economy& economy, EventQueue& events);

    bool active_ = false;
    BuildCategory category_ = BuildCategory::Defenses;
    StructureKind selected_ = StructureKind::Palisade;
    bool rotated_ = false;
    Vector3 cursor_ = {0.0f, 0.0f, 0.0f};
    Vector3 ghost_ = {0.0f, 0.0f, 0.0f};
    std::string refusal_;     // vide : pose possible
    int hovered_ = -1;        // structure sous le curseur
    std::string upgradeText_; // amélioration de la structure visée (coût ou raison du refus)
    bool upgradeOk_ = false;
    bool dragging_ = false;
    Vector3 lastPlaced_ = {0.0f, -1.0f, 0.0f};
    std::vector<Placement> placedThisPause_;
};
