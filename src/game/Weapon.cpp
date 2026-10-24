#include "game/Weapon.hpp"

#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kProjectileLife = 1.2f;

// Gain de chaque niveau d'amélioration (dans l'ordre de WeaponUpgrade) : dégâts, cadence, chargeur.
constexpr float kUpgradeSteps[] = {0.20f, 0.12f, 0.30f};
static_assert(sizeof(kUpgradeSteps) / sizeof(kUpgradeSteps[0]) == static_cast<size_t>(WeaponUpgrade::Count),
              "un gain par amélioration");

float randomSigned()
{
    return static_cast<float>(GetRandomValue(-1000, 1000)) / 1000.0f;
}

bool chance(float probability)
{
    return static_cast<float>(GetRandomValue(0, 9999)) < probability * 10000.0f;
}
}

const std::vector<WeaponStats>& Arsenal::catalog()
{
    static const std::vector<WeaponStats> weapons = [] {
        std::vector<WeaponStats> list;

        WeaponStats pistol;
        pistol.name = "Revolver";
        pistol.model = "blaster-a.glb";
        pistol.damage = 34.0f;
        pistol.fireInterval = 0.28f;
        pistol.magazine = 8;
        pistol.reloadTime = 1.1f;
        pistol.spreadDegrees = 1.5f;
        pistol.knockback = 3.0f;
        pistol.shake = 0.12f;
        pistol.infiniteReserve = true;
        list.push_back(pistol);

        WeaponStats shotgun;
        shotgun.name = "Fusil a pompe";
        shotgun.model = "blaster-d.glb";
        shotgun.damage = 18.0f;
        shotgun.fireInterval = 0.85f;
        shotgun.magazine = 6;
        shotgun.reloadTime = 1.8f;
        shotgun.pellets = 9;
        shotgun.spreadDegrees = 11.0f;
        shotgun.projectileSpeed = 38.0f;
        shotgun.knockback = 4.5f;
        shotgun.shake = 0.45f;
        shotgun.recoil = 3.0f;
        shotgun.reserveMagazines = 5;
        list.push_back(shotgun);

        WeaponStats smg;
        smg.name = "Mitraillette";
        smg.model = "blaster-h.glb";
        smg.damage = 16.0f;
        smg.fireInterval = 0.075f;
        smg.magazine = 40;
        smg.reloadTime = 1.6f;
        smg.spreadDegrees = 5.0f;
        smg.projectileSpeed = 52.0f;
        smg.knockback = 1.2f;
        smg.shake = 0.08f;
        smg.recoil = 0.3f;
        smg.automatic = true;
        smg.reserveMagazines = 5;
        list.push_back(smg);

        WeaponStats rifle;
        rifle.name = "Fusil de chasse";
        rifle.model = "blaster-k.glb";
        rifle.damage = 120.0f;
        rifle.fireInterval = 0.9f;
        rifle.magazine = 5;
        rifle.reloadTime = 2.0f;
        rifle.spreadDegrees = 0.3f;
        rifle.projectileSpeed = 80.0f;
        rifle.knockback = 6.0f;
        rifle.shake = 0.35f;
        rifle.recoil = 2.0f;
        rifle.pierce = 4;
        rifle.reserveMagazines = 5;
        list.push_back(rifle);
        return list;
    }();
    return weapons;
}

Arsenal::Arsenal()
{
    reset();
}

void Arsenal::reset()
{
    const auto& weapons = catalog();
    slots_.assign(weapons.size(), Slot{});
    for (size_t i = 0; i < weapons.size(); ++i) slots_[i].magazine = magazineOf(static_cast<int>(i));
    slots_[0].unlocked = true;
    current_ = 0;
    cooldown_ = 0.0f;
    reloadTimer_ = 0.0f;
    timeSinceShot_ = 10.0f;
}

float Arsenal::upgradeMultiplier(WeaponUpgrade upgrade, int level)
{
    return 1.0f + kUpgradeSteps[static_cast<size_t>(upgrade)] * static_cast<float>(level);
}

float Arsenal::damageOf(int weapon) const
{
    return catalog()[static_cast<size_t>(weapon)].damage * modifiers_.damageMultiplier *
           upgradeOf(weapon, WeaponUpgrade::Damage);
}

float Arsenal::fireIntervalOf(int weapon) const
{
    return catalog()[static_cast<size_t>(weapon)].fireInterval /
           (modifiers_.fireRateMultiplier * upgradeOf(weapon, WeaponUpgrade::FireRate));
}

int Arsenal::magazineOf(int weapon) const
{
    const float size = static_cast<float>(catalog()[static_cast<size_t>(weapon)].magazine);
    const float multiplier = modifiers_.magazineMultiplier * upgradeOf(weapon, WeaponUpgrade::Magazine);
    return std::max(1, static_cast<int>(std::lround(size * multiplier)));
}

int Arsenal::maxReserveOf(int weapon) const
{
    return magazineOf(weapon) * catalog()[static_cast<size_t>(weapon)].reserveMagazines;
}

float Arsenal::reloadProgress() const
{
    const float total = reloadTime();
    return total > 0.0f ? 1.0f - reloadTimer_ / total : 1.0f;
}

void Arsenal::select(int index)
{
    if (index < 0 || index >= static_cast<int>(slots_.size()) || !slots_[index].unlocked || index == current_) return;
    current_ = index;
    reloadTimer_ = 0.0f;
    cooldown_ = std::max(cooldown_, 0.2f);
}

void Arsenal::cycle(int direction)
{
    const int count = static_cast<int>(slots_.size());
    for (int step = 1; step < count; ++step) {
        const int candidate = ((current_ + direction * step) % count + count) % count;
        if (slots_[candidate].unlocked) {
            select(candidate);
            return;
        }
    }
}

void Arsenal::addAmmo(int weapon, int amount)
{
    if (weapon >= 0 && weapon < static_cast<int>(slots_.size())) slots_[weapon].reserve += amount;
}

void Arsenal::unlock(int weapon)
{
    if (weapon < 0 || weapon >= static_cast<int>(slots_.size())) return;
    Slot& slot = slots_[static_cast<size_t>(weapon)];
    slot.unlocked = true;
    slot.magazine = magazineOf(weapon);
    refillReserve(weapon);
}

void Arsenal::refillReserve(int weapon)
{
    if (weapon < 0 || weapon >= static_cast<int>(slots_.size())) return;
    Slot& slot = slots_[static_cast<size_t>(weapon)];
    slot.reserve = std::max(slot.reserve, maxReserveOf(weapon));
}

void Arsenal::upgrade(int weapon, WeaponUpgrade upgrade)
{
    if (weapon < 0 || weapon >= static_cast<int>(slots_.size())) return;
    Slot& slot = slots_[static_cast<size_t>(weapon)];
    int& level = slot.upgrades[static_cast<size_t>(upgrade)];
    if (level >= kMaxUpgradeLevel) return;
    const int before = magazineOf(weapon);
    ++level;
    slot.magazine += magazineOf(weapon) - before;
}

void Arsenal::startReload(EventQueue& events, Vector3 position)
{
    const WeaponStats& weapon = stats();
    const Slot& slot = slots_[current_];
    if (reloadTimer_ > 0.0f || slot.magazine >= magazineSize()) return;
    if (!weapon.infiniteReserve && slot.reserve <= 0) return;
    reloadTimer_ = reloadTime();
    events.push_back({EventType::ReloadStart, position});
}

bool Arsenal::update(float dt, bool fireHeld, bool firePressed, bool reloadPressed, Vector3 muzzle,
                     Vector3 forward, ProjectileSystem& projectiles, EventQueue& events)
{
    const WeaponStats& weapon = stats();
    Slot& slot = slots_[current_];
    cooldown_ = std::max(0.0f, cooldown_ - dt);
    timeSinceShot_ += dt;

    if (reloadTimer_ > 0.0f) {
        reloadTimer_ -= dt;
        if (reloadTimer_ <= 0.0f) {
            reloadTimer_ = 0.0f;
            const int needed = magazineSize() - slot.magazine;
            const int taken = weapon.infiniteReserve ? needed : std::min(needed, slot.reserve);
            slot.magazine += taken;
            if (!weapon.infiniteReserve) slot.reserve -= taken;
        }
        return false;
    }
    if (reloadPressed) {
        startReload(events, muzzle);
        return false;
    }

    const bool wantsToFire = weapon.automatic ? fireHeld : firePressed;
    if (!wantsToFire || cooldown_ > 0.0f) return false;

    if (slot.magazine <= 0) {
        if (firePressed) events.push_back({EventType::DryFire, muzzle});
        startReload(events, muzzle);
        return false;
    }

    // Chargeur sans fond : certains tirs ne consomment pas de balle.
    if (!chance(modifiers_.noAmmoChance)) slot.magazine -= 1;
    cooldown_ = fireIntervalOf(current_);
    timeSinceShot_ = 0.0f;
    const float baseYaw = std::atan2(forward.x, forward.z);
    const int pellets = weapon.pellets * (modifiers_.doubleShot ? 2 : 1);
    const float spreadDegrees = weapon.spreadDegrees + modifiers_.extraSpreadDegrees;
    const float damage = damageOf(current_);
    for (int i = 0; i < pellets; ++i) {
        const float spread = spreadDegrees * DEG2RAD * randomSigned();
        const float yaw = baseYaw + spread;
        const float speed = weapon.projectileSpeed * (pellets > 1 ? 0.9f + 0.2f * std::fabs(randomSigned()) : 1.0f);
        Projectile p;
        p.position = muzzle;
        p.previous = muzzle;
        p.velocity = {std::sin(yaw) * speed, 0.0f, std::cos(yaw) * speed};
        p.damage = damage;
        p.knockback = weapon.knockback;
        p.life = kProjectileLife;
        p.pierce = weapon.pierce + modifiers_.pierceBonus;
        p.ricochets = modifiers_.ricochets;
        p.incendiary = modifiers_.incendiary;
        projectiles.spawn(p);
    }
    events.push_back({EventType::Shot, muzzle, static_cast<float>(current_), forward});
    if (slot.magazine == 0) startReload(events, muzzle);
    return true;
}
