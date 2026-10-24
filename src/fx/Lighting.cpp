#include "fx/Lighting.hpp"

#include "core/AssetPath.hpp"
#include "raymath.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
// Ambiance de nuit : assez sombre pour que les lumières comptent, assez claire
// pour garder les silhouettes lisibles.
constexpr Vector3 kAmbientSky = {0.10f, 0.12f, 0.22f};
constexpr Vector3 kAmbientGround = {0.03f, 0.03f, 0.06f};
constexpr Vector3 kMoonDirection = {0.35f, -1.0f, 0.45f};
constexpr Vector3 kMoonColor = {0.10f, 0.13f, 0.25f};
constexpr Vector3 kFogColor = {12.0f / 255.0f, 14.0f / 255.0f, 22.0f / 255.0f}; // fond de l'écran
constexpr float kFogStart = 15.0f;
constexpr float kFogEnd = 32.0f;
constexpr Vector3 kMistColor = {0.16f, 0.19f, 0.27f};

// Lampe torche : faisceau blanc chaud, ~14 m de portée.
constexpr Vector3 kFlashlightColor = {1.0f, 0.93f, 0.80f};
constexpr float kFlashlightOuterAngle = 27.0f; // degrés, demi-angle
constexpr float kFlashlightInnerAngle = 14.0f;
constexpr float kFlashlightRange = 14.0f;

// Flash de tir : très court, chaud, éclaire tout autour du tireur.
constexpr Color kMuzzleColor = {255, 196, 120, 255};
constexpr float kMuzzleDuration = 0.07f;
constexpr float kMuzzleRange = 8.0f;
constexpr float kMuzzleIntensity = 2.6f;

// Les lumières plus loin que ça (bord de leur portée) du point suivi sont ignorées.
constexpr float kCullDistance = 26.0f;
constexpr int kGlowTextureSize = 64;
constexpr float kHaloAlpha = 1.0f;

Vector3 toVector(Color color, float scale)
{
    return {color.r / 255.0f * scale, color.g / 255.0f * scale, color.b / 255.0f * scale};
}
}

Lighting::Lighting()
    : shader_(LoadShader(assetPath("shaders/lighting.vs").c_str(), assetPath("shaders/lighting.fs").c_str()))
{
    ambientSkyLoc_ = GetShaderLocation(shader_, "ambientSky");
    ambientGroundLoc_ = GetShaderLocation(shader_, "ambientGround");
    moonDirectionLoc_ = GetShaderLocation(shader_, "moonDirection");
    moonColorLoc_ = GetShaderLocation(shader_, "moonColor");
    lightCountLoc_ = GetShaderLocation(shader_, "lightCount");
    lightPositionsLoc_ = GetShaderLocation(shader_, "lightPositions");
    lightColorsLoc_ = GetShaderLocation(shader_, "lightColors");
    spotPositionLoc_ = GetShaderLocation(shader_, "spotPosition");
    spotDirectionLoc_ = GetShaderLocation(shader_, "spotDirection");
    spotColorLoc_ = GetShaderLocation(shader_, "spotColor");
    spotConeLoc_ = GetShaderLocation(shader_, "spotCone");
    fogColorLoc_ = GetShaderLocation(shader_, "fogColor");
    fogCenterLoc_ = GetShaderLocation(shader_, "fogCenter");
    fogRangeLoc_ = GetShaderLocation(shader_, "fogRange");
    mistColorLoc_ = GetShaderLocation(shader_, "mistColor");
    timeLoc_ = GetShaderLocation(shader_, "time");

    // Réglages fixes de la nuit : envoyés une fois pour toutes.
    const Vector3 moon = Vector3Normalize(kMoonDirection);
    const Vector2 fogRange = {kFogStart, kFogEnd};
    SetShaderValue(shader_, ambientSkyLoc_, &kAmbientSky, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, ambientGroundLoc_, &kAmbientGround, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, moonDirectionLoc_, &moon, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, moonColorLoc_, &kMoonColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, fogColorLoc_, &kFogColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, fogRangeLoc_, &fogRange, SHADER_UNIFORM_VEC2);
    SetShaderValue(shader_, mistColorLoc_, &kMistColor, SHADER_UNIFORM_VEC3);

    // Halo : dégradé radial blanc -> transparent, mélangé en additif.
    Image glow = GenImageGradientRadial(kGlowTextureSize, kGlowTextureSize, 0.0f, WHITE, BLANK);
    glow_ = LoadTextureFromImage(glow);
    UnloadImage(glow);
    SetTextureFilter(glow_, TEXTURE_FILTER_BILINEAR);
}

Lighting::~Lighting()
{
    UnloadTexture(glow_);
    UnloadShader(shader_);
}

void Lighting::apply(Model& model) const
{
    for (int i = 0; i < model.materialCount; ++i) model.materials[i].shader = shader_;
}

void Lighting::muzzleFlash(Vector3 position)
{
    muzzlePosition_ = position;
    muzzleTimer_ = kMuzzleDuration;
}

void Lighting::setFlashlight(bool on, float intensity)
{
    flashlightOn_ = on;
    flashlightIntensity_ = std::clamp(intensity, 0.0f, 1.0f);
}

void Lighting::aimFlashlight(Vector3 position, Vector3 direction)
{
    flashlightPosition_ = position;
    if (Vector3Length(direction) > 1e-4f) flashlightDirection_ = Vector3Normalize(direction);
}

float Lighting::flickerAt(const PointLight& light, int index) const
{
    if (light.flicker <= 0.0f) return light.intensity;
    // Somme de sinus déphasés par lumière : vacillement irrégulier, entre 0 et 1.
    const float i = static_cast<float>(index);
    const float wave = 0.5f + 0.25f * std::sin(time_ * 11.3f + i * 1.7f) + 0.15f * std::sin(time_ * 23.1f + i * 3.1f) +
                       0.1f * std::sin(time_ * 41.7f + i * 5.3f);
    return light.intensity * (1.0f - light.flicker * wave);
}

void Lighting::update(float dt, Vector3 focus)
{
    time_ += dt;
    muzzleTimer_ = std::max(0.0f, muzzleTimer_ - dt);
    if (muzzleTimer_ > 0.0f) {
        PointLight flash;
        flash.position = muzzlePosition_;
        flash.color = kMuzzleColor;
        flash.range = kMuzzleRange;
        flash.intensity = kMuzzleIntensity * muzzleTimer_ / kMuzzleDuration;
        dynamic_.push_back(flash);
    }

    // Lumières assez proches pour toucher l'écran, les plus proches d'abord.
    active_.clear();
    const auto consider = [&](const PointLight& light, int index) {
        const float dx = light.position.x - focus.x;
        const float dz = light.position.z - focus.z;
        if (std::sqrt(dx * dx + dz * dz) - light.range > kCullDistance) return;
        active_.push_back({light, flickerAt(light, index)});
    };
    for (size_t i = 0; i < static_.size(); ++i) consider(static_[i], static_cast<int>(i));
    for (size_t i = 0; i < dynamic_.size(); ++i) consider(dynamic_[i], static_cast<int>(static_.size() + i));
    dynamic_.clear();
    const auto distanceSq = [&](const ActiveLight& a) {
        return Vector3DistanceSqr(a.light.position, focus);
    };
    std::sort(active_.begin(), active_.end(),
              [&](const ActiveLight& a, const ActiveLight& b) { return distanceSq(a) < distanceSq(b); });
    if (active_.size() > static_cast<size_t>(kMaxLights)) active_.resize(kMaxLights);

    Vector4 positions[kMaxLights];
    Vector3 colors[kMaxLights];
    const int count = static_cast<int>(active_.size());
    for (int i = 0; i < count; ++i) {
        const ActiveLight& a = active_[static_cast<size_t>(i)];
        positions[i] = {a.light.position.x, a.light.position.y, a.light.position.z, a.light.range};
        colors[i] = toVector(a.light.color, a.strength);
    }
    SetShaderValue(shader_, lightCountLoc_, &count, SHADER_UNIFORM_INT);
    if (count > 0) {
        SetShaderValueV(shader_, lightPositionsLoc_, positions, SHADER_UNIFORM_VEC4, count);
        SetShaderValueV(shader_, lightColorsLoc_, colors, SHADER_UNIFORM_VEC3, count);
    }

    const float spotScale = flashlightOn_ ? flashlightIntensity_ : 0.0f;
    const Vector3 spotColor = Vector3Scale(kFlashlightColor, spotScale);
    const Vector3 cone = {std::cos(kFlashlightOuterAngle * DEG2RAD), std::cos(kFlashlightInnerAngle * DEG2RAD),
                          kFlashlightRange};
    SetShaderValue(shader_, spotPositionLoc_, &flashlightPosition_, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, spotDirectionLoc_, &flashlightDirection_, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, spotColorLoc_, &spotColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, spotConeLoc_, &cone, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, fogCenterLoc_, &focus, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, timeLoc_, &time_, SHADER_UNIFORM_FLOAT);
}

void Lighting::beginGlow() const
{
    // Les halos testent la profondeur sans l'écrire, et s'additionnent.
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);
}

void Lighting::endGlow() const
{
    EndBlendMode();
    rlEnableDepthMask();
}

void Lighting::drawGlow(const Camera3D& camera, Vector3 position, float size, Color color) const
{
    DrawBillboard(camera, glow_, position, size, color);
}

void Lighting::drawHalos(const Camera3D& camera) const
{
    beginGlow();
    for (const ActiveLight& a : active_) {
        if (a.light.halo <= 0.0f || a.light.intensity <= 0.0f) continue;
        const float k = a.strength / a.light.intensity;
        // Avancé vers la caméra : le halo n'est pas coupé par le modèle de la lampe.
        const Vector3 toCamera = Vector3Normalize(Vector3Subtract(camera.position, a.light.position));
        const float size = a.light.halo * (0.85f + 0.15f * k);
        const Vector3 at = Vector3Add(a.light.position, Vector3Scale(toCamera, size * 0.4f));
        Color color = a.light.color;
        color.a = static_cast<unsigned char>(255.0f * std::clamp(kHaloAlpha * k, 0.0f, 1.0f));
        drawGlow(camera, at, size, color);
    }
    endGlow();
}
