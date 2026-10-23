#pragma once

#include "raylib.h"

#include <vector>

// Événements de gameplay produits pendant une image et consommés ensuite par
// le son, les particules, la caméra et le HUD. Les systèmes de jeu n'ont ainsi
// pas besoin de connaître ces modules.
enum class EventType {
    Shot,          // value = indice de l'arme
    DryFire,
    ReloadStart,
    EnemyHit,      // value = dégâts
    EnemyKilled,   // value = points
    EnemyAttack,
    EnemyGroan,
    PlayerHurt,    // value = dégâts
    PlayerDied,
    WaveStarted,   // value = numéro de vague
    WaveCleared,   // value = numéro de vague
    GoldPicked,    // value = or ramassé
    XpPicked,      // value = XP ramassée
    LevelUp,       // value = nouveau niveau
};

struct Event {
    EventType type;
    Vector3 position = {0.0f, 0.0f, 0.0f};
    float value = 0.0f;
    Vector3 direction = {0.0f, 0.0f, 0.0f};
};

using EventQueue = std::vector<Event>;
