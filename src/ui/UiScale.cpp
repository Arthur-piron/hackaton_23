#include "ui/UiScale.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kDesignWidth = 1280.0f;
constexpr float kDesignHeight = 720.0f;
constexpr float kMinScale = 0.5f; // fenêtre minuscule : on garde un texte lisible
}

namespace ui {

float scale()
{
    const float sx = static_cast<float>(GetScreenWidth()) / kDesignWidth;
    const float sy = static_cast<float>(GetScreenHeight()) / kDesignHeight;
    return std::max(kMinScale, std::min(sx, sy));
}

float width()
{
    return static_cast<float>(GetScreenWidth()) / scale();
}

float height()
{
    return static_cast<float>(GetScreenHeight()) / scale();
}

int screenWidth()
{
    return static_cast<int>(std::ceil(width()));
}

int screenHeight()
{
    return static_cast<int>(std::ceil(height()));
}

Vector2 mouse()
{
    return fromScreen(GetMousePosition());
}

Vector2 fromScreen(Vector2 screen)
{
    const float s = scale();
    return {screen.x / s, screen.y / s};
}

Rectangle toScreen(Rectangle r)
{
    const float s = scale();
    return {r.x * s, r.y * s, r.width * s, r.height * s};
}

void begin()
{
    Camera2D camera{};
    camera.zoom = scale();
    BeginMode2D(camera);
}

void end()
{
    EndMode2D();
}

} // namespace ui
