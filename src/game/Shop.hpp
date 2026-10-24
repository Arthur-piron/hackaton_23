#pragma once

#include "game/Events.hpp"
#include "game/Weapon.hpp"

#include <string>
#include <vector>

class Economy;
class Player;

// Colonnes de l'écran du marchand.
enum class ShopCategory { Weapons, Upgrades, Consumables, Count };

enum class ShopItemKind { Weapon, Upgrade, Heal, Ammo, CardReroll };

// Un article du catalogue (données fixes).
struct ShopItem {
    ShopItemKind kind;
    ShopCategory category;
    const char* name;     // sans accents : la police par défaut de raylib est ASCII
    const char* tagline;  // rôle de l'article, en une ligne
    int prices[Arsenal::kMaxUpgradeLevel]; // prix par niveau (améliorations) ; sinon seul le premier compte
    int weapon;           // arme vendue (Weapon), -1 sinon
    WeaponUpgrade upgrade; // amélioration vendue (Upgrade)
};

// Ce qui empêche (ou non) d'acheter un article.
enum class ShopAvailability {
    Available,
    Owned,       // arme déjà possédée
    Maxed,       // amélioration ou stock au maximum
    Full,        // vie ou réserve déjà pleine
    Unavailable, // sans objet (munitions d'une arme à réserve infinie, arme non possédée)
};

// Un article tel que le joueur le voit maintenant.
struct ShopOffer {
    int price = 0;
    int level = 0;      // niveau actuel (améliorations) ou quantité en stock (relances)
    int maxLevel = 0;   // 0 : pas de pastilles
    ShopAvailability availability = ShopAvailability::Available;
    bool affordable = false;
    std::string details; // description chiffrée (« Degats 34 -> 41 »)
};

// Le marchand : catalogue des articles et achats, payés avec l'or de l'économie.
class Shop {
public:
    static const std::vector<ShopItem>& catalog();
    // Raison affichée quand un achat est refusé.
    static const char* refusalReason(const ShopOffer& offer);

    // Les systèmes doivent survivre au marchand.
    Shop(Economy& economy, Arsenal& arsenal, Player& player);

    // weapon : arme visée par une amélioration (ignorée pour les autres articles).
    ShopOffer offer(int item, int weapon) const;
    // Achète l'article s'il est disponible et abordable : débite l'or et publie ItemBought.
    bool buy(int item, int weapon, EventQueue& events);

private:
    Economy& economy_;
    Arsenal& arsenal_;
    Player& player_;
};
