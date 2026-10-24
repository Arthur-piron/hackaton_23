#pragma once

// Personnages jouables, achetés avec les ossements (docs/GAME.md §6).
enum class CharacterId { Keeper, Vampire, Skeleton, Ghost, Count };

struct CharacterInfo {
    const char* key;          // clé dans la sauvegarde
    const char* name;
    const char* description;
    const char* model;        // relatif à assets/models/graveyard/
    int cost;                 // en ossements (0 : disponible dès le départ)
    float speed;              // multiplicateur de vitesse
    float healthBonus;        // PV max en plus (ou en moins)
    float lifeSteal;          // fraction de la vie max rendue à chaque mort
    float damage;             // multiplicateur des dégâts
    float battery;            // multiplicateur de la batterie de la lampe
    float staminaCapacity;    // multiplicateur de l'endurance
    float staminaRegen;       // multiplicateur de la récupération d'endurance
};

inline const CharacterInfo& characterInfo(CharacterId id)
{
    static const CharacterInfo characters[] = {
        {"keeper", "Gardien", "Équilibré. Le dernier vivant du cimetière.", "character-keeper.glb", 0, 1.0f, 0.0f,
         0.0f, 1.0f, 1.0f, 1.0f, 1.0f},
        {"vampire", "Vampire", "Vitesse +20 %, 2 % de vie par mort, mais sa lampe tient moitié moins.",
         "character-vampire.glb", 50, 1.2f, 0.0f, 0.02f, 1.0f, 0.5f, 1.0f, 1.0f},
        {"skeleton", "Squelette", "Dégâts +20 %, mais vie -20.", "character-skeleton.glb", 80, 1.0f, -20.0f, 0.0f,
         1.2f, 1.0f, 1.0f, 1.0f},
        {"ghost", "Spectre", "Sprint presque sans fatigue et vitesse +10 %, mais vie -25.", "character-ghost.glb", 120,
         1.1f, -25.0f, 0.0f, 1.0f, 1.0f, 4.0f, 3.0f},
    };
    return characters[static_cast<int>(id)];
}
