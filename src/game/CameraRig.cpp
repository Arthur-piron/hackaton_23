#include "game/CameraRig.hpp"

#include "raymath.h"

namespace {
// Décalage de la caméra par rapport au point suivi : haut et un peu en arrière.
constexpr Vector3 kOffset = {0.0f, 10.5f, 9.5f};
constexpr float kFovY = 45.0f;
constexpr float kFollowSharpness = 8.0f;
}

CameraRig::CameraRig()
{
    camera_.up = {0.0f, 1.0f, 0.0f};
    camera_.fovy = kFovY;
    camera_.projection = CAMERA_PERSPECTIVE;
    applyFocus();
}

void CameraRig::snapTo(Vector3 target)
{
    focus_ = target;
    applyFocus();
}

void CameraRig::update(float dt, Vector3 target)
{
    focus_ = Vector3Lerp(focus_, target, 1.0f - expf(-kFollowSharpness * dt));
    applyFocus();
}

void CameraRig::applyFocus()
{
    camera_.target = focus_;
    camera_.position = Vector3Add(focus_, kOffset);
}
