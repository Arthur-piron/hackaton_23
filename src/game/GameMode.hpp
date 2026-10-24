#pragma once

// Modes de jeu (docs/GAME.md §5) : la Nuit se gagne à l'aube (vague 10), le Sans fin
// se joue au score, le Cauchemar se débloque après une première victoire.
enum class GameMode { Night, Endless, Nightmare, Count };

struct GameModeRules {
    const char* name;
    const char* description;
    bool endless;              // pas de victoire : on joue au score
    float enemyHealth;         // multiplicateur de vie des morts
    float gold;                // multiplicateur de l'or gagné
    int victoriesToUnlock;     // victoires nécessaires pour débloquer le mode
};

inline const GameModeRules& gameModeRules(GameMode mode)
{
    static const GameModeRules rules[] = {
        {"Nuit", "10 vagues. Tiens jusqu'à l'aube.", false, 1.0f, 1.0f, 0},
        {"Sans fin", "Les vagues ne s'arrêtent jamais. Bats ton record.", true, 1.0f, 1.0f, 0},
        {"Cauchemar", "10 vagues, morts +50 % de vie, or -25 %.", false, 1.5f, 0.75f, 1},
    };
    return rules[static_cast<int>(mode)];
}
