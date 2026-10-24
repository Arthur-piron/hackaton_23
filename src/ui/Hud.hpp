#pragma once

#include "raylib.h"

#include <string>
#include <vector>

// Pastille d'une carte possédée.
struct HudCard {
    std::string label;  // 3 lettres
    Color color;        // famille
    Color border;       // rareté
    int rank;
    int maxRank;
};

// Ce que le HUD affiche, rempli par Game à chaque image.
struct HudData {
    float health = 0.0f;
    float maxHealth = 1.0f;
    float stamina = 1.0f;
    std::string weaponName;
    int magazine = 0;
    int magazineSize = 0;
    int reserve = 0;
    bool infiniteReserve = false;
    bool reloading = false;
    float reloadProgress = 0.0f;
    int wave = 0;
    bool intermission = false;
    float intermissionLeft = 0.0f;
    int remaining = 0;
    int score = 0;
    int kills = 0;
    float hurtFlash = 0.0f;
    int gold = 0;
    int level = 1;
    float xpProgress = 0.0f;
    int combo = 0;
    int comboMultiplier = 1;
    float comboTimeLeft = 0.0f; // fraction 0..1 de la fenêtre du combo
    std::vector<HudCard> cards;
};

// Interface en jeu : barres, munitions, vague, messages, chiffres de dégâts.
class Hud {
public:
    void update(float dt);
    void draw(const Camera3D& camera, const HudData& data) const;

    // Grand message central (« VAGUE 3 », « VAGUE TERMINÉE »...).
    void announce(const std::string& title, const std::string& subtitle, Color color);
    // scale : taille relative du chiffre (coups critiques en grand, brûlures en petit).
    void addDamageNumber(Vector3 position, float value, Color color, float scale = 1.0f);
    // Petits sursauts visuels du compteur d'or et du combo.
    void pulseGold() { goldPulse_ = 1.0f; }
    void pulseCombo() { comboPulse_ = 1.0f; }
    void clear();

private:
    struct FloatingText {
        Vector3 position;
        std::string text;
        Color color;
        float age;
        float scale;
    };

    void drawBars(const HudData& data) const;
    void drawAmmo(const HudData& data) const;
    void drawWave(const HudData& data) const;
    void drawAnnouncement() const;
    void drawEconomy(const HudData& data) const;
    void drawCombo(const HudData& data) const;
    void drawCards(const HudData& data) const;
    void drawCrosshair() const;

    std::vector<FloatingText> numbers_;
    std::string title_;
    std::string subtitle_;
    Color titleColor_ = WHITE;
    float announceAge_ = 999.0f;
    float goldPulse_ = 0.0f;
    float comboPulse_ = 0.0f;
    float time_ = 0.0f;
};
