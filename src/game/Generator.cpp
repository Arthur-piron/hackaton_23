#include "game/Generator.hpp"

#include "core/Fonts.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kTank = 3.0f;               // bidons
constexpr float kSecondsPerFuel = 90.0f;    // pour 2 structures alimentées
constexpr int kBaseConsumers = 2;
constexpr float kExtraConsumer = 0.2f;      // +20 % par structure en plus
// Lampadaire : rayon 6 m, ralentit de 40 % ; sans courant, une lanterne de 3 m.
constexpr float kLampRadius = 6.0f;
constexpr float kLampSlow = 0.4f;
constexpr float kLampSlowRefresh = 0.2f;
constexpr float kLanternRadius = 3.0f;
constexpr float kLampHeight = 2.6f;
constexpr Color kLampColor = {255, 225, 160, 255};

constexpr float kLedHeight = 1.4f;
constexpr Color kRunning = {90, 255, 120, 255};
constexpr Color kEmpty = {255, 60, 50, 255};
constexpr float kPromptDistance = 8.0f;
constexpr Color kPromptColor = {255, 215, 120, 255};
constexpr Color kGaugeColor = {230, 200, 120, 220};
constexpr Color kShadow = {0, 0, 0, 200};

float flatDistance(Vector3 a, Vector3 b)
{
    return Vector2Distance({a.x, a.z}, {b.x, b.z});
}

void drawCenteredText(const char* text, Vector2 at, int size, Color color)
{
    const int x = static_cast<int>(at.x) - fonts::measureText(text, size) / 2;
    const int y = static_cast<int>(at.y);
    fonts::drawText(text, x + 2, y + 2, size, kShadow);
    fonts::drawText(text, x, y, size, color);
}
}

bool Generator::powered(const Structures& structures, Vector3 point) const
{
    for (int id : structures.ids()) {
        if (structures.kind(id) == StructureKind::Generator && structures.turret(id).fuel > 0.0f &&
            flatDistance(structures.center(id), point) <= kRange) {
            return true;
        }
    }
    return false;
}

void Generator::update(float dt, bool fighting, Structures& structures, std::vector<Enemy>& enemies,
                       EventQueue& events)
{
    time_ += dt;
    const std::vector<int> ids = structures.ids();
    for (int id : ids) {
        if (structures.kind(id) != StructureKind::Generator) continue;
        float& fuel = structures.turret(id).fuel;
        if (!fighting || fuel <= 0.0f) continue;
        // Consommation : selon le nombre de structures électriques qu'il alimente.
        int consumers = 0;
        for (int other : ids) {
            if (Structures::info(structures.kind(other)).electric &&
                flatDistance(structures.center(other), structures.center(id)) <= kRange) {
                ++consumers;
            }
        }
        if (consumers == 0) continue;
        const float rate = 1.0f + kExtraConsumer * static_cast<float>(std::max(0, consumers - kBaseConsumers));
        fuel = std::max(0.0f, fuel - rate * dt / kSecondsPerFuel);
        if (fuel <= 0.0f) events.push_back({EventType::GeneratorEmpty, structures.center(id)});
    }

    // Les lampadaires alimentés ralentissent les morts dans leur lumière (les squelettes n'en ont cure).
    for (int id : ids) {
        if (structures.kind(id) != StructureKind::Lamppost || !powered(structures, structures.center(id))) continue;
        for (Enemy& e : enemies) {
            if (!e.isAlive() || e.type() == EnemyType::Skeleton) continue;
            if (flatDistance(e.position(), structures.center(id)) <= kLampRadius) e.slow(kLampSlow, kLampSlowRefresh);
        }
    }
}

bool Generator::refuelNear(Vector3 player, Structures& structures, Economy& economy, EventQueue& events)
{
    for (int id : structures.ids()) {
        if (structures.kind(id) != StructureKind::Generator) continue;
        if (flatDistance(structures.center(id), player) > kInteractDistance + 1.0f) continue;
        float& fuel = structures.turret(id).fuel;
        if (fuel <= kTank - 1.0f && economy.spendResource(Resource::Fuel, 1)) {
            fuel += 1.0f;
            events.push_back({EventType::GeneratorRefueled, structures.center(id), fuel});
        } else {
            events.push_back({EventType::BuildDenied, structures.center(id)});
        }
        return true;
    }
    return false;
}

void Generator::draw(const Structures& structures) const
{
    for (int id : structures.ids()) {
        if (structures.kind(id) != StructureKind::Generator) continue;
        const bool running = structures.turret(id).fuel > 0.0f;
        // En marche, le voyant vert pulse ; à sec, il clignote en rouge.
        const float pulse = running ? 0.7f + 0.3f * std::sin(time_ * 6.0f) : (std::fmod(time_, 0.6f) < 0.3f ? 1.0f : 0.2f);
        const Vector3 c = structures.center(id);
        DrawSphere({c.x, kLedHeight, c.z}, 0.08f, Fade(running ? kRunning : kEmpty, pulse));
    }
}

void Generator::drawPrompt(const Camera3D& camera, Vector3 player, const Structures& structures,
                           const Economy& economy) const
{
    for (int id : structures.ids()) {
        if (structures.kind(id) != StructureKind::Generator) continue;
        const Vector3 c = structures.center(id);
        const float distance = flatDistance(c, player);
        if (distance > kPromptDistance) continue;
        const Vector2 at = GetWorldToScreen({c.x, kLedHeight + 0.9f, c.z}, camera);
        const float fuel = structures.turret(id).fuel;
        drawCenteredText(fuel > 0.0f ? TextFormat("Carburant %.1f / %.0f", fuel, kTank) : "Générateur à sec", at, 18,
                         fuel > 0.0f ? kGaugeColor : kEmpty);
        if (distance <= kInteractDistance + 1.0f) {
            const char* text = economy.resource(Resource::Fuel) > 0 ? "E : verser un bidon" : "Il te faut un bidon";
            drawCenteredText(text, {at.x, at.y - 28.0f}, 22, kPromptColor);
        }
    }
}

std::vector<PointLight> Generator::lights(const Structures& structures) const
{
    std::vector<PointLight> list;
    for (int id : structures.ids()) {
        if (structures.kind(id) != StructureKind::Lamppost) continue;
        const bool on = powered(structures, structures.center(id));
        PointLight light;
        light.position = Vector3Add(structures.center(id), {0.0f, kLampHeight, 0.0f});
        light.color = kLampColor;
        light.range = on ? kLampRadius : kLanternRadius;
        light.intensity = on ? 1.4f : 0.6f;
        light.flicker = on ? 0.05f : 0.4f;
        light.halo = on ? 1.6f : 0.8f;
        list.push_back(light);
    }
    return list;
}
