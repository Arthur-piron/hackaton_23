#pragma once

#include "game/Enemy.hpp"
#include "game/Events.hpp"

// Or, XP, niveaux et combo du joueur pendant une partie.
// L'or et l'XP d'une mort ne sont crédités qu'au ramassage (voir Pickups).
class Economy {
public:
    // Délai maximal entre deux morts pour enchaîner le combo.
    static constexpr float kComboWindow = 2.0f;

    struct Reward {
        int gold = 0; // déjà multiplié par le combo
        int xp = 0;
    };

    void reset();
    void update(float dt);

    // Enregistre une mort : fait progresser le combo et renvoie ce qu'elle lâche.
    Reward addKill(EnemyType type);
    // Prendre un coup divise le combo par deux.
    void onPlayerDamaged();

    void addGold(int amount);
    // Publie un événement LevelUp par niveau gagné.
    void addXp(int amount, EventQueue& events);
    // Débite l'or si le joueur en a assez.
    bool spend(int amount);

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

private:
    int gold_ = 0;
    int xp_ = 0;
    int level_ = 1;
    int pendingLevelUps_ = 0;
    int combo_ = 0;
    float comboTimer_ = 0.0f;
};
