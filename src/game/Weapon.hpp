#pragma once

#include "game/CardModifiers.hpp"
#include "game/Events.hpp"
#include "game/Projectile.hpp"
#include "raylib.h"

#include <array>
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
    int reserveMagazines = 0; // réserve maximale, en nombre de chargeurs
};

// Améliorations achetées chez le marchand, propres à chaque arme.
enum class WeaponUpgrade { Damage, FireRate, Magazine, Count };

// Arsenal du joueur : armes, munitions, cadence, rechargement.
class Arsenal {
public:
    static constexpr int kMaxUpgradeLevel = 3;

    static const std::vector<WeaponStats>& catalog();
    // Multiplicateur apporté par une amélioration à un niveau donné (1 au niveau 0).
    static float upgradeMultiplier(WeaponUpgrade upgrade, int level);

    Arsenal();
    void reset();

    // fireHeld / firePressed : état du bouton de tir. Renvoie vrai si un tir a eu lieu.
    bool update(float dt, bool fireHeld, bool firePressed, bool reloadPressed, Vector3 muzzle, Vector3 forward,
                ProjectileSystem& projectiles, EventQueue& events);
    void select(int index);
    void cycle(int direction);
    void addAmmo(int weapon, int amount);
    // Débloque une arme, chargeur plein et réserve au maximum.
    void unlock(int weapon);
    // Remplit la réserve d'une arme à son maximum.
    void refillReserve(int weapon);
    // Monte une amélioration d'un niveau (le chargeur reçoit aussitôt les balles en plus).
    void upgrade(int weapon, WeaponUpgrade upgrade);
    // Bonus des cartes, appliqués à toutes les armes ; ils se multiplient aux améliorations.
    void setModifiers(const CardModifiers& modifiers) { modifiers_ = modifiers; }

    int current() const { return current_; }
    const WeaponStats& stats() const { return catalog()[current_]; }
    // Taille du chargeur de l'arme courante, bonus compris.
    int magazineSize() const { return magazineOf(current_); }
    int ammoInMagazine() const { return slots_[current_].magazine; }
    int reserve() const { return slots_[current_].reserve; }
    bool isReloading() const { return reloadTimer_ > 0.0f; }
    float reloadProgress() const;
    bool isUnlocked(int weapon) const { return slots_[weapon].unlocked; }
    // Temps écoulé depuis le dernier tir (pour l'animation de recul).
    float timeSinceShot() const { return timeSinceShot_; }

    // Statistiques effectives d'une arme : cartes et améliorations comprises.
    float damageOf(int weapon) const;
    float fireIntervalOf(int weapon) const;
    int magazineOf(int weapon) const;
    int reserveOf(int weapon) const { return slots_[static_cast<size_t>(weapon)].reserve; }
    int maxReserveOf(int weapon) const;
    int upgradeLevel(int weapon, WeaponUpgrade upgrade) const
    {
        return slots_[static_cast<size_t>(weapon)].upgrades[static_cast<size_t>(upgrade)];
    }

private:
    struct Slot {
        bool unlocked = false;
        int magazine = 0;
        int reserve = 0;
        std::array<int, static_cast<size_t>(WeaponUpgrade::Count)> upgrades{};
    };

    void startReload(EventQueue& events, Vector3 position);
    float reloadTime() const { return stats().reloadTime * modifiers_.reloadMultiplier; }
    float upgradeOf(int weapon, WeaponUpgrade upgrade) const
    {
        return upgradeMultiplier(upgrade, upgradeLevel(weapon, upgrade));
    }

    CardModifiers modifiers_;
    std::vector<Slot> slots_;
    int current_ = 0;
    float cooldown_ = 0.0f;
    float reloadTimer_ = 0.0f;
    float timeSinceShot_ = 10.0f;
};
