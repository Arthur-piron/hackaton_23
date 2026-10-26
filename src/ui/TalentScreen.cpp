#include "ui/TalentScreen.hpp"

#include "core/Fonts.hpp"

#include "ui/UiScale.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPanelWidth = 780.0f;
constexpr float kRowHeight = 58.0f;
constexpr float kRowGap = 7.0f;
constexpr float kTop = 150.0f;
constexpr float kPipSize = 14.0f;
constexpr float kPipGap = 6.0f;
constexpr float kFlashDecay = 3.0f;

constexpr Color kBackground = {8, 8, 14, 255};
constexpr Color kRow = {24, 22, 32, 235};
constexpr Color kRowSelected = {44, 40, 58, 245};
constexpr Color kBorder = {230, 200, 120, 255};
constexpr Color kText = {238, 232, 218, 255};
constexpr Color kTextDim = {160, 154, 140, 255};
constexpr Color kTitle = {230, 200, 120, 255};
constexpr Color kBone = {235, 225, 195, 255};
constexpr Color kPipFull = {230, 200, 120, 255};
constexpr Color kPipEmpty = {70, 66, 80, 255};
constexpr Color kAffordable = {140, 230, 150, 255};
constexpr Color kTooExpensive = {220, 90, 80, 255};
constexpr Color kLocked = {110, 105, 120, 255};

int talentCount()
{
    return static_cast<int>(Progression::talents().size());
}

void drawCentered(const char* text, int cx, int y, int size, Color color)
{
    fonts::drawText(text, cx - fonts::measureText(text, size) / 2, y, size, color);
}
}

void TalentScreen::open()
{
    selected_ = 0;
    time_ = 0.0f;
    flash_ = 0.0f;
}

Rectangle TalentScreen::rowRect(int index) const
{
    const float x = (ui::width() - kPanelWidth) / 2.0f;
    return {x, kTop + static_cast<float>(index) * (kRowHeight + kRowGap), kPanelWidth, kRowHeight};
}

TalentScreen::Result TalentScreen::update(float dt)
{
    Result result;
    time_ += dt;
    flash_ = std::max(0.0f, flash_ - dt * kFlashDecay);
    const int previous = selected_;

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) selected_ = (selected_ + talentCount() - 1) % talentCount();
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) selected_ = (selected_ + 1) % talentCount();

    const Vector2 mouse = ui::mouse();
    bool clicked = false;
    for (int i = 0; i < talentCount(); ++i) {
        if (!CheckCollisionPointRec(mouse, rowRect(i))) continue;
        if (Vector2Length(GetMouseDelta()) > 0.0f) selected_ = i;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selected_ = i;
            clicked = true;
        }
    }

    result.moved = selected_ != previous;
    if (clicked || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) result.purchase = selected_;
    result.close = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_T) || IsKeyPressed(KEY_BACKSPACE);
    return result;
}

void TalentScreen::notifyPurchase(bool bought)
{
    flash_ = 1.0f;
    flashOk_ = bought;
}

void TalentScreen::draw(const Progression& progression) const
{
    const int w = ui::screenWidth();
    const int h = ui::screenHeight();
    DrawRectangle(0, 0, w, h, kBackground);

    drawCentered("TALENTS", w / 2, 40, 56, kTitle);
    const char* bones = TextFormat("Ossements : %d", progression.bones());
    const Color bonesColor = flash_ > 0.0f ? ColorLerp(kBone, flashOk_ ? kAffordable : kTooExpensive, flash_) : kBone;
    drawCentered(bones, w / 2, 104, 26, bonesColor);

    for (int i = 0; i < talentCount(); ++i) {
        const TalentInfo& talent = Progression::talents()[static_cast<size_t>(i)];
        const Rectangle r = rowRect(i);
        const bool selected = i == selected_;
        DrawRectangleRounded(r, 0.18f, 6, selected ? kRowSelected : kRow);
        if (selected) {
            const float pulse = 0.6f + 0.4f * std::sin(time_ * 5.0f);
            DrawRectangleRoundedLinesEx(r, 0.18f, 6, 2.0f, Fade(kBorder, pulse));
        }

        const Color nameColor = talent.implemented ? kText : kLocked;
        const int x = static_cast<int>(r.x) + 18;
        const int y = static_cast<int>(r.y);
        fonts::drawText(talent.name, x, y + 7, 24, nameColor);
        fonts::drawText(talent.description, x, y + 35, 16, talent.implemented ? kTextDim : kLocked);

        // Rangs : une pastille par rang, pleine si acquise.
        const int max = progression.maxRank(talent.id);
        const int rank = progression.rank(talent.id);
        const float pipsRight = r.x + r.width - 150.0f;
        for (int p = 0; p < max; ++p) {
            const float px = pipsRight - static_cast<float>(max - p) * (kPipSize + kPipGap);
            DrawRectangleRounded({px, r.y + (r.height - kPipSize) / 2.0f, kPipSize, kPipSize}, 0.3f, 4,
                                 p < rank ? kPipFull : kPipEmpty);
        }

        // Coût du prochain rang.
        const char* cost = nullptr;
        Color costColor = kTextDim;
        if (!talent.implemented) {
            cost = "À venir";
            costColor = kLocked;
        } else if (progression.nextCost(talent.id) < 0) {
            cost = "MAX";
            costColor = kPipFull;
        } else {
            cost = TextFormat("%d os", progression.nextCost(talent.id));
            costColor = progression.canBuy(talent.id) ? kAffordable : kTooExpensive;
        }
        const int costX = static_cast<int>(r.x + r.width) - 18 - fonts::measureText(cost, 22);
        fonts::drawText(cost, costX, y + 18, 22, costColor);
    }

    const char* records = TextFormat("Record : vague %d   -   score %d   -   victoires %d", progression.bestWave(),
                                     progression.bestScore(), progression.victories());
    drawCentered(records, w / 2, h - 84, 18, kTextDim);
    drawCentered("Entrée / clic : acheter   Échap : retour   1 ossement par vague survécue", w / 2, h - 52, 18,
                 kTextDim);
    DrawCircleV(ui::mouse(), 5.0f, {255, 230, 170, 200});
}
