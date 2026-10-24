#pragma once

#include "raylib.h"

#include <vector>

// Événements de gameplay produits pendant une image et consommés ensuite par
// le son, les particules, la caméra et le HUD. Les systèmes de jeu n'ont ainsi
// pas besoin de connaître ces modules.
enum class EventType {
    Shot,          // value = indice de l'arme
    DryFire,
    ReloadStart,
    EnemyHit,      // value = dégâts
    EnemyKilled,   // value = points
    EnemyAttack,
    EnemyGroan,
    PlayerHurt,    // value = dégâts
    PlayerDied,
    WaveStarted,   // value = numéro de vague
    WaveCleared,   // value = numéro de vague
    GoldPicked,    // value = or ramassé
    XpPicked,      // value = XP ramassée
    LevelUp,       // value = nouveau niveau
    GateOpened,    // value = indice de la zone débloquée
    BulletImpact,  // balle arrêtée par le décor ; direction = sens de la balle
    Explosion,     // value = rayon (cadavre nécrotique, flash du Soleil de poche...)
    LastStand,     // le Dernier rempart a sauvé le joueur
    ItemBought,    // achat chez le marchand ; value = indice de l'article (Shop::catalog)
    FlashlightSwitched, // value = 1 allumée, 0 éteinte, -1 refus (batterie vide)
    FlashlightFlicker,  // la lampe hoquette : batterie presque vide
    BatteryPicked,      // pile de lampe ramassée
    MysteryBoxOpened,   // tirage payé à la boîte mystère
    MysteryBoxTick,     // l'arme affichée change pendant le tirage
    MysteryBoxReady,    // l'arme tirée flotte au-dessus de la boîte ; value = 1 si légendaire
    MysteryBoxMoved,    // la boîte s'envole (position de départ) vers un autre emplacement
    AltarUsed,          // prière à l'autel de la chapelle : soins complets
    CrateLanded,        // une caisse touche le sol
    CrateBroken,        // value = CrateKind
    ResourcePicked,     // value = quantité, direction.x = Resource
    BuildDenied,        // pose refusée, ou mode construction hors des pauses
    StructurePlaced,    // value = StructureKind
    StructureRemoved,   // revente ou annulation
    StructureDamaged,   // un mort frappe une structure ; value = StructureKind
    StructureDestroyed, // value = StructureKind
};

// Nature d'un coup porté (EnemyHit) : change la couleur du chiffre de dégâts et le son.
// Light : brûlure de la lampe (ni sang, ni son de chair).
enum class HitKind { Normal, Critical, Burn, Light };

struct Event {
    EventType type;
    Vector3 position = {0.0f, 0.0f, 0.0f};
    float value = 0.0f;
    Vector3 direction = {0.0f, 0.0f, 0.0f};
    HitKind hit = HitKind::Normal;
};

using EventQueue = std::vector<Event>;
