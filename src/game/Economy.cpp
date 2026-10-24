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

void Economy::onPlayerDamaged()
{
    combo_ /= 2;
    if (combo_ == 0) comboTimer_ = 0.0f;
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
