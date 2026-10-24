#include "game/Shop.hpp"

#include "game/Economy.hpp"
#include "game/Player.hpp"
#include "raylib.h"

#include <cmath>

namespace {
constexpr float kHealAmount = 50.0f;
constexpr int kMaxCardRerolls = 3;

using K = ShopItemKind;
using C = ShopCategory;
using U = WeaponUpgrade;

int rounded(float value)
{
    return static_cast<int>(std::lround(value));
}

int percent(WeaponUpgrade upgrade)
{
    return rounded((Arsenal::upgradeMultiplier(upgrade, 1) - 1.0f) * 100.0f);
}

// Valeur affichée d'une amélioration pour une arme (dégâts, tirs par seconde, balles).
std::string upgradeValue(const Arsenal& arsenal, int weapon, WeaponUpgrade upgrade)
{
    switch (upgrade) {
    case U::Damage: return TextFormat("%d", rounded(arsenal.damageOf(weapon)));
    case U::FireRate: return TextFormat("%.1f", 1.0f / arsenal.fireIntervalOf(weapon));
    case U::Magazine: return TextFormat("%d", arsenal.magazineOf(weapon));
    case U::Count: break;
    }
    return "";
}

const char* upgradeUnit(WeaponUpgrade upgrade)
{
    switch (upgrade) {
    case U::Damage: return "degats";
    case U::FireRate: return "tirs/s";
    case U::Magazine: return "balles";
    case U::Count: break;
    }
    return "";
}

// Fiche d'une arme : dégâts, plombs, chargeur, perforation.
std::string weaponDetails(const Arsenal& arsenal, int weapon)
{
    const WeaponStats& stats = Arsenal::catalog()[static_cast<size_t>(weapon)];
    const int damage = rounded(arsenal.damageOf(weapon));
    std::string text = stats.pellets > 1 ? TextFormat("%d plombs x %d degats", stats.pellets, damage)
                                         : TextFormat("%d degats", damage);
    text += TextFormat(", %d coups", arsenal.magazineOf(weapon));
    if (stats.pierce > 0) text += TextFormat(", perfore %d", stats.pierce);
    if (stats.automatic) text += ", auto";
    return text;
}
}

const std::vector<ShopItem>& Shop::catalog()
{
    static const std::vector<ShopItem> items = {
        {K::Weapon, C::Weapons, "Fusil a pompe", "Controle de foule, gros recul", {400, 0, 0}, 1, U::Damage},
        {K::Weapon, C::Weapons, "Mitraillette", "Arrose les hordes sans relacher", {650, 0, 0}, 2, U::Damage},
        {K::Weapon, C::Weapons, "Fusil de chasse", "Une balle, toute une rangee", {900, 0, 0}, 3, U::Damage},
        {K::Upgrade, C::Upgrades, "Degats", "Balles plus lourdes", {200, 450, 800}, -1, U::Damage},
        {K::Upgrade, C::Upgrades, "Cadence", "Mecanisme huile", {200, 450, 800}, -1, U::FireRate},
        {K::Upgrade, C::Upgrades, "Chargeur", "Chargeur allonge", {150, 350, 600}, -1, U::Magazine},
        {K::Heal, C::Consumables, "Soin", "Bandages et eau benite", {120, 0, 0}, -1, U::Damage},
        {K::Ammo, C::Consumables, "Munitions", "Remplit la reserve de l'arme en main", {100, 0, 0}, -1, U::Damage},
        {K::CardReroll, C::Consumables, "Relance des cartes", "Retire 3 cartes au prochain niveau", {150, 0, 0},
         -1, U::Damage},
    };
    return items;
}

const char* Shop::refusalReason(const ShopOffer& offer)
{
    switch (offer.availability) {
    case ShopAvailability::Owned: return "Deja possede";
    case ShopAvailability::Maxed: return "Deja au maximum";
    case ShopAvailability::Full: return "Deja plein";
    case ShopAvailability::Unavailable: return "Indisponible";
    case ShopAvailability::Available: break;
    }
    return offer.affordable ? "" : "Pas assez d'or";
}

Shop::Shop(Economy& economy, Arsenal& arsenal, Player& player)
    : economy_(economy)
    , arsenal_(arsenal)
    , player_(player)
{
}

ShopOffer Shop::offer(int index, int weapon) const
{
    const ShopItem& item = catalog()[static_cast<size_t>(index)];
    ShopOffer o;
    o.price = item.prices[0];
    switch (item.kind) {
    case K::Weapon:
        o.details = weaponDetails(arsenal_, item.weapon);
        if (arsenal_.isUnlocked(item.weapon)) o.availability = ShopAvailability::Owned;
        break;
    case K::Upgrade: {
        if (weapon < 0 || !arsenal_.isUnlocked(weapon)) {
            o.availability = ShopAvailability::Unavailable;
            break;
        }
        o.level = arsenal_.upgradeLevel(weapon, item.upgrade);
        o.maxLevel = Arsenal::kMaxUpgradeLevel;
        const std::string now = upgradeValue(arsenal_, weapon, item.upgrade);
        if (o.level >= o.maxLevel) {
            o.availability = ShopAvailability::Maxed;
            o.price = 0;
            o.details = TextFormat("%s %s (max)", now.c_str(), upgradeUnit(item.upgrade));
            break;
        }
        o.price = item.prices[o.level];
        // Aperçu exact : on applique l'amélioration à une copie de l'arsenal (cartes comprises).
        Arsenal preview = arsenal_;
        preview.upgrade(weapon, item.upgrade);
        o.details = TextFormat("+%d %% : %s -> %s %s", percent(item.upgrade), now.c_str(),
                               upgradeValue(preview, weapon, item.upgrade).c_str(), upgradeUnit(item.upgrade));
        break;
    }
    case K::Heal:
        o.details = TextFormat("+%d PV   (vie %d / %d)", rounded(kHealAmount), rounded(std::ceil(player_.health())),
                               rounded(player_.maxHealth()));
        if (player_.health() >= player_.maxHealth()) o.availability = ShopAvailability::Full;
        break;
    case K::Ammo: {
        const int current = arsenal_.current();
        const WeaponStats& stats = Arsenal::catalog()[static_cast<size_t>(current)];
        if (stats.infiniteReserve) {
            o.details = TextFormat("%s : munitions infinies", stats.name.c_str());
            o.availability = ShopAvailability::Unavailable;
            break;
        }
        o.details = TextFormat("%s : reserve %d / %d", stats.name.c_str(), arsenal_.reserveOf(current),
                               arsenal_.maxReserveOf(current));
        if (arsenal_.reserveOf(current) >= arsenal_.maxReserveOf(current)) o.availability = ShopAvailability::Full;
        break;
    }
    case K::CardReroll:
        o.level = economy_.cardRerolls();
        o.maxLevel = kMaxCardRerolls;
        o.details = TextFormat("En stock : %d / %d", o.level, o.maxLevel);
        if (o.level >= o.maxLevel) o.availability = ShopAvailability::Maxed;
        break;
    }
    o.affordable = economy_.gold() >= o.price;
    return o;
}

bool Shop::buy(int index, int weapon, EventQueue& events)
{
    const ShopOffer o = offer(index, weapon);
    if (o.availability != ShopAvailability::Available || !economy_.spend(o.price)) return false;

    const ShopItem& item = catalog()[static_cast<size_t>(index)];
    switch (item.kind) {
    case K::Weapon:
        arsenal_.unlock(item.weapon);
        arsenal_.select(item.weapon);
        break;
    case K::Upgrade:
        arsenal_.upgrade(weapon, item.upgrade);
        break;
    case K::Heal:
        player_.heal(kHealAmount);
        break;
    case K::Ammo:
        arsenal_.refillReserve(arsenal_.current());
        break;
    case K::CardReroll:
        economy_.addCardReroll();
        break;
    }
    events.push_back({EventType::ItemBought, player_.position(), static_cast<float>(index)});
    return true;
}
