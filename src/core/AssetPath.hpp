#pragma once

#include <string>

// Chemin absolu d'un asset, à partir du dossier assets/ copié à côté de l'exécutable.
// Exemple : assetPath("models/graveyard/character-zombie.glb").
std::string assetPath(const std::string& relative);
