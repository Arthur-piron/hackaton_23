#pragma once

#include "game/Events.hpp"
#include "game/Projectile.hpp"
#include "raylib.h"

#include <string>
#include <vector>

struct WeaponStats {
    std::string name;
    std::string model;        // fichier dans assets/models/blaster/
    float damage = 10.0f;     // par projectile
    float fireInterval = 0.3f;
    int magazine = 10;
    float reloadTime = 1.2f;
    int pellets = 1;
    float spreadDegrees = 2.0f;
    float projectileSpeed = 45.0f;
    float knockback = 2.0f;
    float shake = 0.15f;      // secousse de caméra par tir
    float recoil = 0.6f;      // recul du joueur (vitesse)
    int pierce = 0;
    bool automatic = false;   // tir continu bouton maintenu
    bool infiniteReserve = false;
};

// Arsenal du joueur : armes, munitions, cadence, rechargement.
class Arsenal {
public:
    static const std::vector<WeaponStats>& catalog();

    Arsenal();
    void reset();

    // fireHeld / firePressed : état du bouton de tir. Renvoie vrai si un tir a eu lieu.
    bool update(float dt, bool fireHeld, bool firePressed, bool reloadPressed, Vector3 muzzle, Vector3 forward,
                ProjectileSystem& projectiles, EventQueue& events);
    void select(int index);
    void cycle(int direction);
    void addAmmo(int weapon, int amount);
    void unlock(int weapon);

    int current() const { return current_; }
    const WeaponStats& stats() const { return catalog()[current_]; }
    int ammoInMagazine() const { return slots_[current_].magazine; }
    int reserve() const { return slots_[current_].reserve; }
    bool isReloading() const { return reloadTimer_ > 0.0f; }
    float reloadProgress() const;
    bool isUnlocked(int weapon) const { return slots_[weapon].unlocked; }
    // Temps écoulé depuis le dernier tir (pour l'animation de recul).
    float timeSinceShot() const { return timeSinceShot_; }

private:
    struct Slot {
        bool unlocked = false;
        int magazine = 0;
        int reserve = 0;
    };

    void startReload(EventQueue& events, Vector3 position);

    std::vector<Slot> slots_;
    int current_ = 0;
    float cooldown_ = 0.0f;
    float reloadTimer_ = 0.0f;
    float timeSinceShot_ = 10.0f;
};
