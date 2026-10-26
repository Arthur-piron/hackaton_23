#include "ui/MainMenu.hpp"

#include "core/Fonts.hpp"
#include "ui/UiScale.hpp"
#include "raymath.h"

#include <cmath>

namespace {
constexpr int kItemCount = 7; // MainMenu::Item::Count
constexpr int kCharacterCount = static_cast<int>(CharacterId::Count);
constexpr float kItemWidth = 380.0f;
constexpr float kItemHeight = 44.0f;
constexpr float kItemGap = 6.0f;
constexpr float kMenuTop = 240.0f;
constexpr float kBoardWidth = 420.0f;
constexpr float kBoardRowHeight = 28.0f;
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
constexpr Color kAffordable = {140, 230, 150, 255};
constexpr Color kTooExpensive = {220, 90, 80, 255};

void drawCentered(const char* text, float cx, float y, float size, Color color)
{
    const Vector2 m = fonts::measureTextEx(text, size);
    fonts::drawTextEx(text, {std::round(cx - m.x / 2.0f), std::round(y)}, size, color);
}

void drawRight(const char* text, float right, float y, float size, Color color)
{
    const Vector2 m = fonts::measureTextEx(text, size);
    fonts::drawTextEx(text, {std::round(right - m.x), std::round(y)}, size, color);
}

// Le menu à gauche, le tableau des scores à droite, l'ensemble centré.
float layoutLeft()
{
    return (ui::width() - (kItemWidth + kColumnGap + kBoardWidth)) / 2.0f;
}
}

void MainMenu::open()
{
    selected_ = 0;
    characterIndex_ = -1;
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
    case Item::Character: return "Personnage";
    case Item::Talents: return "Talents";
    case Item::Options: return "Options et crédits";
    case Item::Quit: return "Quitter";
    case Item::Count: break;
    }
    return "";
}

MainMenu::Result MainMenu::update(float dt, const Progression& progression)
{
    Result result;
    time_ += dt;
    if (characterIndex_ < 0) characterIndex_ = static_cast<int>(progression.selectedCharacter());
    const int previous = selected_;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) selected_ = (selected_ + kItemCount - 1) % kItemCount;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) selected_ = (selected_ + 1) % kItemCount;

    bool clicked = false;
    const Vector2 mouse = ui::mouse();
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

    const Item item = static_cast<Item>(selected_);
    const bool confirm = clicked || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    if (item == Item::Character) {
        // Flèches (ou Q/D, ou clic) : parcourir ; un personnage acheté est choisi aussitôt.
        int step = 0;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) step = -1;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) step = 1;
        const CharacterId shown = static_cast<CharacterId>(characterIndex_);
        if (confirm && !progression.owns(shown)) {
            if (progression.canBuy(shown)) {
                result.buyCharacter = characterIndex_;
            } else {
                result.refused = true;
            }
        } else if (clicked) {
            step = 1;
        }
        if (step != 0) {
            characterIndex_ = (characterIndex_ + step + kCharacterCount) % kCharacterCount;
            result.moved = true;
            if (progression.owns(static_cast<CharacterId>(characterIndex_))) result.selectCharacter = characterIndex_;
        }
        return result;
    }
    if (!confirm) return result;

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
    case Item::Options: result.openOptions = true; break;
    case Item::Quit: result.quit = true; break;
    case Item::Character:
    case Item::Count: break;
    }
    return result;
}

void MainMenu::draw(const Progression& progression) const
{
    const float w = ui::width();
    const float h = ui::height();
    drawCentered("LAST LIGHT", w / 2.0f, 60.0f, 96.0f, kTitle);
    drawCentered("Les morts craignent la lumière. Tiens jusqu'à l'aube.", w / 2.0f, 170.0f, 22.0f, kTagline);

    const CharacterId shown = static_cast<CharacterId>(characterIndex_ < 0
        ? static_cast<int>(progression.selectedCharacter()) : characterIndex_);
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
        const float right = r.x + r.width - 18.0f;
        const float detailY = r.y + 15.0f;
        if (item == Item::Character) {
            // « < Vampire > » : le personnage affiché, et son prix s'il n'est pas acheté.
            const CharacterInfo& c = characterInfo(shown);
            fonts::drawTextEx(TextFormat("<  %s  >", c.name), {r.x + 22.0f, r.y + 10.0f}, 24.0f,
                              progression.owns(shown) ? kText : kLocked);
            if (!progression.owns(shown)) {
                drawRight(TextFormat("%d os", c.cost), right, detailY, 15.0f,
                          progression.canBuy(shown) ? kAffordable : kTooExpensive);
            } else if (shown == progression.selectedCharacter()) {
                drawRight("choisi", right, detailY, 15.0f, kGold);
            }
            continue;
        }
        fonts::drawTextEx(itemLabel(item), {r.x + 22.0f, r.y + 10.0f}, 24.0f, locked ? kLocked : kText);
        if (locked) {
            drawRight("Gagne une Nuit", right, detailY, 15.0f, kLocked);
        } else if (item == Item::Talents) {
            drawRight(TextFormat("%d ossements", progression.bones()), right, detailY, 15.0f, kBone);
        }
    }

    // Description du mode ou du personnage sélectionné.
    const Rectangle last = itemRect(kItemCount - 1);
    const char* description = nullptr;
    if (selected_ <= static_cast<int>(Item::Nightmare)) {
        description = gameModeRules(static_cast<GameMode>(selected_)).description;
    } else if (selected_ == static_cast<int>(Item::Character)) {
        description = progression.owns(shown) ? characterInfo(shown).description
                                              : TextFormat("%s   (Entrée : acheter)", characterInfo(shown).description);
    }
    if (description) fonts::drawTextEx(description, {last.x, last.y + kItemHeight + 14.0f}, 15.0f, kTextDim);

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
    drawCentered(move, w / 2.0f, h - 66.0f, 15.0f, kTextDim);
    drawCentered(fight, w / 2.0f, h - 42.0f, 15.0f, kTextDim);
    DrawCircleV(ui::mouse(), 5.0f, {255, 230, 170, 200});
}
