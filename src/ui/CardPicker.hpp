#pragma once

#include "game/Cards.hpp"
#include "raylib.h"

#include <vector>

// Écran de choix de carte à la montée de niveau : fond assombri, cartes qui
// glissent depuis le bas, survol qui agrandit, choix à la souris ou avec 1/2/3.
class CardPicker {
public:
    struct Result {
        int chosen = -1;            // indice de la carte choisie, une fois l'animation de sortie finie
        bool hoverChanged = false;  // pour un petit son de survol
        bool picked = false;        // le joueur vient de cliquer (son de validation)
    };

    static Color rarityColor(CardRarity rarity);
    static Color categoryColor(CardCategory category);

    void open(const std::vector<CardId>& choices, const Cards& owned, int level);
    Result update(float dt);
    void draw() const;
    // Valide une carte (clavier, souris ou pilote automatique du --smoke).
    void choose(int index);
    // Vrai quand toutes les cartes sont arrivées et qu'aucun choix n'est en cours.
    bool isSettled() const;
    CardId choice(int index) const { return slots_[static_cast<size_t>(index)].id; }

private:
    struct Slot {
        CardId id = CardId::SteadyHand;
        int nextRank = 1;    // rang obtenu si on la prend
        float hover = 0.0f;  // 0..1, animé
    };

    // Position finale de la carte i, avant animation.
    Rectangle baseRect(int index) const;
    // Position animée (arrivée, survol, sortie) et facteur d'agrandissement.
    Rectangle animatedRect(int index, float& scale, float& alpha) const;
    void drawCard(int index) const;

    std::vector<Slot> slots_;
    int level_ = 1;
    int hovered_ = -1;
    int chosen_ = -1;
    float time_ = 0.0f;
    float outro_ = 0.0f;
};
