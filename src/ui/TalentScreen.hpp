#pragma once

#include "game/Progression.hpp"
#include "raylib.h"

// Écran des talents, ouvert depuis le menu : liste des talents, rangs, coût en
// ossements. Flèches / ZS ou souris pour choisir, Entrée ou clic pour acheter.
class TalentScreen {
public:
    struct Result {
        bool close = false;
        bool moved = false;   // la sélection a changé (petit son)
        int purchase = -1;    // indice du talent à acheter (TalentId)
    };

    void open();
    Result update(float dt);
    void draw(const Progression& progression) const;
    // Retour visuel d'une tentative d'achat.
    void notifyPurchase(bool bought);

private:
    Rectangle rowRect(int index) const;

    int selected_ = 0;
    float time_ = 0.0f;
    float flash_ = 0.0f;   // 1 → 0 après un achat ou un refus
    bool flashOk_ = false;
};
