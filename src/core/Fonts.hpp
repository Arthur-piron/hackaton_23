#pragma once

#include "raylib.h"

// Police du jeu (Kenney Future) avec les accents français. raylib ne charge que
// l'ASCII par défaut : on demande explicitement Latin-1 et les apostrophes typographiques.
// Chargée après InitWindow, libérée avant CloseWindow ; les chaînes sont en UTF-8.
namespace fonts {

void load();
void unload();
const Font& ui();

// Remplaçants de DrawText / MeasureText avec la police du jeu.
void drawText(const char* text, int x, int y, int size, Color color);
int measureText(const char* text, int size);
// Versions flottantes (mise en page fine des cartes et des écrans).
void drawTextEx(const char* text, Vector2 position, float size, Color color);
Vector2 measureTextEx(const char* text, float size);

} // namespace fonts
