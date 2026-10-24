#pragma once

// Effets cumulés des cartes possédées, recalculés à chaque carte prise.
// Lus par l'arsenal, le joueur, l'économie, les objets au sol et Game (crochets de combat).
struct CardModifiers {
    // Armes
    float damageMultiplier = 1.0f;
    float fireRateMultiplier = 1.0f;   // divise l'intervalle entre deux tirs
    float magazineMultiplier = 1.0f;
    float reloadMultiplier = 1.0f;     // multiplie la durée de rechargement
    float noAmmoChance = 0.0f;         // chance qu'un tir ne consomme pas de balle
    int pierceBonus = 0;
    int ricochets = 0;                 // rebonds d'une balle qui tue
    bool doubleShot = false;           // chaque tir part en double
    float extraSpreadDegrees = 0.0f;
    bool frost = false;                // les balles ralentissent les morts touchés

    // Lampe torche
    float batteryMultiplier = 1.0f;    // capacité de la batterie
    float lightBonusDamage = 0.0f;     // dégâts par seconde ajoutés dans le cône (Lampe UV)

    // Joueur
    float moveSpeedMultiplier = 1.0f;
    float maxHealthBonus = 0.0f;
    float staminaMultiplier = 1.0f;    // capacité d'endurance
    float staminaRegenMultiplier = 1.0f;
    float lifeSteal = 0.0f;            // fraction de la vie max rendue à chaque mort
    bool lastStand = false;            // survit à un coup mortel une fois par vague

    // Économie
    float goldMultiplier = 1.0f;
    float magnetMultiplier = 1.0f;
    bool deadMansFortune = false;      // pluie de pièces au combo ×5

    // Crochets de combat
    float critChance = 0.0f;
    bool incendiary = false;
    bool fireChain = false;            // les rebonds enflamment et propagent le feu
    float explosiveCorpseChance = 0.0f;
    bool reaper = false;               // exécute les ennemis presque morts
    bool pocketSun = false;            // flash brûlant périodique autour du joueur
};
