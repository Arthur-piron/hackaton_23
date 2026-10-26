#pragma once

#include "raylib.h"

// Mise à l'échelle de l'interface. Les écrans sont mis en page pour 1280 x 720 :
// on dessine dans un espace virtuel (width() x height()) agrandi d'un facteur scale()
// pour remplir la fenêtre, quelle que soit la résolution (plein écran 1080p, 1440p, 4K...).
// Le monde 3D, lui, est rendu à la résolution réelle.
namespace ui {

float scale();
// Taille de l'écran dans l'espace de l'interface (au moins 1280 x 720 sur l'axe limitant).
float width();
float height();
// Versions entières arrondies au-dessus (remplaçants de GetScreenWidth / GetScreenHeight) :
// un fond plein écran ne laisse pas de liseré quand l'échelle n'est pas entière.
int screenWidth();
int screenHeight();
// Souris dans l'espace de l'interface.
Vector2 mouse();
// Point en pixels réels (GetWorldToScreen) vers l'espace de l'interface.
Vector2 fromScreen(Vector2 screen);
// Rectangle de l'interface vers les pixels réels (BeginScissorMode).
Rectangle toScreen(Rectangle r);

// Encadre tout le dessin 2D de l'interface.
void begin();
void end();

} // namespace ui
