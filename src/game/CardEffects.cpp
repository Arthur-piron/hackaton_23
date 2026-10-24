#include "game/CardEffects.hpp"

#include "raymath.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kBurnTickInterval = 0.5f;
constexpr float kBlastDuration = 0.45f;
constexpr int kFlameTongues = 5;
constexpr float kFlameBaseOffset = 0.15f;  // au-dessus du torse
constexpr float kFlameHeight = 1.0f;
constexpr float kFlameSpread = 0.2f;

constexpr Color kFlameOuter = {255, 100, 15, 200};
constexpr Color kFlameInner = {255, 230, 140, 230};
constexpr Color kFlameGlow = {255, 110, 20, 70};
constexpr Color kFlameCore = {255, 150, 40, 130};
}

void CardEffects::clear()
{
    burns_.clear();
    blasts_.clear();
}

void CardEffects::ignite(const Enemy& enemy)
{
    if (!enemy.isAlive()) return;
    Burn& burn = burns_[enemy.id()];
    if (burn.timeLeft <= 0.0f) burn.tick = kBurnTickInterval;
    burn.timeLeft = kBurnDuration;
}

std::vector<CardEffects::BurnTick> CardEffects::update(float dt, std::vector<Enemy>& enemies)
{
    time_ += dt;
    for (Blast& blast : blasts_) blast.age += dt;
    blasts_.erase(std::remove_if(blasts_.begin(), blasts_.end(),
                                 [](const Blast& b) { return b.age >= kBlastDuration; }),
                  blasts_.end());

    std::vector<BurnTick> ticks;
    if (burns_.empty()) return ticks;
    for (auto& entry : burns_) entry.second.seen = false;
    for (Enemy& enemy : enemies) {
        if (!enemy.isAlive()) continue;
        const auto it = burns_.find(enemy.id());
        if (it == burns_.end()) continue;
        Burn& burn = it->second;
        burn.seen = true;
        burn.timeLeft -= dt;
        burn.tick -= dt;
        if (burn.tick <= 0.0f) {
            burn.tick += kBurnTickInterval;
            ticks.push_back({&enemy, kBurnDamagePerSecond * kBurnTickInterval});
        }
    }
    for (auto it = burns_.begin(); it != burns_.end();) {
        it = (!it->second.seen || it->second.timeLeft <= 0.0f) ? burns_.erase(it) : std::next(it);
    }
    return ticks;
}

void CardEffects::addBlast(Vector3 center, float radius, Color color)
{
    blasts_.push_back({center, radius, color, 0.0f});
}

void CardEffects::drawFlame(const Enemy& enemy) const
{
    const Vector3 base = Vector3Add(enemy.hitCenter(), {0.0f, kFlameBaseOffset, 0.0f});
    const float seed = static_cast<float>(enemy.id()) * 1.7f;
    for (int i = 0; i < kFlameTongues; ++i) {
        // Langues de feu qui ondulent, chacune à son rythme.
        const float phase = seed + static_cast<float>(i) * 2.1f;
        const float angle = phase + time_ * 1.3f;
        const float flicker = 0.65f + 0.35f * std::sin(time_ * 17.0f + phase * 3.0f);
        const Vector3 root = {base.x + std::cos(angle) * kFlameSpread, base.y, base.z + std::sin(angle) * kFlameSpread};
        const Vector3 tip = {root.x + 0.08f * std::sin(time_ * 9.0f + phase), root.y + kFlameHeight * flicker,
                             root.z + 0.08f * std::cos(time_ * 7.0f + phase)};
        DrawCylinderEx(root, tip, 0.17f, 0.0f, 6, kFlameOuter);
        const Vector3 innerTip = Vector3Lerp(root, tip, 0.6f);
        DrawCylinderEx(root, innerTip, 0.09f, 0.0f, 6, kFlameInner);
    }
    const float pulse = 1.0f + 0.12f * std::sin(time_ * 13.0f + seed);
    // Vu de dessus, les langues sont raccourcies : le cœur et le halo rendent le feu lisible.
    DrawSphereEx(Vector3Add(base, {0.0f, 0.35f, 0.0f}), 0.28f * pulse, 8, 8, kFlameCore);
    DrawSphereEx(Vector3Add(base, {0.0f, 0.25f, 0.0f}), 0.6f * pulse, 8, 8, kFlameGlow);
}

void CardEffects::draw(const std::vector<Enemy>& enemies) const
{
    if (burns_.empty() && blasts_.empty()) return;
    // Feu et ondes : mélange additif, sans écrire dans le tampon de profondeur.
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();
    for (const Enemy& enemy : enemies) {
        if (enemy.isAlive() && isBurning(enemy)) drawFlame(enemy);
    }
    for (const Blast& blast : blasts_) {
        const float t = blast.age / kBlastDuration;
        const float radius = blast.radius * (0.25f + 0.75f * std::sqrt(t));
        Color c = blast.color;
        c.a = static_cast<unsigned char>(static_cast<float>(c.a) * (1.0f - t));
        Color soft = c;
        soft.a = static_cast<unsigned char>(soft.a / 3);
        DrawSphereEx(blast.center, radius * 0.6f, 12, 12, soft);
        const Vector3 ground = {blast.center.x, 0.06f, blast.center.z};
        // Disque au sol qui montre la zone touchée, bordé d'anneaux plus vifs.
        DrawCylinder({ground.x, 0.03f, ground.z}, radius, radius, 0.02f, 32, soft);
        for (int ring = 0; ring < 4; ++ring) {
            DrawCircle3D(ground, radius - 0.06f * static_cast<float>(ring), {1.0f, 0.0f, 0.0f}, 90.0f, c);
        }
    }
    EndBlendMode();
    rlEnableDepthMask();
}
