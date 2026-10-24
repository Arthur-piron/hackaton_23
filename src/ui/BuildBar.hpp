#pragma once

#include "game/BuildMode.hpp"
#include "game/Economy.hpp"

// Barre du mode construction, en bas de l'écran : catégories (1-4), éléments de la
// catégorie avec leur coût (en rouge s'il manque), et rappel des touches.
class BuildBar {
public:
    void draw(const BuildMode& mode, const Economy& economy) const;
};
