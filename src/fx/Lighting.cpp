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
constexpr Vector3 kMoonDirection = {0.35f, -1.0f, 0.45f};
// Par ambiance : ciel, sol, lune, brouillard (= fond de l'écran), brume, début et fin du brouillard.
constexpr Vector3 kNight[] = {{0.10f, 0.12f, 0.22f}, {0.03f, 0.03f, 0.06f}, {0.10f, 0.13f, 0.25f},
                              {12.0f / 255.0f, 14.0f / 255.0f, 22.0f / 255.0f}, {0.16f, 0.19f, 0.27f}};
constexpr Vector2 kNightFog = {15.0f, 32.0f};
// Lune de sang : tout baigne dans le rouge.
constexpr Vector3 kBloodMoon[] = {{0.30f, 0.06f, 0.06f}, {0.09f, 0.02f, 0.02f}, {0.42f, 0.07f, 0.05f},
                                  {0.13f, 0.03f, 0.04f}, {0.34f, 0.07f, 0.07f}};
constexpr Vector2 kBloodMoonFog = {15.0f, 32.0f};
// Brouillard : on n'y voit plus à 8 m.
constexpr Vector3 kFog[] = {{0.13f, 0.14f, 0.18f}, {0.05f, 0.05f, 0.07f}, {0.10f, 0.11f, 0.15f},
                            {0.17f, 0.18f, 0.22f}, {0.30f, 0.32f, 0.38f}};
constexpr Vector2 kFogFog = {3.0f, 9.5f};
constexpr float kMoodBlendRate = 1.2f;  // taux du fondu exponentiel (≈ 2 s pour l'essentiel)
// Coupure : les lampes électriques (peu de vacillement) grésillent puis s'éteignent.
constexpr float kElectricFlickerMax = 0.2f;
constexpr float kPowerFadeRate = 0.7f;  // par seconde
constexpr float kSputterSpeed = 37.0f;

// Lampe torche : faisceau blanc chaud, ~14 m de portée.
constexpr Vector3 kFlashlightColor = {1.0f, 0.93f, 0.80f};
constexpr float kFlashlightOuterAngle = 27.0f; // degrés, demi-angle
constexpr float kFlashlightInnerAngle = 14.0f;
constexpr float kFlashlightRange = 14.0f;

// Les lumières plus loin que ça (bord de leur portée) du point suivi sont ignorées.
constexpr float kCullDistance = 26.0f;
constexpr int kGlowTextureSize = 64;
constexpr float kHaloAlpha = 1.0f;

Vector3 toVector(Color color, float scale)
{
    return {color.r / 255.0f * scale, color.g / 255.0f * scale, color.b / 255.0f * scale};
}
}

Lighting::Atmosphere Lighting::atmosphereOf(Mood mood)
{
    const Vector3* colors = kNight;
    Vector2 fog = kNightFog;
    if (mood == Mood::BloodMoon) {
        colors = kBloodMoon;
        fog = kBloodMoonFog;
    } else if (mood == Mood::Fog) {
        colors = kFog;
        fog = kFogFog;
    }
    return {colors[0], colors[1], colors[2], colors[3], colors[4], fog.x, fog.y};
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

    // La direction de la lune ne change pas ; les couleurs suivent l'ambiance (sendAtmosphere).
    const Vector3 moon = Vector3Normalize(kMoonDirection);
    SetShaderValue(shader_, moonDirectionLoc_, &moon, SHADER_UNIFORM_VEC3);
    atmosphere_ = atmosphereOf(Mood::Night);
    sendAtmosphere(0.0f);

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

void Lighting::flash(const PointLight& light, float duration)
{
    if (duration > 0.0f) flashes_.push_back({light, duration, duration});
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

void Lighting::setMood(Mood mood)
{
    mood_ = mood;
}

Color Lighting::backgroundColor() const
{
    const Vector3& fog = atmosphere_.fogColor;
    return {static_cast<unsigned char>(fog.x * 255.0f), static_cast<unsigned char>(fog.y * 255.0f),
            static_cast<unsigned char>(fog.z * 255.0f), 255};
}

void Lighting::sendAtmosphere(float dt)
{
    // Fondu exponentiel vers l'ambiance voulue.
    const Atmosphere target = atmosphereOf(mood_);
    const float k = 1.0f - std::exp(-kMoodBlendRate * dt);
    Atmosphere& a = atmosphere_;
    a.ambientSky = Vector3Lerp(a.ambientSky, target.ambientSky, k);
    a.ambientGround = Vector3Lerp(a.ambientGround, target.ambientGround, k);
    a.moonColor = Vector3Lerp(a.moonColor, target.moonColor, k);
    a.fogColor = Vector3Lerp(a.fogColor, target.fogColor, k);
    a.mistColor = Vector3Lerp(a.mistColor, target.mistColor, k);
    a.fogStart += (target.fogStart - a.fogStart) * k;
    a.fogEnd += (target.fogEnd - a.fogEnd) * k;
    const Vector2 fogRange = {a.fogStart, a.fogEnd};
    SetShaderValue(shader_, ambientSkyLoc_, &a.ambientSky, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, ambientGroundLoc_, &a.ambientGround, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, moonColorLoc_, &a.moonColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, fogColorLoc_, &a.fogColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, fogRangeLoc_, &fogRange, SHADER_UNIFORM_VEC2);
    SetShaderValue(shader_, mistColorLoc_, &a.mistColor, SHADER_UNIFORM_VEC3);
}

float Lighting::powerAt(const PointLight& light, int index) const
{
    if (light.flicker >= kElectricFlickerMax || power_ >= 1.0f) return 1.0f;
    if (power_ <= 0.0f) return 0.0f;
    // Pendant la transition, l'ampoule grésille : allumée, éteinte, à demi...
    const float sputter = std::sin(time_ * kSputterSpeed + static_cast<float>(index) * 2.3f);
    return power_ * (sputter > 0.0f ? 1.0f : 0.15f);
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
    power_ = std::clamp(power_ + (powerCut_ ? -kPowerFadeRate : kPowerFadeRate) * dt, 0.0f, 1.0f);
    sendAtmosphere(dt);
    for (Flash& f : flashes_) {
        f.left -= dt;
        if (f.left <= 0.0f) continue;
        PointLight light = f.light;
        light.intensity *= f.left / f.duration;
        dynamic_.push_back(light);
    }
    flashes_.erase(std::remove_if(flashes_.begin(), flashes_.end(), [](const Flash& f) { return f.left <= 0.0f; }),
                   flashes_.end());

    // Lumières assez proches pour toucher l'écran, les plus proches d'abord.
    active_.clear();
    const auto consider = [&](const PointLight& light, int index, float power) {
        const float dx = light.position.x - focus.x;
        const float dz = light.position.z - focus.z;
        if (power <= 0.0f || std::sqrt(dx * dx + dz * dz) - light.range > kCullDistance) return;
        active_.push_back({light, flickerAt(light, index) * power});
    };
    for (size_t i = 0; i < static_.size(); ++i) {
        consider(static_[i], static_cast<int>(i), powerAt(static_[i], static_cast<int>(i)));
    }
    for (size_t i = 0; i < dynamic_.size(); ++i) consider(dynamic_[i], static_cast<int>(static_.size() + i), 1.0f);
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
