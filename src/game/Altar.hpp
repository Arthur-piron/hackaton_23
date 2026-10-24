#pragma once

#include "fx/PointLight.hpp"
#include "game/Events.hpp"
#include "game/MapFile.hpp"
#include "raylib.h"

class Zones;

// L'autel de la chapelle (docs/GAME.md §4) : une prière (E) rend toute la vie,
// une fois par vague. Il luit quand il est prêt et s'éteint après usage.
class Altar {
public:
    static constexpr float kInteractDistance = 2.6f;

    explicit Altar(const AltarDef& def);

    // Nouvelle partie : l'autel est prêt.
    void reset();
    // Début de vague : l'autel se recharge.
    void recharge() { ready_ = true; }
    void update(float dt) { time_ += dt; }
    // Prière : publie AltarUsed si le joueur est blessé et l'autel prêt, à portée et dans une zone ouverte.
    bool tryUse(Vector3 player, bool wounded, const Zones& zones, EventQueue& events);

    bool inReach(Vector3 player) const;
    Vector3 position() const { return position_; }
    // Enseigne au loin, invite à portée (wounded : le joueur a perdu de la vie).
    void drawPrompt(const Camera3D& camera, Vector3 player, bool wounded, const Zones& zones) const;
    // Lueur de l'autel, à ajouter à chaque image.
    PointLight light() const;

private:
    Vector3 position_;
    bool ready_ = true;
    float time_ = 0.0f;
    float usedAt_ = -100.0f; // instant de la dernière prière (éclat de lumière)
};
