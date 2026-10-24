#include "ui/MainMenu.hpp"

#include "core/Fonts.hpp"
#include "raymath.h"

#include <cmath>

namespace {
constexpr int kItemCount = 5;
constexpr float kItemWidth = 380.0f;
constexpr float kItemHeight = 54.0f;
constexpr float kItemGap = 10.0f;
constexpr float kMenuTop = 250.0f;
constexpr float kBoardWidth = 420.0f;
constexpr float kBoardRowHeight = 30.0f;
constexpr float kColumnGap = 60.0f;

constexpr Color kTitle = {230, 200, 120, 255};
constexpr Color kTagline = {180, 170, 150, 255};
constexpr Color kItem = {24, 22, 32, 230};
constexpr Color kItemSelected = {48, 42, 60, 245};
constexpr Color kBorder = {230, 200, 120, 255};
constexpr Color kText = {238, 232, 218, 255};
constexpr Color kTextDim = {150, 145, 135, 255};
constexpr Color kLocked = {100, 96, 110, 255};
constexpr Color kBone = {235, 225, 195, 255};
constexpr Color kPanel = {14, 13, 20, 220};
constexpr Color kGold = {255, 205, 90, 255};

void drawCentered(const char* text, float cx, float y, float size, Color color)
{
    const Vector2 m = fonts::measureTextEx(text, size);
    fonts::drawTextEx(text, {std::round(cx - m.x / 2.0f), std::round(y)}, size, color);
}

// Le menu à gauche, le tableau des scores à droite, l'ensemble centré.
float layoutLeft()
{
    return (static_cast<float>(GetScreenWidth()) - (kItemWidth + kColumnGap + kBoardWidth)) / 2.0f;
}
}

void MainMenu::open()
{
    selected_ = 0;
    time_ = 0.0f;
}

Rectangle MainMenu::itemRect(int index) const
{
    return {layoutLeft(), kMenuTop + static_cast<float>(index) * (kItemHeight + kItemGap), kItemWidth, kItemHeight};
}

const char* MainMenu::itemLabel(Item item) const
{
    switch (item) {
    case Item::Night: return gameModeRules(GameMode::Night).name;
    case Item::Endless: return gameModeRules(GameMode::Endless).name;
    case Item::Nightmare: return gameModeRules(GameMode::Nightmare).name;
    case Item::Talents: return "Talents";
    case Item::Quit: return "Quitter";
    case Item::Count: break;
    }
    return "";
}

MainMenu::Result MainMenu::update(float dt, const Progression& progression)
{
    Result result;
    time_ += dt;
    const int previous = selected_;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) selected_ = (selected_ + kItemCount - 1) % kItemCount;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) selected_ = (selected_ + 1) % kItemCount;

    bool clicked = false;
    const Vector2 mouse = GetMousePosition();
    for (int i = 0; i < kItemCount; ++i) {
        if (!CheckCollisionPointRec(mouse, itemRect(i))) continue;
        if (Vector2Length(GetMouseDelta()) > 0.0f) selected_ = i;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selected_ = i;
            clicked = true;
        }
    }
    result.moved = selected_ != previous;
    // T reste un raccourci vers les talents.
    if (IsKeyPressed(KEY_T)) result.openTalents = true;
    if (!clicked && !IsKeyPressed(KEY_ENTER) && !IsKeyPressed(KEY_SPACE)) return result;

    const Item item = static_cast<Item>(selected_);
    switch (item) {
    case Item::Night:
    case Item::Endless:
    case Item::Nightmare: {
        const GameMode mode = static_cast<GameMode>(selected_);
        if (progression.isUnlocked(mode)) {
            result.startMode = static_cast<int>(mode);
        } else {
            result.refused = true;
        }
        break;
    }
    case Item::Talents: result.openTalents = true; break;
    case Item::Quit: result.quit = true; break;
    case Item::Count: break;
    }
    return result;
}

void MainMenu::draw(const Progression& progression) const
{
    const float w = static_cast<float>(GetScreenWidth());
    const float h = static_cast<float>(GetScreenHeight());
    drawCentered("LAST LIGHT", w / 2.0f, 70.0f, 96.0f, kTitle);
    drawCentered("Les morts craignent la lumière. Tiens jusqu'à l'aube.", w / 2.0f, 180.0f, 22.0f, kTagline);

    // Menu
    for (int i = 0; i < kItemCount; ++i) {
        const Item item = static_cast<Item>(i);
        const Rectangle r = itemRect(i);
        const bool selected = i == selected_;
        const bool locked = i <= static_cast<int>(Item::Nightmare) && !progression.isUnlocked(static_cast<GameMode>(i));
        DrawRectangleRounded(r, 0.2f, 6, selected ? kItemSelected : kItem);
        if (selected) {
            const float pulse = 0.6f + 0.4f * std::sin(time_ * 5.0f);
            DrawRectangleRoundedLinesEx(r, 0.2f, 6, 2.0f, Fade(kBorder, pulse));
        }
        const Color color = locked ? kLocked : kText;
        fonts::drawTextEx(itemLabel(item), {r.x + 22.0f, r.y + 14.0f}, 26.0f, color);
        if (locked) {
            const char* lock = "Gagne une Nuit";
            const Vector2 m = fonts::measureTextEx(lock, 15.0f);
            fonts::drawTextEx(lock, {r.x + r.width - 18.0f - m.x, r.y + 20.0f}, 15.0f, kLocked);
        } else if (item == Item::Talents) {
            const char* bones = TextFormat("%d ossements", progression.bones());
            const Vector2 m = fonts::measureTextEx(bones, 15.0f);
            fonts::drawTextEx(bones, {r.x + r.width - 18.0f - m.x, r.y + 20.0f}, 15.0f, kBone);
        }
    }
    // Description du mode sélectionné.
    if (selected_ <= static_cast<int>(Item::Nightmare)) {
        const Rectangle last = itemRect(kItemCount - 1);
        fonts::drawTextEx(gameModeRules(static_cast<GameMode>(selected_)).description,
                          {last.x, last.y + kItemHeight + 18.0f}, 16.0f, kTextDim);
    }

    // Tableau des scores
    const Rectangle board = {layoutLeft() + kItemWidth + kColumnGap, kMenuTop, kBoardWidth,
                             (kItemHeight + kItemGap) * kItemCount - kItemGap};
    DrawRectangleRounded(board, 0.06f, 6, kPanel);
    fonts::drawTextEx("MEILLEURES NUITS", {board.x + 20.0f, board.y + 14.0f}, 20.0f, kTitle);
    const std::vector<ScoreEntry>& scores = progression.leaderboard();
    if (scores.empty()) {
        fonts::drawTextEx("Aucune partie pour l'instant.", {board.x + 20.0f, board.y + 56.0f}, 16.0f, kTextDim);
    }
    for (size_t i = 0; i < scores.size(); ++i) {
        const float y = board.y + 52.0f + static_cast<float>(i) * kBoardRowHeight;
        const ScoreEntry& s = scores[i];
        const Color color = i == 0 ? kGold : kText;
        fonts::drawTextEx(TextFormat("%d.", static_cast<int>(i) + 1), {board.x + 20.0f, y}, 18.0f, kTextDim);
        fonts::drawTextEx(TextFormat("%d", s.score), {board.x + 60.0f, y}, 18.0f, color);
        fonts::drawTextEx(TextFormat("vague %d", s.wave), {board.x + 180.0f, y}, 16.0f, kTextDim);
        fonts::drawTextEx(gameModeRules(s.mode).name, {board.x + 300.0f, y}, 16.0f, kTextDim);
    }

    const char* move = "ZQSD : bouger   Maj : sprint   E : interagir   Entrée : vague suivante";
    const char* fight = "Clic : tirer   Clic droit : lampe   R : recharger   1-4 / molette : armes";
    drawCentered(move, w / 2.0f, h - 76.0f, 16.0f, kTextDim);
    drawCentered(fight, w / 2.0f, h - 50.0f, 16.0f, kTextDim);
    DrawCircleV(GetMousePosition(), 5.0f, {255, 230, 170, 200});
}
