#include "game/Projectile.hpp"

#include "game/World.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kTracerLength = 0.9f;
constexpr Color kTracerHead = {255, 240, 180, 255};
constexpr Color kTracerTail = {255, 170, 60, 0};
}

void ProjectileSystem::update(float dt)
{
    for (Projectile& p : projectiles_) {
        p.previous = p.position;
        p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
        p.life -= dt;
        const float limit = World::kHalfSize + 2.0f;
        if (p.life <= 0.0f || std::fabs(p.position.x) > limit || std::fabs(p.position.z) > limit) p.alive = false;
    }
    projectiles_.erase(std::remove_if(projectiles_.begin(), projectiles_.end(),
                                      [](const Projectile& p) { return !p.alive; }),
                       projectiles_.end());
}

void ProjectileSystem::draw() const
{
    for (const Projectile& p : projectiles_) {
        const Vector3 dir = Vector3Normalize(p.velocity);
        const Vector3 tail = Vector3Subtract(p.position, Vector3Scale(dir, kTracerLength));
        DrawCylinderEx(tail, p.position, 0.0f, 0.045f, 4, kTracerTail);
        DrawSphere(p.position, 0.06f, kTracerHead);
        DrawLine3D(tail, p.position, kTracerHead);
    }
}

bool segmentHitsSphere(Vector3 a, Vector3 b, Vector3 center, float radius, float& t)
{
    const Vector3 ab = Vector3Subtract(b, a);
    const float lengthSq = Vector3DotProduct(ab, ab);
    t = lengthSq > 0.0f ? std::clamp(Vector3DotProduct(Vector3Subtract(center, a), ab) / lengthSq, 0.0f, 1.0f) : 0.0f;
    const Vector3 closest = Vector3Add(a, Vector3Scale(ab, t));
    return Vector3DistanceSqr(closest, center) <= radius * radius;
}
