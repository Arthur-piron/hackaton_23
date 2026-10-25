#pragma once

#include "raylib.h"

#include <string>

enum class EnemyType;

// Modificateurs d'élite (GAME.md §5) : un halo coloré, un nom, un effet.
enum class EliteKind { None, Explosive, Armored, Swift, Regenerating, Count };

struct EliteInfo {
    const char* masculine;   // « Marcheur explosif »
    const char* feminine;    // « Brute explosive »
    Color color;             // halo, lumière et nom
};

const EliteInfo& eliteInfo(EliteKind kind);
// Nom affiché au-dessus de l'élite, par exemple « Vampire blindé ».
std::string eliteName(EnemyType type, EliteKind kind);
// Tirage à l'apparition d'un mort : None la plupart du temps.
EliteKind rollElite(int wave, float chanceMultiplier);

namespace elites {
constexpr float kHealthMultiplier = 1.5f;
constexpr int kRewardMultiplier = 3;        // or et XP
constexpr int kMinBones = 1;
constexpr int kMaxBones = 3;
// Explosif : souffle à sa mort, qui blesse le joueur et les autres morts.
constexpr float kExplosionRadius = 3.0f;
constexpr float kExplosionDamage = 35.0f;
constexpr float kExplosionKnockback = 6.0f;
// Blindé : moitié des dégâts reçus de face.
constexpr float kArmorFrontFactor = 0.5f;
// Rapide.
constexpr float kSwiftSpeed = 1.8f;
// Régénérant : 5 % de sa vie par seconde s'il n'est pas touché pendant 2 s.
constexpr float kRegenFraction = 0.05f;
constexpr float kRegenDelay = 2.0f;
}
