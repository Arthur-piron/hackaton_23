#include "fx/PostFx.hpp"

#include "core/AssetPath.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kHurtDecay = 2.5f;      // par seconde
constexpr float kFlashDecay = 9.0f;     // par seconde
constexpr float kFlashAmount = 0.6f;    // intensité d'un flash de mort (le shader l'atténue encore)
constexpr float kLowHealthSmoothing = 4.0f;
// Suréchantillonnage : la scène est rendue en 2x puis réduite par le filtrage
// bilinéaire (une lecture = moyenne de 2x2 pixels). Remplace le MSAA de la
// fenêtre, perdu avec le rendu dans une texture. Désactivé au-delà du 1080p.
constexpr int kSupersample = 2;
constexpr int kSupersampleMaxPixels = 1920 * 1080;

int supersampleFor(int width, int height)
{
    return width * height <= kSupersampleMaxPixels ? kSupersample : 1;
}
}

PostFx::PostFx()
    : shader_(LoadShader(nullptr, assetPath("shaders/postfx.fs").c_str()))
{
    timeLoc_ = GetShaderLocation(shader_, "time");
    hurtLoc_ = GetShaderLocation(shader_, "hurt");
    lowHealthLoc_ = GetShaderLocation(shader_, "lowHealth");
    flashLoc_ = GetShaderLocation(shader_, "flash");
    recreateTarget(GetScreenWidth(), GetScreenHeight());
}

PostFx::~PostFx()
{
    UnloadRenderTexture(target_);
    UnloadShader(shader_);
}

void PostFx::recreateTarget(int width, int height)
{
    if (target_.id != 0) UnloadRenderTexture(target_);
    const int scale = supersampleFor(width, height);
    target_ = LoadRenderTexture(width * scale, height * scale);
    // Filtrage bilinéaire : réduction du suréchantillonnage et aberration chromatique.
    SetTextureFilter(target_.texture, TEXTURE_FILTER_BILINEAR);
}

void PostFx::update(float dt, float lowHealth)
{
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();
    const int scale = supersampleFor(width, height);
    // Fenêtre réduite : taille nulle, on garde l'ancienne cible.
    if (width > 0 && height > 0 &&
        (width * scale != target_.texture.width || height * scale != target_.texture.height)) {
        recreateTarget(width, height);
    }

    time_ += dt;
    hurt_ = std::max(0.0f, hurt_ - kHurtDecay * dt);
    flash_ = std::max(0.0f, flash_ - kFlashDecay * dt);
    lowHealth_ = Lerp(lowHealth_, std::clamp(lowHealth, 0.0f, 1.0f), 1.0f - std::exp(-kLowHealthSmoothing * dt));
}

void PostFx::hurt()
{
    hurt_ = 1.0f;
}

void PostFx::killFlash()
{
    flash_ = std::max(flash_, kFlashAmount);
}

void PostFx::begin() const
{
    BeginTextureMode(target_);
}

void PostFx::end() const
{
    EndTextureMode();
}

void PostFx::draw() const
{
    SetShaderValue(shader_, timeLoc_, &time_, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader_, hurtLoc_, &hurt_, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader_, lowHealthLoc_, &lowHealth_, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader_, flashLoc_, &flash_, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(shader_);
    // Les textures de rendu sont stockées à l'envers : hauteur négative pour les retourner.
    const Rectangle source = {0.0f, 0.0f, static_cast<float>(target_.texture.width),
                              -static_cast<float>(target_.texture.height)};
    const Rectangle screen = {0.0f, 0.0f, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
    DrawTexturePro(target_.texture, source, screen, {0.0f, 0.0f}, 0.0f, WHITE);
    EndShaderMode();
}
