#include "game/Economy.hpp"

#include <algorithm>
#include <cmath>

namespace {
// Or et XP lâchés par type d'ennemi (dans l'ordre d'EnemyType).
const Economy::Reward kKillRewards[] = {
    {5, 3},   // Marcheur
    {6, 4},   // Coureur
    {7, 5},   // Squelette
    {30, 20}, // Brute
    {10, 7},  // Vampire
    {9, 6},   // Spectre
    {150, 100}, // Le Fossoyeur
};
static_assert(sizeof(kKillRewards) / sizeof(kKillRewards[0]) == static_cast<size_t>(EnemyType::Count),
              "une récompense par type d'ennemi");

// Paliers du combo : à partir de kComboSteps[i] morts, l'or est multiplié par i + 2.
constexpr int kComboSteps[] = {5, 10, 20, 35};
}

void Economy::reset()
{
    *this = Economy{};
}

void Economy::update(float dt)
{
    if (combo_ == 0) return;
    comboTimer_ -= dt;
    if (comboTimer_ <= 0.0f) {
        combo_ = 0;
        comboTimer_ = 0.0f;
    }
}

Economy::Reward Economy::addKill(EnemyType type)
{
    ++combo_;
    comboTimer_ = kComboWindow;
    Reward reward = kKillRewards[static_cast<int>(type)];
    reward.gold = static_cast<int>(std::lround(static_cast<float>(reward.gold * comboMultiplier()) * goldMultiplier_));
    return reward;
}

Economy::Reward Economy::addDefenseKill(EnemyType type)
{
    Reward reward = kKillRewards[static_cast<int>(type)];
    reward.gold = static_cast<int>(std::lround(static_cast<float>(reward.gold) * goldMultiplier_));
    reward.xp /= 2;
    return reward;
}

void Economy::onPlayerDamaged()
{
    combo_ /= 2;
    if (combo_ == 0) comboTimer_ = 0.0f;
}

Economy::Salvage Economy::rollSalvage(EnemyType type)
{
    // Chance et quantité de bois puis de ferraille, dans l'ordre d'EnemyType.
    struct Drop {
        float woodChance;
        int wood;
        float scrapChance;
        int scrap;
    };
    static constexpr Drop kDrops[] = {
        {0.25f, 1, 0.0f, 0},  // marcheur
        {0.0f, 0, 0.15f, 1},  // coureur
        {0.10f, 1, 0.30f, 1}, // squelette
        {1.0f, 3, 0.60f, 2},  // brute
        {0.0f, 0, 0.35f, 1},  // vampire
        {0.0f, 0, 0.0f, 0},   // spectre
        {1.0f, 8, 1.0f, 6},   // le Fossoyeur
    };
    static_assert(sizeof(kDrops) / sizeof(kDrops[0]) == static_cast<size_t>(EnemyType::Count),
                  "un butin par type d'ennemi");
    const Drop& drop = kDrops[static_cast<size_t>(type)];
    Salvage salvage;
    if (static_cast<float>(GetRandomValue(0, 9999)) < drop.woodChance * 10000.0f) salvage.wood = drop.wood;
    if (static_cast<float>(GetRandomValue(0, 9999)) < drop.scrapChance * 10000.0f) salvage.scrap = drop.scrap;
    return salvage;
}

int Economy::resourceCap(Resource resource)
{
    static constexpr int kCaps[] = {200, 120, 6};
    static_assert(sizeof(kCaps) / sizeof(kCaps[0]) == static_cast<size_t>(Resource::Count), "un plafond par ressource");
    return kCaps[static_cast<size_t>(resource)];
}

void Economy::addResource(Resource resource, int amount)
{
    int& stock = resources_[static_cast<size_t>(resource)];
    stock = std::min(resourceCap(resource), stock + amount);
}

bool Economy::spendResource(Resource resource, int amount)
{
    int& stock = resources_[static_cast<size_t>(resource)];
    if (amount > stock) return false;
    stock -= amount;
    return true;
}

void Economy::addGold(int amount)
{
    gold_ += amount;
}

void Economy::addXp(int amount, EventQueue& events)
{
    xp_ += amount;
    while (xp_ >= xpToNextLevel()) {
        xp_ -= xpToNextLevel();
        ++level_;
        ++pendingLevelUps_;
        events.push_back({EventType::LevelUp, {0.0f, 0.0f, 0.0f}, static_cast<float>(level_)});
    }
}

bool Economy::spend(int amount)
{
    if (amount > gold_) return false;
    gold_ -= amount;
    return true;
}

int Economy::xpToNextLevel() const
{
    return 20 + 15 * level_;
}

float Economy::xpProgress() const
{
    return std::clamp(static_cast<float>(xp_) / static_cast<float>(xpToNextLevel()), 0.0f, 1.0f);
}

int Economy::comboMultiplier() const
{
    int multiplier = 1;
    for (int step : kComboSteps) {
        if (combo_ >= step) ++multiplier;
    }
    return multiplier;
}

bool Economy::consumeLevelUp()
{
    if (pendingLevelUps_ == 0) return false;
    --pendingLevelUps_;
    return true;
}

bool Economy::useCardReroll()
{
    if (cardRerolls_ == 0) return false;
    --cardRerolls_;
    return true;
}
