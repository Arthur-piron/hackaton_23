#pragma once

#include "game/Enemy.hpp"
#include "game/Events.hpp"

#include <array>

// Ressources de construction (docs/GAME.md §7.1) : bois, ferraille, bidons de carburant.
enum class Resource { Wood, Scrap, Fuel, Count };

// Or, XP, niveaux, combo et ressources du joueur pendant une partie.
// L'or et l'XP d'une mort ne sont crédités qu'au ramassage (voir Pickups).
class Economy {
public:
    // Délai maximal entre deux morts pour enchaîner le combo.
    static constexpr float kComboWindow = 2.0f;

    struct Reward {
        int gold = 0; // déjà multiplié par le combo
        int xp = 0;
    };
    // Bois et ferraille lâchés par une mort (pas de multiplicateur de combo).
    struct Salvage {
        int wood = 0;
        int scrap = 0;
    };

    // Tire au hasard le bois et la ferraille d'une mort (table de GAME.md §7.1).
    static Salvage rollSalvage(EnemyType type);
    static int resourceCap(Resource resource);

    void reset();
    void update(float dt);

    // Enregistre une mort : fait progresser le combo et renvoie ce qu'elle lâche.
    Reward addKill(EnemyType type);
    // Prendre un coup divise le combo par deux.
    void onPlayerDamaged();

    // Bonus d'or des cartes, appliqué à l'or lâché par les morts.
    void setGoldMultiplier(float multiplier) { goldMultiplier_ = multiplier; }
    void addGold(int amount);
    // Publie un événement LevelUp par niveau gagné.
    void addXp(int amount, EventQueue& events);
    // Débite l'or si le joueur en a assez.
    bool spend(int amount);
    // Ajoute une ressource, dans la limite de son plafond.
    void addResource(Resource resource, int amount);
    // Débite une ressource si le joueur en a assez.
    bool spendResource(Resource resource, int amount);
    int resource(Resource resource) const { return resources_[static_cast<size_t>(resource)]; }

    int gold() const { return gold_; }
    // XP accumulée dans le niveau courant.
    int xp() const { return xp_; }
    int xpToNextLevel() const;
    float xpProgress() const;
    int level() const { return level_; }

    int combo() const { return combo_; }
    int comboMultiplier() const;
    // Secondes restantes avant la fin du combo (0 si aucun combo).
    float comboTimeLeft() const { return comboTimer_; }

    // Niveaux gagnés dont la carte n'a pas encore été choisie.
    int pendingLevelUps() const { return pendingLevelUps_; }
    bool consumeLevelUp();

    // Relances de cartes achetées chez le marchand, utilisables aux montées de niveau suivantes.
    void addCardReroll() { ++cardRerolls_; }
    bool useCardReroll();
    int cardRerolls() const { return cardRerolls_; }

private:
    int gold_ = 0;
    int xp_ = 0;
    int level_ = 1;
    int pendingLevelUps_ = 0;
    int combo_ = 0;
    float comboTimer_ = 0.0f;
    float goldMultiplier_ = 1.0f;
    int cardRerolls_ = 0;
    std::array<int, static_cast<size_t>(Resource::Count)> resources_{20, 6, 1}; // dotation de départ
};
