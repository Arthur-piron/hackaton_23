#pragma once

#include "game/Events.hpp"
#include "raylib.h"

#include <vector>

class Economy;

// Objets lâchés au sol par les morts : pièces d'or, orbes d'XP et piles de lampe. Ils jaillissent,
// rebondissent, sont aimantés vers le joueur puis disparaissent s'ils traînent.
class Pickups {
public:
    void clear() { items_.clear(); }
    // Fait jaillir l'or (en plusieurs pièces) et l'XP d'une mort.
    void spawnLoot(Vector3 position, int gold, int xp);
    // Fait pleuvoir des pièces du ciel autour d'un point (carte Fortune du mort).
    void spawnCoinRain(Vector3 center, int coins, int goldPerCoin);
    // Pile pour la lampe torche : ramassée, elle publie BatteryPicked.
    void spawnBattery(Vector3 position);
    // Bonus des cartes sur le rayon d'aimantation.
    void setMagnetMultiplier(float multiplier) { magnetMultiplier_ = multiplier; }
    // Aimante, ramasse (crédite l'économie) et fait vieillir les objets.
    void update(float dt, Vector3 playerPosition, Economy& economy, EventQueue& events);
    void draw() const;

private:
    enum class Kind { Gold, Xp, Battery };

    struct Item {
        Kind kind = Kind::Gold;
        int value = 0;
        Vector3 position = {0.0f, 0.0f, 0.0f};
        Vector3 velocity = {0.0f, 0.0f, 0.0f};
        float age = 0.0f;
        float phase = 0.0f;   // décale rotation et flottement d'un objet à l'autre
        bool attracted = false;
        bool collected = false;
    };

    void spawn(Kind kind, int value, Vector3 position);

    std::vector<Item> items_;
    float magnetMultiplier_ = 1.0f;
};
