#pragma once

#include "game/GameMode.hpp"
#include "game/Progression.hpp"
#include "raylib.h"

// Écran titre : choix du mode (Nuit, Sans fin, Cauchemar une fois débloqué), talents,
// quitter ; le tableau des scores local s'affiche à droite.
class MainMenu {
public:
    struct Result {
        int startMode = -1;   // GameMode choisi, -1 si aucun
        bool openTalents = false;
        bool quit = false;
        bool moved = false;   // la sélection a changé (petit son)
        bool refused = false; // mode verrouillé
    };

    void open();
    Result update(float dt, const Progression& progression);
    void draw(const Progression& progression) const;

private:
    enum class Item { Night, Endless, Nightmare, Talents, Quit, Count };

    Rectangle itemRect(int index) const;
    const char* itemLabel(Item item) const;

    int selected_ = 0;
    float time_ = 0.0f;
};
