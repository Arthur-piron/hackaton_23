#include "game/Cards.hpp"

#include "raylib.h"

#include <algorithm>
#include <cstddef>

namespace {
// Chances de rareté à chaque tirage (en %), le reste est commun.
constexpr int kLegendaryChance = 5;
constexpr int kRareChance = 25;

constexpr float kSteadyHandDamage = 0.12f;
constexpr float kNervousTriggerRate = 0.10f;
constexpr float kWolfStepSpeed = 0.15f;
constexpr float kToughSkinHealth = 25.0f;
constexpr float kDeepPocketsMagazine = 0.25f;
constexpr float kQuickHandsReload = 0.15f;
constexpr float kMiserGold = 0.15f;
constexpr float kMagnetRadius = 0.5f;
constexpr float kSecondWindStamina = 0.30f;
constexpr float kSecondWindRegen = 0.20f;
constexpr float kVampirismHeal = 0.01f;
constexpr float kBottomlessChance = 0.10f;
constexpr float kNecroticChance = 0.20f;
constexpr float kCriticalChance = 0.10f;
constexpr float kLeadRainSpread = 5.0f;
constexpr float kLongBatteryCapacity = 0.30f;
constexpr float kUvLampDamage = 15.0f;

using C = CardId;
using R = CardRarity;
using K = CardCategory;

const std::vector<CardInfo>& buildCatalog()
{
    static const std::vector<CardInfo> cards = {
        {C::SteadyHand, "Main sûre", "DMG", "Dégâts +12 %", R::Common, K::Weapon, 3, true, {}},
        {C::NervousTrigger, "Gâchette nerveuse", "CAD", "Cadence de tir +10 %", R::Common, K::Weapon, 3, true, {}},
        {C::WolfStep, "Pas de loup", "VIT", "Vitesse de déplacement +15 %", R::Common, K::Mobility, 3, true, {}},
        {C::ToughSkin, "Peau dure", "PV", "Vie max +25 et soigne 25 PV", R::Common, K::Survival, 3, true, {}},
        {C::DeepPockets, "Poches profondes", "CHG", "Chargeurs +25 %", R::Common, K::Weapon, 3, true, {}},
        {C::QuickHands, "Main leste", "REC", "Rechargement 15 % plus rapide", R::Common, K::Weapon, 3, true, {}},
        {C::Miser, "Avare", "OR", "Or gagné +15 %", R::Common, K::Fortune, 3, true, {}},
        {C::Magnet, "Aimant", "AIM", "Rayon d'aimantation de l'or et de l'XP +50 %", R::Common, K::Fortune, 2, true,
         {}},
        {C::LongBattery, "Pile longue durée", "BAT", "Batterie de la lampe +30 %", R::Common, K::Light, 3, true, {}},
        {C::SecondWind, "Second souffle", "END", "Endurance +30 %, récupération +20 %", R::Common, K::Mobility, 2,
         true, {}},
        {C::Incendiary, "Balles incendiaires", "FEU", "Les balles enflamment : 6 dégâts/s pendant 3 s", R::Rare,
         K::Weapon, 1, true, {}},
        {C::Ricochet, "Ricochet", "RIC", "Une balle qui tue rebondit vers l'ennemi le plus proche (8 m)", R::Rare,
         K::Weapon, 2, true, {}},
        {C::Piercing, "Perforation", "PER", "Les balles traversent +1 ennemi", R::Rare, K::Weapon, 2, true, {}},
        {C::UvLamp, "Lampe UV", "UV", "Le cône de lumière inflige 15 dégâts/s", R::Rare, K::Light, 2, true, {}},
        {C::Vampirism, "Vampirisme", "VAM", "Chaque mort rend 1 % de la vie max", R::Rare, K::Occult, 3, true, {}},
        {C::BottomlessMag, "Chargeur sans fond", "INF", "10 % de chances qu'un tir ne consomme pas de balle",
         R::Rare, K::Weapon, 3, true, {}},
        {C::FlyingLantern, "Lanterne volante", "LAN", "Une lanterne orbite autour de toi, éclaire et brûle",
         R::Rare, K::Light, 2, true, {}},
        {C::NecroticBlast, "Explosion nécrotique", "NEC", "20 % des morts explosent (2,5 m, 40 dégâts)", R::Rare,
         K::Occult, 2, true, {}},
        {C::Frost, "Gel", "GEL", "Les balles ralentissent de 30 % pendant 1,5 s", R::Rare, K::Weapon, 1, true, {}},
        {C::CriticalHit, "Coup critique", "CRT", "10 % de chances d'infliger x3 dégâts", R::Rare, K::Weapon, 3, true,
         {}},
        {C::LastStand, "Dernier rempart", "REM",
         "Une fois par vague, un coup mortel te laisse à 1 PV et repousse tout le monde", R::Legendary, K::Survival,
         1, true, {}},
        {C::FireChain, "Chaîne de feu", "CHN", "Les rebonds enflamment et le feu se propage à 3 cibles",
         R::Legendary, K::Occult, 1, true, {C::Incendiary, C::Ricochet}},
        {C::PocketSun, "Soleil de poche", "SOL", "Toutes les 20 s, un flash brûle tout dans un rayon de 8 m",
         R::Legendary, K::Light, 1, true, {}},
        {C::LeadRain, "Pluie de plomb", "x2", "Chaque tir part en double, dispersion +5 degrés", R::Legendary,
         K::Weapon, 1, true, {}},
        {C::Reaper, "Faucheuse", "FAU", "Les ennemis sous 15 % de vie meurent au contact d'une balle", R::Legendary,
         K::Occult, 1, true, {}},
        {C::DeadMansFortune, "Fortune du mort", "FOR", "Chaque combo x5 fait pleuvoir 50 pièces", R::Legendary,
         K::Fortune, 1, true, {}},
    };
    return cards;
}

// bonusPercent (talent Chance) s'ajoute aux chances légendaire et rare.
CardRarity rollRarity(int bonusPercent)
{
    const int roll = GetRandomValue(1, 100);
    const int legendary = kLegendaryChance + bonusPercent;
    if (roll <= legendary) return CardRarity::Legendary;
    if (roll <= legendary + kRareChance + bonusPercent) return CardRarity::Rare;
    return CardRarity::Common;
}
}

const std::vector<CardInfo>& Cards::catalog()
{
    return buildCatalog();
}

const CardInfo& Cards::info(CardId id)
{
    return catalog()[static_cast<size_t>(id)];
}

void Cards::reset()
{
    ranks_.fill(0);
    owned_.clear();
    recompute();
}

bool Cards::isAvailable(const CardInfo& card) const
{
    if (!card.implemented || rank(card.id) >= card.maxRank) return false;
    return std::all_of(card.prerequisites.begin(), card.prerequisites.end(), [this](CardId id) { return has(id); });
}

std::vector<CardId> Cards::drawChoices(int count) const
{
    std::vector<CardId> pool;
    for (const CardInfo& card : catalog()) {
        if (isAvailable(card)) pool.push_back(card.id);
    }

    std::vector<CardId> choices;
    while (static_cast<int>(choices.size()) < count && !pool.empty()) {
        // On tire d'abord une rareté ; si plus aucune carte de cette rareté n'est disponible, n'importe laquelle.
        const CardRarity rarity = rollRarity(rarityBonus_);
        std::vector<size_t> matching;
        for (size_t i = 0; i < pool.size(); ++i) {
            if (info(pool[i]).rarity == rarity) matching.push_back(i);
        }
        const size_t pick = matching.empty()
            ? static_cast<size_t>(GetRandomValue(0, static_cast<int>(pool.size()) - 1))
            : matching[static_cast<size_t>(GetRandomValue(0, static_cast<int>(matching.size()) - 1))];
        choices.push_back(pool[pick]);
        pool.erase(pool.begin() + static_cast<std::ptrdiff_t>(pick));
    }
    return choices;
}

void Cards::take(CardId id)
{
    int& r = ranks_[static_cast<size_t>(id)];
    if (r >= info(id).maxRank) return;
    if (r == 0) owned_.push_back(id);
    ++r;
    recompute();
}

void Cards::recompute()
{
    CardModifiers m;
    const auto r = [this](CardId id) { return static_cast<float>(rank(id)); };
    m.damageMultiplier = 1.0f + kSteadyHandDamage * r(C::SteadyHand);
    m.fireRateMultiplier = 1.0f + kNervousTriggerRate * r(C::NervousTrigger);
    m.moveSpeedMultiplier = 1.0f + kWolfStepSpeed * r(C::WolfStep);
    m.maxHealthBonus = kToughSkinHealth * r(C::ToughSkin);
    m.magazineMultiplier = 1.0f + kDeepPocketsMagazine * r(C::DeepPockets);
    m.reloadMultiplier = 1.0f - kQuickHandsReload * r(C::QuickHands);
    m.goldMultiplier = 1.0f + kMiserGold * r(C::Miser);
    m.magnetMultiplier = 1.0f + kMagnetRadius * r(C::Magnet);
    m.staminaMultiplier = 1.0f + kSecondWindStamina * r(C::SecondWind);
    m.staminaRegenMultiplier = 1.0f + kSecondWindRegen * r(C::SecondWind);
    m.incendiary = has(C::Incendiary);
    m.ricochets = rank(C::Ricochet);
    m.pierceBonus = rank(C::Piercing);
    m.lifeSteal = kVampirismHeal * r(C::Vampirism);
    m.noAmmoChance = kBottomlessChance * r(C::BottomlessMag);
    m.explosiveCorpseChance = kNecroticChance * r(C::NecroticBlast);
    m.critChance = kCriticalChance * r(C::CriticalHit);
    m.lastStand = has(C::LastStand);
    m.fireChain = has(C::FireChain);
    m.pocketSun = has(C::PocketSun);
    m.doubleShot = has(C::LeadRain);
    m.extraSpreadDegrees = m.doubleShot ? kLeadRainSpread : 0.0f;
    m.reaper = has(C::Reaper);
    m.deadMansFortune = has(C::DeadMansFortune);
    m.frost = has(C::Frost);
    m.batteryMultiplier = 1.0f + kLongBatteryCapacity * r(C::LongBattery);
    m.lightBonusDamage = kUvLampDamage * r(C::UvLamp);
    m.lanterns = rank(C::FlyingLantern);
    modifiers_ = m;
}
