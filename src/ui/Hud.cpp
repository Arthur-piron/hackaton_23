#include "ui/Hud.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kNumberLifetime = 0.8f;
constexpr float kNumberRise = 1.4f; // unités monde par seconde
constexpr float kAnnounceDuration = 2.6f;
constexpr float kAnnounceFade = 0.5f;
constexpr int kMargin = 24;

constexpr Color kPanel = {10, 10, 16, 170};
constexpr Color kHealth = {200, 40, 50, 255};
constexpr Color kHealthBack = {60, 15, 20, 255};
constexpr Color kStamina = {230, 200, 90, 255};
constexpr Color kText = {235, 230, 215, 255};
constexpr Color kDim = {150, 145, 135, 255};
constexpr Color kGold = {255, 205, 90, 255};
constexpr Color kGoldDark = {170, 110, 20, 255};
constexpr Color kXp = {70, 225, 190, 255};
constexpr Color kXpBack = {20, 55, 50, 255};
// Couleur du combo selon le multiplicateur (×1 à ×5).
constexpr Color kComboColors[] = {
    {235, 225, 200, 255}, {255, 205, 90, 255}, {255, 160, 50, 255}, {255, 95, 40, 255}, {255, 50, 90, 255},
};
constexpr float kGoldPulseDecay = 4.0f;
constexpr float kComboPulseDecay = 5.0f;
constexpr int kComboMinShown = 2; // une mort isolée n'affiche pas de combo

void drawTextCentered(const char* text, int cx, int y, int size, Color color)
{
    DrawText(text, cx - MeasureText(text, size) / 2, y, size, color);
}

void drawTextShadow(const char* text, int x, int y, int size, Color color)
{
    DrawText(text, x + 2, y + 2, size, {0, 0, 0, static_cast<unsigned char>(color.a * 0.7f)});
    DrawText(text, x, y, size, color);
}
}

void Hud::update(float dt)
{
    announceAge_ += dt;
    time_ += dt;
    goldPulse_ = std::max(0.0f, goldPulse_ - kGoldPulseDecay * dt);
    comboPulse_ = std::max(0.0f, comboPulse_ - kComboPulseDecay * dt);
    for (FloatingText& n : numbers_) {
        n.age += dt;
        n.position.y += kNumberRise * dt;
    }
    numbers_.erase(std::remove_if(numbers_.begin(), numbers_.end(),
                                  [](const FloatingText& n) { return n.age >= kNumberLifetime; }),
                   numbers_.end());
}

void Hud::clear()
{
    numbers_.clear();
    announceAge_ = 999.0f;
    goldPulse_ = 0.0f;
    comboPulse_ = 0.0f;
}

void Hud::announce(const std::string& title, const std::string& subtitle, Color color)
{
    title_ = title;
    subtitle_ = subtitle;
    titleColor_ = color;
    announceAge_ = 0.0f;
}

void Hud::addDamageNumber(Vector3 position, float value, Color color)
{
    numbers_.push_back({position, TextFormat("%d", static_cast<int>(std::lround(value))), color, 0.0f});
}

void Hud::draw(const Camera3D& camera, const HudData& data) const
{
    for (const FloatingText& n : numbers_) {
        const Vector2 p = GetWorldToScreen(n.position, camera);
        const float t = n.age / kNumberLifetime;
        const int size = static_cast<int>(22 + 10 * (1.0f - t));
        Color c = n.color;
        c.a = static_cast<unsigned char>(255 * (1.0f - t * t));
        drawTextShadow(n.text.c_str(), static_cast<int>(p.x) - MeasureText(n.text.c_str(), size) / 2,
                       static_cast<int>(p.y), size, c);
    }

    if (data.hurtFlash > 0.0f) {
        const int w = GetScreenWidth();
        const int h = GetScreenHeight();
        const unsigned char a = static_cast<unsigned char>(110 * data.hurtFlash);
        DrawRectangleGradientV(0, 0, w, h / 4, {160, 0, 0, a}, {160, 0, 0, 0});
        DrawRectangleGradientV(0, h * 3 / 4, w, h / 4, {160, 0, 0, 0}, {160, 0, 0, a});
        DrawRectangleGradientH(0, 0, w / 5, h, {160, 0, 0, a}, {160, 0, 0, 0});
        DrawRectangleGradientH(w * 4 / 5, 0, w / 5, h, {160, 0, 0, 0}, {160, 0, 0, a});
    }

    drawBars(data);
    drawAmmo(data);
    drawWave(data);
    drawEconomy(data);
    drawCombo(data);
    drawAnnouncement();
    drawCrosshair();
}

void Hud::drawBars(const HudData& data) const
{
    const int x = kMargin;
    const int y = GetScreenHeight() - kMargin - 58;
    const int width = 300;
    DrawRectangleRounded({static_cast<float>(x - 10), static_cast<float>(y - 10), width + 20.0f, 78.0f}, 0.2f, 6, kPanel);

    const float ratio = data.maxHealth > 0.0f ? std::clamp(data.health / data.maxHealth, 0.0f, 1.0f) : 0.0f;
    DrawRectangle(x, y, width, 26, kHealthBack);
    DrawRectangle(x, y, static_cast<int>(width * ratio), 26, kHealth);
    DrawRectangleLines(x, y, width, 26, {0, 0, 0, 200});
    DrawText(TextFormat("%d / %d", static_cast<int>(std::ceil(data.health)), static_cast<int>(data.maxHealth)),
             x + 8, y + 4, 20, kText);

    DrawRectangle(x, y + 36, width, 10, {50, 45, 20, 255});
    DrawRectangle(x, y + 36, static_cast<int>(width * std::clamp(data.stamina, 0.0f, 1.0f)), 10, kStamina);
}

void Hud::drawAmmo(const HudData& data) const
{
    const int w = GetScreenWidth();
    const int h = GetScreenHeight();
    const int x = w - kMargin - 260;
    const int y = h - kMargin - 58;
    DrawRectangleRounded({static_cast<float>(x - 10), static_cast<float>(y - 10), 280.0f, 78.0f}, 0.2f, 6, kPanel);
    DrawText(data.weaponName.c_str(), x, y, 20, kDim);

    if (data.reloading) {
        DrawText("RECHARGEMENT", x, y + 26, 26, kGold);
        DrawRectangle(x, y + 56, static_cast<int>(240 * data.reloadProgress), 4, kGold);
        return;
    }
    const Color ammoColor = data.magazine == 0 ? Color{230, 60, 60, 255} : kText;
    const char* magazine = TextFormat("%d", data.magazine);
    DrawText(magazine, x, y + 22, 40, ammoColor);
    const int after = x + MeasureText(magazine, 40) + 8;
    const char* reserve = data.infiniteReserve ? TextFormat("/ %d   inf.", data.magazineSize)
                                               : TextFormat("/ %d   %d", data.magazineSize, data.reserve);
    DrawText(reserve, after, y + 38, 20, kDim);
}

void Hud::drawWave(const HudData& data) const
{
    const int cx = GetScreenWidth() / 2;
    const char* wave = data.wave > 0 ? TextFormat("VAGUE %d", data.wave) : "PREPAREZ-VOUS";
    drawTextShadow(wave, cx - MeasureText(wave, 30) / 2, 16, 30, kText);
    const char* info = data.intermission ? TextFormat("prochaine vague dans %d s   (Entree : maintenant)",
                                                      static_cast<int>(std::ceil(data.intermissionLeft)))
                                         : TextFormat("%d morts-vivants restants", data.remaining);
    drawTextCentered(info, cx, 52, 18, kDim);

    const char* score = TextFormat("%d", data.score);
    drawTextShadow(score, GetScreenWidth() - kMargin - MeasureText(score, 36), 14, 36, kGold);
    const char* kills = TextFormat("%d tues", data.kills);
    DrawText(kills, GetScreenWidth() - kMargin - MeasureText(kills, 18), 54, 18, kDim);
}

void Hud::drawEconomy(const HudData& data) const
{
    // Sous le compteur de FPS : or, puis niveau et barre d'XP.
    const int x = 12;
    const int y = 38;
    DrawRectangleRounded({static_cast<float>(x), static_cast<float>(y), 236.0f, 74.0f}, 0.2f, 6, kPanel);

    const int coinX = x + 24;
    const int coinY = y + 22;
    const float coinRadius = 12.0f * (1.0f + 0.25f * goldPulse_);
    DrawCircle(coinX, coinY, coinRadius, kGoldDark);
    DrawCircle(coinX, coinY, coinRadius - 2.0f, kGold);
    DrawCircleLines(coinX, coinY, coinRadius * 0.55f, kGoldDark);
    DrawCircle(coinX - 4, coinY - 4, 2.5f, {255, 245, 200, 220});

    const int goldSize = static_cast<int>(30 * (1.0f + 0.15f * goldPulse_));
    Color goldColor = kGold;
    goldColor.g = static_cast<unsigned char>(goldColor.g + (255 - goldColor.g) * goldPulse_);
    goldColor.b = static_cast<unsigned char>(goldColor.b + (255 - goldColor.b) * goldPulse_);
    drawTextShadow(TextFormat("%d", data.gold), coinX + 22, coinY - goldSize / 2, goldSize, goldColor);

    const int barY = y + 52;
    const char* level = TextFormat("NIV %d", data.level);
    DrawText(level, x + 12, barY - 5, 18, kXp);
    const int barX = x + 22 + MeasureText(level, 18);
    const int barWidth = x + 236 - 14 - barX;
    DrawRectangle(barX, barY, barWidth, 8, kXpBack);
    DrawRectangle(barX, barY, static_cast<int>(barWidth * std::clamp(data.xpProgress, 0.0f, 1.0f)), 8, kXp);
}

void Hud::drawCombo(const HudData& data) const
{
    if (data.combo < kComboMinShown) return;
    // Sous le score, aligné à droite : grand multiplicateur qui pulse, compteur et jauge du temps restant.
    const int right = GetScreenWidth() - kMargin;
    const int y = 84;
    const int tier = std::clamp(data.comboMultiplier, 1, 5) - 1;
    const Color color = kComboColors[tier];

    const float beat = 0.04f * std::sin(time_ * (8.0f + 3.0f * static_cast<float>(tier)));
    const int size = static_cast<int>((52 + 6 * tier) * (1.0f + 0.35f * comboPulse_ + beat));
    const char* multiplier = TextFormat("x%d", data.comboMultiplier);
    const int boxHeight = 52 + 6 * 4;
    drawTextShadow(multiplier, right - MeasureText(multiplier, size), y + (boxHeight - size) / 2, size, color);

    const char* count = TextFormat("COMBO %d", data.combo);
    const int infoY = y + boxHeight + 4;
    DrawText(count, right - MeasureText(count, 18), infoY, 18, kText);

    const int gaugeWidth = 140;
    const int gaugeY = infoY + 24;
    DrawRectangle(right - gaugeWidth, gaugeY, gaugeWidth, 6, {40, 35, 25, 220});
    const int filled = static_cast<int>(gaugeWidth * std::clamp(data.comboTimeLeft, 0.0f, 1.0f));
    DrawRectangle(right - filled, gaugeY, filled, 6, color);
}

void Hud::drawAnnouncement() const
{
    if (announceAge_ >= kAnnounceDuration) return;
    const float fadeIn = std::min(1.0f, announceAge_ / 0.15f);
    const float fadeOut = std::clamp((kAnnounceDuration - announceAge_) / kAnnounceFade, 0.0f, 1.0f);
    const float alpha = fadeIn * fadeOut;
    // Léger zoom à l'apparition.
    const int size = static_cast<int>(72 * (1.0f + 0.25f * (1.0f - fadeIn)));
    const int cx = GetScreenWidth() / 2;
    const int y = GetScreenHeight() / 3;
    Color c = titleColor_;
    c.a = static_cast<unsigned char>(255 * alpha);
    drawTextShadow(title_.c_str(), cx - MeasureText(title_.c_str(), size) / 2, y, size, c);
    Color sub = kText;
    sub.a = static_cast<unsigned char>(220 * alpha);
    drawTextCentered(subtitle_.c_str(), cx, y + size + 10, 24, sub);
}

void Hud::drawCrosshair() const
{
    const Vector2 m = GetMousePosition();
    const Color c = {255, 230, 170, 220};
    DrawCircleLinesV(m, 9.0f, c);
    DrawLineEx({m.x - 16, m.y}, {m.x - 6, m.y}, 2.0f, c);
    DrawLineEx({m.x + 6, m.y}, {m.x + 16, m.y}, 2.0f, c);
    DrawLineEx({m.x, m.y - 16}, {m.x, m.y - 6}, 2.0f, c);
    DrawLineEx({m.x, m.y + 6}, {m.x, m.y + 16}, 2.0f, c);
}
