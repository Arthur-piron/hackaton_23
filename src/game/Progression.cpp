#include "game/Progression.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace {
constexpr float kConstitutionHealth = 10.0f;
constexpr int kHeritageGold = 50;
constexpr int kLuckPercent = 1;

using T = TalentId;

const std::vector<TalentInfo>& buildCatalog()
{
    static const std::vector<TalentInfo> talents = {
        {T::Constitution, "constitution", "Constitution", "Vie de départ +10", {5, 10, 15, 20, 25}, true},
        {T::Heritage, "heritage", "Héritage", "Or de départ +50", {5, 10, 15, 20, 25}, true},
        {T::Luck, "luck", "Chance", "Cartes rares et légendaires +1 %", {10, 20, 30}, true},
        {T::Reroll, "reroll", "Relance", "+1 relance de cartes gratuite par partie", {15, 30, 45}, false},
        {T::Armory, "armory", "Arsenal", "Commence avec le fusil à pompe", {40}, false},
        {T::SteadyLight, "steady_light", "Lumière tenace", "Batterie de la lampe +15 %", {10, 20, 30}, false},
        {T::Lucidity, "lucidity", "Lucidité", "4 cartes au choix au lieu de 3", {60}, false},
    };
    return talents;
}
}

const std::vector<TalentInfo>& Progression::talents()
{
    return buildCatalog();
}

const TalentInfo& Progression::info(TalentId id)
{
    return talents()[static_cast<size_t>(id)];
}

Progression::Progression(std::string savePath, bool persistent)
    : file_(std::move(savePath))
    , persistent_(persistent)
{
    load();
}

void Progression::load()
{
    if (!persistent_ || !file_.load()) return;
    bones_ = std::max(0, file_.getInt("bones", 0));
    bestWave_ = file_.getInt("best_wave", 0);
    bestScore_ = file_.getInt("best_score", 0);
    victories_ = file_.getInt("victories", 0);
    for (const TalentInfo& talent : talents()) {
        const int max = static_cast<int>(talent.costs.size());
        ranks_[static_cast<size_t>(talent.id)] = std::clamp(file_.getInt(std::string("talent.") + talent.key, 0), 0, max);
    }
}

void Progression::save()
{
    if (!persistent_) return;
    // file_ garde aussi les clés inconnues (versions futures) : on ne fait que mettre à jour les nôtres.
    file_.setInt("bones", bones_);
    file_.setInt("best_wave", bestWave_);
    file_.setInt("best_score", bestScore_);
    file_.setInt("victories", victories_);
    for (const TalentInfo& talent : talents()) {
        file_.setInt(std::string("talent.") + talent.key, rank(talent.id));
    }
    file_.save();
}

int Progression::nextCost(TalentId id) const
{
    const std::vector<int>& costs = info(id).costs;
    const int r = rank(id);
    return r < static_cast<int>(costs.size()) ? costs[static_cast<size_t>(r)] : -1;
}

bool Progression::canBuy(TalentId id) const
{
    const int cost = nextCost(id);
    return info(id).implemented && cost >= 0 && bones_ >= cost;
}

bool Progression::buy(TalentId id)
{
    if (!canBuy(id)) return false;
    bones_ -= nextCost(id);
    ++ranks_[static_cast<size_t>(id)];
    save();
    return true;
}

int Progression::finishRun(int wavesSurvived, int score, bool victory)
{
    const int earned = std::max(0, wavesSurvived);
    bones_ += earned;
    bestWave_ = std::max(bestWave_, wavesSurvived);
    bestScore_ = std::max(bestScore_, score);
    if (victory) ++victories_;
    save();
    return earned;
}

float Progression::startHealthBonus() const
{
    return kConstitutionHealth * static_cast<float>(rank(TalentId::Constitution));
}

int Progression::startGold() const
{
    return kHeritageGold * rank(TalentId::Heritage);
}

int Progression::rarityBonusPercent() const
{
    return kLuckPercent * rank(TalentId::Luck);
}
