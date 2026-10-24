#include "ui/OptionsScreen.hpp"

#include "core/Fonts.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr int kRowCount = 6; // OptionsScreen::Row::Count
constexpr float kRowWidth = 720.0f;
constexpr float kRowHeight = 50.0f;
constexpr float kRowGap = 10.0f;
constexpr float kTop = 200.0f;
constexpr int kStep = 10;
constexpr float kSliderWidth = 180.0f;

constexpr Color kTitle = {230, 200, 120, 255};
constexpr Color kRow = {24, 22, 32, 230};
constexpr Color kRowSelected = {48, 42, 60, 245};
constexpr Color kBorder = {230, 200, 120, 255};
constexpr Color kText = {238, 232, 218, 255};
constexpr Color kTextDim = {150, 145, 135, 255};
constexpr Color kSliderBack = {50, 46, 60, 255};
constexpr Color kSliderFill = {230, 200, 120, 255};

void drawCentered(const char* text, float cx, float y, int size, Color color)
{
    fonts::drawText(text, static_cast<int>(std::round(cx)) - fonts::measureText(text, size) / 2, static_cast<int>(y),
                    size, color);
}
}

void OptionsScreen::open(const Settings& settings)
{
    settings_ = settings;
    selected_ = 0;
    time_ = 0.0f;
}

Rectangle OptionsScreen::rowRect(int index) const
{
    const float x = (static_cast<float>(GetScreenWidth()) - kRowWidth) / 2.0f;
    return {x, kTop + static_cast<float>(index) * (kRowHeight + kRowGap), kRowWidth, kRowHeight};
}

const char* OptionsScreen::label(Row row) const
{
    switch (row) {
    case Row::Master: return "Volume général";
    case Row::Music: return "Musique et ambiance";
    case Row::Effects: return "Effets sonores";
    case Row::Fullscreen: return "Plein écran";
    case Row::Credits: return "Crédits";
    case Row::Back: return "Retour";
    case Row::Count: break;
    }
    return "";
}

int* OptionsScreen::volume(Row row)
{
    return const_cast<int*>(static_cast<const OptionsScreen*>(this)->volume(row));
}

const int* OptionsScreen::volume(Row row) const
{
    if (row == Row::Master) return &settings_.master;
    if (row == Row::Music) return &settings_.music;
    if (row == Row::Effects) return &settings_.effects;
    return nullptr;
}

OptionsScreen::Result OptionsScreen::update(float dt)
{
    Result result;
    time_ += dt;
    const int previous = selected_;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) selected_ = (selected_ + kRowCount - 1) % kRowCount;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) selected_ = (selected_ + 1) % kRowCount;
    const Vector2 mouse = GetMousePosition();
    const Vector2 delta = GetMouseDelta();
    const bool mouseMoved = delta.x != 0.0f || delta.y != 0.0f;
    int direction = 0;
    bool clicked = false;
    for (int i = 0; i < kRowCount; ++i) {
        const Rectangle r = rowRect(i);
        if (!CheckCollisionPointRec(mouse, r)) continue;
        if (mouseMoved) selected_ = i;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selected_ = i;
            clicked = true;
            // Sur une ligne de volume : moitié gauche baisse, moitié droite monte.
            direction = mouse.x < r.x + r.width / 2.0f ? -1 : 1;
        }
    }
    result.moved = selected_ != previous;
    if (IsKeyPressed(KEY_ESCAPE)) result.close = true;

    const Row row = static_cast<Row>(selected_);
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) direction = -1;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) direction = 1;
    const bool confirm = clicked || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    if (int* value = volume(row)) {
        if (direction != 0) {
            const int next = std::clamp(*value + direction * kStep, 0, 100);
            result.changed = next != *value;
            *value = next;
        }
    } else if (row == Row::Fullscreen && (confirm || direction != 0)) {
        settings_.fullscreen = !settings_.fullscreen;
        result.changed = true;
    } else if (confirm) {
        if (row == Row::Credits) result.openCredits = true;
        if (row == Row::Back) result.close = true;
    }
    return result;
}

void OptionsScreen::draw() const
{
    const float w = static_cast<float>(GetScreenWidth());
    drawCentered("OPTIONS", w / 2.0f, 90.0f, 64, kTitle);
    for (int i = 0; i < kRowCount; ++i) {
        const Row row = static_cast<Row>(i);
        const Rectangle r = rowRect(i);
        const bool selected = i == selected_;
        DrawRectangleRounded(r, 0.2f, 6, selected ? kRowSelected : kRow);
        if (selected) {
            const float pulse = 0.6f + 0.4f * std::sin(time_ * 5.0f);
            DrawRectangleRoundedLinesEx(r, 0.2f, 6, 2.0f, Fade(kBorder, pulse));
        }
        fonts::drawText(label(row), static_cast<int>(r.x + 22.0f), static_cast<int>(r.y + 14.0f), 22, kText);
        const float right = r.x + r.width - 20.0f;
        if (const int* value = volume(row)) {
            // Curseur et pourcentage.
            const Rectangle bar = {right - kSliderWidth - 84.0f, r.y + r.height / 2.0f - 5.0f, kSliderWidth, 10.0f};
            DrawRectangleRec(bar, kSliderBack);
            DrawRectangleRec({bar.x, bar.y, bar.width * static_cast<float>(*value) / 100.0f, bar.height}, kSliderFill);
            const char* percent = TextFormat("%d %%", *value);
            fonts::drawText(percent, static_cast<int>(right - fonts::measureText(percent, 20)),
                            static_cast<int>(r.y + 15.0f), 20, kText);
        } else if (row == Row::Fullscreen) {
            const char* state = settings_.fullscreen ? "Oui" : "Non";
            fonts::drawText(state, static_cast<int>(right - fonts::measureText(state, 22)),
                            static_cast<int>(r.y + 14.0f), 22, settings_.fullscreen ? kSliderFill : kTextDim);
        }
    }
    drawCentered("Haut / Bas : choisir   Gauche / Droite : régler   Échap : retour",
                 w / 2.0f, static_cast<float>(GetScreenHeight()) - 60.0f, 16, kTextDim);
    DrawCircleV(GetMousePosition(), 5.0f, {255, 230, 170, 200});
}
