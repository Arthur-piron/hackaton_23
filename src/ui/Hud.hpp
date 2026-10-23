#pragma once

#include "raylib.h"

#include <string>
#include <vector>

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
};

// Interface en jeu : barres, munitions, vague, messages, chiffres de dégâts.
class Hud {
public:
    void update(float dt);
    void draw(const Camera3D& camera, const HudData& data) const;

    // Grand message central (« VAGUE 3 », « VAGUE TERMINÉE »...).
    void announce(const std::string& title, const std::string& subtitle, Color color);
    void addDamageNumber(Vector3 position, float value, Color color);
    void clear();

private:
    struct FloatingText {
        Vector3 position;
        std::string text;
        Color color;
        float age;
    };

    void drawBars(const HudData& data) const;
    void drawAmmo(const HudData& data) const;
    void drawWave(const HudData& data) const;
    void drawAnnouncement() const;
    void drawCrosshair() const;

    std::vector<FloatingText> numbers_;
    std::string title_;
    std::string subtitle_;
    Color titleColor_ = WHITE;
    float announceAge_ = 999.0f;
};
