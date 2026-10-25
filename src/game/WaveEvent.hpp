#pragma once

#include "raylib.h"

// Événements de vague (GAME.md §5) : à partir de la vague 3, une vague sur trois
// change les règles, annoncée en grand. Termites (v3) viendra avec la pause v3 (B7).
enum class WaveEvent { None, BloodMoon, Fog, Blackout, Horde, Count };

struct WaveEventInfo {
    const char* title;         // annonce, en capitales
    const char* description;   // sous-titre de l'annonce
    Color color;
    float haste;               // multiplicateur de vitesse des morts
    int goldMultiplier;
};

const WaveEventInfo& waveEventInfo(WaveEvent event);
// Tirage au début d'une vague ; jamais deux fois de suite le même événement.
WaveEvent rollWaveEvent(int wave, WaveEvent previous);

namespace waveEvents {
// Horde : deux fois plus de morts, surtout des marcheurs affaiblis.
constexpr int kHordeQuotaMultiplier = 2;
constexpr int kHordeWalkerPercent = 75;
constexpr float kHordeWalkerHealth = 0.6f;
constexpr float kHordeSpawnInterval = 0.6f;  // les apparitions s'enchaînent plus vite
}
