#include "game/Altar.hpp"

#include "core/Fonts.hpp"
#include "game/Zones.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr Color kReadyColor = {170, 215, 255, 255};
constexpr float kLightHeight = 1.6f;
constexpr float kLightRange = 6.0f;
constexpr float kReadyIntensity = 1.0f;
constexpr float kSpentIntensity = 0.12f;
constexpr float kReadyHalo = 1.4f;
// Après la prière : un éclat qui retombe en kBurstDuration secondes.
constexpr float kBurstIntensity = 3.0f;
constexpr float kBurstDuration = 1.2f;
constexpr float kBurstHalo = 3.0f;

constexpr float kSignHeight = 2.4f;
constexpr float kSignDistance = 12.0f;
constexpr Color kPromptColor = {190, 225, 255, 255};
constexpr Color kSignColor = {170, 205, 235, 200};
constexpr Color kSpentColor = {150, 150, 165, 220};
constexpr Color kShadow = {0, 0, 0, 200};

void drawCenteredText(const char* text, Vector2 at, int size, Color color)
{
    const int x = static_cast<int>(at.x) - fonts::measureText(text, size) / 2;
    const int y = static_cast<int>(at.y);
    fonts::drawText(text, x + 2, y + 2, size, kShadow);
    fonts::drawText(text, x, y, size, color);
}
}

Altar::Altar(const AltarDef& def)
    : position_(def.position)
{
}

void Altar::reset()
{
    ready_ = true;
    usedAt_ = -100.0f;
}

bool Altar::inReach(Vector3 player) const
{
    return Vector2Distance({player.x, player.z}, {position_.x, position_.z}) <= kInteractDistance;
}

bool Altar::tryUse(Vector3 player, bool wounded, const Zones& zones, EventQueue& events)
{
    if (!ready_ || !wounded || !inReach(player) || !zones.isOpen(zones.zoneAt(position_))) return false;
    ready_ = false;
    usedAt_ = time_;
    events.push_back({EventType::AltarUsed, position_});
    return true;
}

void Altar::drawPrompt(const Camera3D& camera, Vector3 player, bool wounded, const Zones& zones) const
{
    if (!zones.isOpen(zones.zoneAt(position_))) return;
    const float distance = Vector2Distance({player.x, player.z}, {position_.x, position_.z});
    if (distance > kSignDistance) return;
    const Vector2 screen = GetWorldToScreen(Vector3Add(position_, {0.0f, kSignHeight, 0.0f}), camera);
    if (distance > kInteractDistance) {
        if (ready_) drawCenteredText("AUTEL", screen, 18, kSignColor);
        return;
    }
    if (ready_) {
        drawCenteredText("E : prier", {screen.x, screen.y - 30.0f}, 24, kPromptColor);
        drawCenteredText(wounded ? "Soins complets, une fois par vague" : "Tu n'es pas blessé... pas encore", screen,
                         18, kSignColor);
    } else {
        drawCenteredText("L'autel se tait", {screen.x, screen.y - 30.0f}, 24, kSpentColor);
        drawCenteredText("Il se ranime à la prochaine vague", screen, 18, kSpentColor);
    }
}

PointLight Altar::light() const
{
    PointLight light;
    light.position = Vector3Add(position_, {0.0f, kLightHeight, 0.0f});
    light.color = kReadyColor;
    light.range = kLightRange;
    light.flicker = 0.15f;
    const float burst = std::max(0.0f, 1.0f - (time_ - usedAt_) / kBurstDuration);
    if (burst > 0.0f) {
        light.intensity = kSpentIntensity + kBurstIntensity * burst;
        light.halo = kBurstHalo * burst;
    } else if (ready_) {
        light.intensity = kReadyIntensity * (0.85f + 0.15f * std::sin(time_ * 1.7f));
        light.halo = kReadyHalo;
    } else {
        light.intensity = kSpentIntensity;
    }
    return light;
}
