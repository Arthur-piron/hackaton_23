#include "game/CameraRig.hpp"

#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
// Décalage de la caméra par rapport au point suivi : haut et un peu en arrière.
constexpr Vector3 kOffset = {0.0f, 10.5f, 9.5f};
constexpr float kFovY = 45.0f;
constexpr float kFollowSharpness = 8.0f;
constexpr float kTraumaDecay = 1.6f;
constexpr float kMaxShake = 0.55f;
// Fréquences différentes par axe : un tremblement organique plutôt que périodique.
constexpr float kShakeFrequencyX = 37.0f;
constexpr float kShakeFrequencyZ = 29.0f;
}

CameraRig::CameraRig()
{
    camera_.up = {0.0f, 1.0f, 0.0f};
    camera_.fovy = kFovY;
    camera_.projection = CAMERA_PERSPECTIVE;
    apply();
}

void CameraRig::snapTo(Vector3 target)
{
    focus_ = target;
    trauma_ = 0.0f;
    shakeOffset_ = {0.0f, 0.0f, 0.0f};
    apply();
}

void CameraRig::shake(float amount)
{
    trauma_ = std::min(1.0f, trauma_ + amount);
}

void CameraRig::update(float dt, Vector3 target, Vector3 lookAhead)
{
    time_ += dt;
    focus_ = Vector3Lerp(focus_, Vector3Add(target, lookAhead), 1.0f - std::exp(-kFollowSharpness * dt));
    trauma_ = std::max(0.0f, trauma_ - kTraumaDecay * dt);
    const float power = trauma_ * trauma_ * kMaxShake;
    shakeOffset_ = {std::sin(time_ * kShakeFrequencyX) * power, 0.0f,
                    std::sin(time_ * kShakeFrequencyZ + 1.3f) * power};
    apply();
}

void CameraRig::apply()
{
    camera_.target = Vector3Add(focus_, shakeOffset_);
    camera_.position = Vector3Add(camera_.target, kOffset);
}
