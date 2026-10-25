#include "game/WaveEvent.hpp"

#include <cstddef>

namespace {
constexpr int kFirstEventWave = 3;
constexpr int kEventOdds = 3;  // une vague sur trois

const WaveEventInfo kInfos[] = {
    {"", "", {255, 255, 255, 255}, 1.0f, 1},
    {"LUNE DE SANG", "Les morts sont plus rapides. Or ×2.", {235, 50, 45, 255}, 1.3f, 2},
    {"BROUILLARD", "On n'y voit plus à 8 mètres...", {175, 185, 205, 255}, 1.0f, 1},
    {"COUPURE", "Les lampadaires s'éteignent.", {120, 150, 255, 255}, 1.0f, 1},
    {"HORDE", "Deux fois plus de morts, mais plus faibles.", {150, 220, 90, 255}, 1.0f, 1},
};
static_assert(sizeof(kInfos) / sizeof(kInfos[0]) == static_cast<size_t>(WaveEvent::Count), "un profil par événement");
}

const WaveEventInfo& waveEventInfo(WaveEvent event)
{
    return kInfos[static_cast<int>(event)];
}

WaveEvent rollWaveEvent(int wave, WaveEvent previous)
{
    if (wave < kFirstEventWave || GetRandomValue(1, kEventOdds) != 1) return WaveEvent::None;
    const int first = static_cast<int>(WaveEvent::BloodMoon);
    const int count = static_cast<int>(WaveEvent::Count) - first;
    int index = GetRandomValue(0, count - 1);
    // Pas deux fois de suite : on prend le suivant dans la liste.
    if (first + index == static_cast<int>(previous)) index = (index + 1) % count;
    return static_cast<WaveEvent>(first + index);
}
