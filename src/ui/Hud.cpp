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
