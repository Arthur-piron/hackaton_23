#pragma once

#include "raylib.h"

// Caméra vue de dessus, inclinée, qui suit une cible avec un léger retard
// et peut trembler (tirs, coups, explosions).
class CameraRig {
public:
    CameraRig();

    void snapTo(Vector3 target);
    // lookAhead : décalage vers l'endroit visé, pour voir plus loin devant soi.
    void update(float dt, Vector3 target, Vector3 lookAhead = {0.0f, 0.0f, 0.0f});
    // Ajoute du « trauma » (0..1) : la secousse est proportionnelle à son carré.
    void shake(float amount);

    const Camera3D& camera() const { return camera_; }

private:
    void apply();

    Camera3D camera_{};
    Vector3 focus_ = {0.0f, 0.0f, 0.0f};
    Vector3 shakeOffset_ = {0.0f, 0.0f, 0.0f};
    float trauma_ = 0.0f;
    float time_ = 0.0f;
};
