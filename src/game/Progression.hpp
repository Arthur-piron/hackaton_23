#pragma once

#include "core/SaveFile.hpp"
#include "game/GameMode.hpp"

#include <array>
#include <string>
#include <vector>

// Talents permanents achetés avec les ossements (docs/GAME.md §6).
enum class TalentId {
    Constitution,  // vie de départ
    Heritage,      // or de départ
    Luck,          // rareté des cartes
    Reroll,        // relances de cartes gratuites
    Armory,        // commence avec le fusil à pompe
    SteadyLight,   // batterie de la lampe
    Lucidity,      // 4 cartes au choix
    Count
};

struct TalentInfo {
    TalentId id;
    const char* key;          // clé dans le fichier de sauvegarde
    const char* name;
    const char* description;  // effet d'un rang
    std::vector<int> costs;   // coût de chaque rang, en ossements
    bool implemented;         // un talent pas encore branché s'affiche « à venir » et ne s'achète pas
};

// Une ligne du tableau des scores local.
struct ScoreEntry {
    int score = 0;
    int wave = 0;
    GameMode mode = GameMode::Night;
};

// Ossements, talents et records, conservés d'une partie à l'autre.
class Progression {
public:
    static constexpr int kLeaderboardSize = 10;

    static const std::vector<TalentInfo>& talents();
    static const TalentInfo& info(TalentId id);

    // persistent = faux : rien n'est lu ni écrit (mode --smoke, pour ne pas toucher la vraie sauvegarde).
    Progression(std::string savePath, bool persistent);

    int bones() const { return bones_; }
    int rank(TalentId id) const { return ranks_[static_cast<size_t>(id)]; }
    int maxRank(TalentId id) const { return static_cast<int>(info(id).costs.size()); }
    // Coût du prochain rang, -1 au rang maximal.
    int nextCost(TalentId id) const;
    bool canBuy(TalentId id) const;
    // Dépense les ossements et sauvegarde ; faux si c'est impossible.
    bool buy(TalentId id);

    // Ossements gagnés en cours de partie (élites, boss) : sauvegardés en fin de partie.
    void addBones(int amount) { bones_ += amount; }
    // Fin de partie : 1 ossement par vague survécue, records, tableau des scores, sauvegarde.
    // Renvoie les ossements de la fin de partie.
    int finishRun(GameMode mode, int wavesSurvived, int score, bool victory);

    int bestWave() const { return bestWave_; }
    int bestScore() const { return bestScore_; }
    int victories() const { return victories_; }
    bool isUnlocked(GameMode mode) const { return victories_ >= gameModeRules(mode).victoriesToUnlock; }
    // Meilleures parties, de la meilleure à la moins bonne.
    const std::vector<ScoreEntry>& leaderboard() const { return leaderboard_; }
    // Place obtenue par la dernière partie dans le tableau (0 = première), -1 si elle n'y entre pas.
    int lastRank() const { return lastRank_; }

    // Effets des talents.
    float startHealthBonus() const;
    int startGold() const;
    int rarityBonusPercent() const;
    int freeCardRerolls() const;
    bool startsWithShotgun() const;
    float batteryMultiplier() const;

private:
    void load();
    void save();

    SaveFile file_;
    bool persistent_;
    int bones_ = 0;
    int bestWave_ = 0;
    int bestScore_ = 0;
    int victories_ = 0;
    std::array<int, static_cast<size_t>(TalentId::Count)> ranks_{};
    std::vector<ScoreEntry> leaderboard_;
    int lastRank_ = -1;
};
