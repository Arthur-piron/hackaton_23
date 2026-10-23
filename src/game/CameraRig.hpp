#pragma once

#include "raylib.h"

// Caméra vue de dessus, inclinée, qui suit une cible avec un léger retard.
class CameraRig {
public:
    CameraRig();

    void snapTo(Vector3 target);
    void update(float dt, Vector3 target);

    const Camera3D& camera() const { return camera_; }

private:
    void applyFocus();

    Camera3D camera_{};
    Vector3 focus_ = {0.0f, 0.0f, 0.0f};
};
