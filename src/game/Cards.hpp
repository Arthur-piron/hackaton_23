#pragma once

#include "game/CardModifiers.hpp"

#include <array>
#include <cstddef>
#include <vector>

enum class CardId {
    SteadyHand,      // Main sûre
    NervousTrigger,  // Gâchette nerveuse
    WolfStep,        // Pas de loup
    ToughSkin,       // Peau dure
    DeepPockets,     // Poches profondes
    QuickHands,      // Main leste
    Miser,           // Avare
    Magnet,          // Aimant
    LongBattery,     // Pile longue durée
    SecondWind,      // Second souffle
    Incendiary,      // Balles incendiaires
    Ricochet,        // Ricochet
    Piercing,        // Perforation
    UvLamp,          // Lampe UV
    Vampirism,       // Vampirisme
    BottomlessMag,   // Chargeur sans fond
    FlyingLantern,   // Lanterne volante
    NecroticBlast,   // Explosion nécrotique
    Frost,           // Gel
    CriticalHit,     // Coup critique
    LastStand,       // Dernier rempart
    FireChain,       // Chaîne de feu
    PocketSun,       // Soleil de poche
    LeadRain,        // Pluie de plomb
    Reaper,          // Faucheuse
    DeadMansFortune, // Fortune du mort
    Count,
};

enum class CardRarity { Common, Rare, Legendary };

// Famille d'une carte : donne la couleur et le pictogramme à l'écran.
enum class CardCategory { Weapon, Survival, Mobility, Fortune, Light, Occult };

struct CardInfo {
    CardId id;
    const char* name;        // sans accents : la police par défaut de raylib est ASCII
    const char* shortName;   // 3 lettres pour les pastilles du HUD
    const char* description; // effet d'un rang, avec les chiffres
    CardRarity rarity;
    CardCategory category;
    int maxRank;
    bool implemented;        // une carte non implémentée n'est jamais tirée
    std::vector<CardId> prerequisites; // synergies : cartes à posséder avant
};

// Catalogue des cartes, tirage au sort à la montée de niveau et rangs possédés.
class Cards {
public:
    static constexpr int kChoices = 3;

    static const std::vector<CardInfo>& catalog();
    static const CardInfo& info(CardId id);

    void reset();
    // Tire jusqu'à count cartes distinctes selon leur rareté, parmi celles encore disponibles.
    std::vector<CardId> drawChoices(int count) const;
    void take(CardId id);

    int rank(CardId id) const { return ranks_[static_cast<size_t>(id)]; }
    bool has(CardId id) const { return rank(id) > 0; }
    const CardModifiers& modifiers() const { return modifiers_; }
    // Cartes possédées, dans l'ordre où elles ont été prises la première fois.
    const std::vector<CardId>& owned() const { return owned_; }

private:
    bool isAvailable(const CardInfo& card) const;
    void recompute();

    std::array<int, static_cast<size_t>(CardId::Count)> ranks_{};
    std::vector<CardId> owned_;
    CardModifiers modifiers_;
};
