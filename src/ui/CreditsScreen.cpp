#include "ui/CreditsScreen.hpp"

#include "core/Fonts.hpp"
#include "ui/UiScale.hpp"
#include "raylib.h"

#include <algorithm>
#include <cmath>

namespace {
// Une ligne : texte et style (0 : titre de section, 1 : nom, 2 : détail).
struct Line {
    const char* text;
    int style;
};
const Line kLines[] = {
    {"LAST LIGHT", 0},
    {"Un jeu de survie fait en un week-end de hackathon", 2},
    {"", 2},
    {"L'équipe", 0},
    {"Arthur, Léandre et David", 1},
    {"et leurs Claude, qui ont codé jour et nuit", 2},
    {"", 2},
    {"Modèles 3D et textures", 0},
    {"Kenney (www.kenney.nl)", 1},
    {"Graveyard, Blaster, Survival, Tower Defense, Retro Urban, Fantasy Town, Castle, Nature Kits", 2},
    {"", 2},
    {"Interface", 0},
    {"Kenney : Fonts, Board Game Icons, Game Icons, UI Pack", 1},
    {"", 2},
    {"Sons", 0},
    {"Kenney : Impact, Interface, RPG Audio, Sci-fi Sounds, Music Jingles", 1},
    {"ianzazz, Darsycho, Tabasco, artisticdude, saturn91 (OpenGameArt.org)", 1},
    {"", 2},
    {"Musique (OpenGameArt.org)", 0},
    {"Cleyton Kauffman, congusbongus, wolfgang, sketchman3", 1},
    {"ferk, cynicmusic, MintoDog", 1},
    {"", 2},
    {"Technologie", 0},
    {"raylib 5.5, Ramon Santamaria et contributeurs", 1},
    {"cgltf, Johannes Kuhlmann", 1},
    {"", 2},
    {"Tous les assets sont sous licence CC0 1.0. Merci à leurs auteurs.", 2},
    {"", 2},
    {"Tiens jusqu'à l'aube.", 0},
};
constexpr int kSizes[] = {34, 22, 17};
constexpr Color kColors[] = {{230, 200, 120, 255}, {238, 232, 218, 255}, {150, 145, 135, 255}};
constexpr float kLineGap = 12.0f;
constexpr float kScrollSpeed = 28.0f; // pixels par seconde
constexpr float kScrollDelay = 1.5f;  // le haut de la liste reste un instant à l'écran
constexpr float kTop = 120.0f;

float lineHeight(const Line& line)
{
    // Un peu d'air sous les titres de section.
    const float extra = line.style == 0 && line.text[0] != '\0' ? kLineGap : 0.0f;
    return static_cast<float>(kSizes[line.style]) + kLineGap + extra;
}
}

bool CreditsScreen::update(float dt)
{
    time_ += dt;
    return IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
           IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void CreditsScreen::draw() const
{
    const float w = ui::width();
    const float h = ui::height();
    float total = 0.0f;
    for (const Line& line : kLines) total += lineHeight(line);
    // Défile vers le haut puis repart du début quand tout est passé.
    const float travel = total + h - kTop;
    const float scroll = std::fmod(std::max(0.0f, time_ - kScrollDelay) * kScrollSpeed, travel);
    float y = kTop - scroll;
    for (const Line& line : kLines) {
        const int size = kSizes[line.style];
        if (y > -static_cast<float>(size) && y < h) {
            fonts::drawText(line.text, static_cast<int>(std::round(w / 2.0f)) - fonts::measureText(line.text, size) / 2,
                            static_cast<int>(y), size, kColors[line.style]);
        }
        y += lineHeight(line);
    }
    const char* hint = "Échap : retour";
    fonts::drawText(hint, static_cast<int>(w) - fonts::measureText(hint, 16) - 20, static_cast<int>(h) - 34, 16,
                    kColors[2]);
}
