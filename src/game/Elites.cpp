#include "game/Elites.hpp"

#include "game/Enemy.hpp"

#include <cstddef>

namespace {
constexpr int kFirstEliteWave = 3;
// Chance d'élite en pour mille : 5 % à la vague 3, +1 % par vague.
constexpr int kBaseChance = 50;
constexpr int kChancePerWave = 10;

const EliteInfo kInfos[] = {
    {"", "", {255, 255, 255, 255}},
    {"explosif", "explosive", {255, 140, 30, 255}},
    {"blindé", "blindée", {170, 190, 215, 255}},
    {"rapide", "rapide", {255, 225, 50, 255}},
    {"régénérant", "régénérante", {90, 255, 110, 255}},
};
static_assert(sizeof(kInfos) / sizeof(kInfos[0]) == static_cast<size_t>(EliteKind::Count), "un profil par élite");
}

const EliteInfo& eliteInfo(EliteKind kind)
{
    return kInfos[static_cast<int>(kind)];
}

std::string eliteName(EnemyType type, EliteKind kind)
{
    const EliteInfo& info = eliteInfo(kind);
    // Seule la Brute est féminine parmi les morts.
    const char* adjective = type == EnemyType::Brute ? info.feminine : info.masculine;
    return std::string(enemyStats(type).name) + " " + adjective;
}

EliteKind rollElite(int wave, float chanceMultiplier)
{
    if (wave < kFirstEliteWave) return EliteKind::None;
    const float perMille = static_cast<float>(kBaseChance + kChancePerWave * (wave - kFirstEliteWave)) * chanceMultiplier;
    if (GetRandomValue(1, 1000) > static_cast<int>(perMille)) return EliteKind::None;
    const int first = static_cast<int>(EliteKind::Explosive);
    const int last = static_cast<int>(EliteKind::Count) - 1;
    return static_cast<EliteKind>(GetRandomValue(first, last));
}
