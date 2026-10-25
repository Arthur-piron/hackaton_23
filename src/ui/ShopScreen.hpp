#pragma once

#include "game/Shop.hpp"
#include "raylib.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

// Écran du marchand : trois colonnes (armes, améliorations, consommables) de trois
// articles, navigation à la souris ou au clavier, or restant en grand.
// L'achat lui-même est fait par Game via Shop ; l'écran ne fait que le demander.
class ShopScreen {
public:
    struct Result {
        bool close = false;
        bool moved = false;   // la sélection a changé (petit clic)
        int purchase = -1;    // article demandé (indice de Shop::catalog), à acheter par Game
    };

    // weapon : arme proposée d'abord pour les améliorations ; gold : or affiché au départ.
    void open(int weapon, int gold);
    Result update(float dt, const Shop& shop, const Arsenal& arsenal, int gold);
    // Retour de l'achat demandé : éclair doré si réussi, secousse rouge sinon.
    void notifyPurchase(bool success, const std::string& message);
    // Place la sélection sur un article (pilote automatique du --smoke).
    void focus(int item);
    // frozenLeft : temps figé restant ; waveIn : secondes avant la vague une fois ce temps écoulé.
    void draw(const Shop& shop, const Arsenal& arsenal, int gold, float frozenLeft, float waveIn) const;

    // Arme visée par la colonne des améliorations.
    int upgradeWeapon() const { return weapon_; }

private:
    static constexpr int kColumns = static_cast<int>(ShopCategory::Count);

    // Articles de chaque colonne (indices du catalogue), dans l'ordre du catalogue.
    const std::vector<int>& column(int index) const { return columns_[static_cast<size_t>(index)]; }
    Rectangle columnRect(int index) const;
    Rectangle rowRect(int columnIndex, int row) const;
    // Flèches du sélecteur d'arme (colonne des améliorations).
    Rectangle selectorRect() const;
    int selectedItem() const { return column(column_)[static_cast<size_t>(row_)]; }
    void cycleWeapon(const Arsenal& arsenal, int direction);

    void drawHeader(int gold, float frozenLeft, float waveIn) const;
    void drawColumn(int index, const Shop& shop, const Arsenal& arsenal, int gold) const;
    void drawRow(int columnIndex, int row, const Shop& shop) const;

    std::array<std::vector<int>, kColumns> columns_;
    std::vector<float> hover_;   // 0..1 par article, animé
    int column_ = 0;
    int row_ = 0;
    int weapon_ = 0;
    int requested_ = -1;         // article dont on attend le résultat d'achat
    int flashItem_ = -1;
    float flash_ = 0.0f;         // éclair d'un achat réussi
    int shakeItem_ = -1;
    float shake_ = 0.0f;         // secousse d'un achat refusé
    float goldShown_ = 0.0f;     // l'or affiché descend jusqu'à l'or réel
    float goldPulse_ = 0.0f;
    std::string message_;
    Color messageColor_ = WHITE;
    float messageAge_ = 999.0f;
    float time_ = 0.0f;
    Vector2 lastMouse_ = {0.0f, 0.0f};
};
