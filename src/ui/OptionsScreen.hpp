#pragma once

#include "game/Progression.hpp"
#include "raylib.h"

// Écran des options, ouvert depuis le menu : volumes (général, musique, effets) réglés par
// pas de 10 % avec ← → (ou clic sur les bords), plein écran, accès aux crédits.
class OptionsScreen {
public:
    struct Result {
        bool close = false;
        bool moved = false;       // la sélection a changé (petit son)
        bool changed = false;     // un réglage a changé : à appliquer et sauvegarder
        bool openCredits = false;
    };

    void open(const Settings& settings);
    Result update(float dt);
    void draw() const;
    const Settings& settings() const { return settings_; }

private:
    enum class Row { Master, Music, Effects, Fullscreen, Credits, Back, Count };

    Rectangle rowRect(int index) const;
    const char* label(Row row) const;
    // Valeur réglable d'une ligne de volume (nul pour les autres).
    int* volume(Row row);
    const int* volume(Row row) const;

    Settings settings_;
    int selected_ = 0;
    float time_ = 0.0f;
};
