#pragma once

#include "game/Enemy.hpp"
#include "game/Events.hpp"
#include "raylib.h"

#include <unordered_map>
#include <vector>

class World;

// Lampe torche du gardien : allumée tant que le clic droit est maintenu.
// La batterie se vide en éclairant ; éteinte, la dynamo la remonte lentement,
// mais seulement jusqu'à la moitié : au-delà, il faut ramasser des piles.
// Les morts dans le cône (sauf les squelettes) sont ralentis et brûlent.
class Flashlight {
public:
    static constexpr float kRange = 12.0f;            // mètres
    static constexpr float kHalfAngleDegrees = 25.0f;
    static constexpr float kPickupCharge = 0.35f;     // batterie rendue par une pile

    struct LightTick {
        Enemy* enemy;
        float damage;
    };

    // Chance qu'un mort de ce type lâche une pile.
    static float batteryDropChance(EnemyType type);
    // Multiplicateur des brûlures et du ralenti de la lumière : 0 = insensible (squelettes).
    static float lightSensitivity(EnemyType type);

    void reset();
    // Bonus des cartes : capacité de la batterie (Pile longue durée), dégâts en plus (Lampe UV).
    void setModifiers(float capacityMultiplier, float bonusDamagePerSecond);
    // held : clic droit enfoncé. Allume, éteint, vide ou recharge la batterie, fait hoqueter la lampe.
    void update(float dt, bool held, Vector3 position, EventQueue& events);
    void recharge(float fraction);
    // Ralentit les morts éclairés (le faisceau ne traverse pas le décor) et renvoie
    // les brûlures à infliger pendant cette image.
    std::vector<LightTick> illuminate(float dt, Vector3 origin, Vector3 direction, std::vector<Enemy>& enemies,
                                      const World& world);

    bool isOn() const { return on_; }
    // Allumée et pas en plein hoquet : le faisceau agit.
    bool isLit() const { return on_ && dropout_ <= 0.0f; }
    // Intensité du faisceau (0..1) : baisse quand la batterie faiblit.
    float intensity() const;
    // Charge de la batterie, 0..1.
    float battery() const { return charge_; }
    bool isLow() const;
    // Vrai si les dégâts de la lampe sont renforcés (Lampe UV).
    bool isUv() const { return bonusDamage_ > 0.0f; }

private:
    void updateFlicker(float dt, Vector3 position, EventQueue& events);

    float charge_ = 1.0f;
    float capacityMultiplier_ = 1.0f;
    float bonusDamage_ = 0.0f;
    bool on_ = false;
    bool needsRelease_ = false;   // batterie vidée : il faut relâcher le bouton pour rallumer
    float idleTime_ = 0.0f;       // temps passé éteinte (la dynamo attend un peu)
    float flickerTimer_ = 0.0f;
    float dropout_ = 0.0f;        // durée restante du hoquet en cours
    float flickerSoundCooldown_ = 0.0f;
    float tickTimer_ = 0.0f;
    std::unordered_map<unsigned, float> pending_; // dégâts accumulés par ennemi avant la prochaine morsure
};
