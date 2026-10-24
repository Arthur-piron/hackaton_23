#include "fx/Particles.hpp"

#include "core/AssetPath.hpp"
#include "raymath.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {
constexpr size_t kMaxParticles = 2048;
constexpr size_t kMaxDecals = 150;
constexpr float kPoolLife = 20.0f;   // flaque laissée par un mort
constexpr float kStainLife = 8.0f;   // goutte tombée au sol
constexpr float kDecalFade = 4.0f;    // estompage pendant les dernières secondes
constexpr float kDecalSpread = 0.3f;  // la flaque s'étale pendant ce temps
constexpr float kDecalHeight = 0.18f; // au-dessus du sol et des dalles des allées (0,16 m)
constexpr float kGroundY = 0.0f;
constexpr float kDustInterval = 0.06f;
constexpr int kStainChance = 6;       // une goutte sur 6 laisse une trace au sol

// Même ordre que l'énumération Tex.
const char* const kTextureFiles[] = {"muzzle_01.png", "muzzle_02.png", "muzzle_03.png", "muzzle_04.png",
                                     "light_01.png",  "flare_01.png",  "smoke_01.png",  "dirt_01.png",
                                     "dirt_02.png",   "scorch_01.png"};

constexpr Color kFlashCore = {255, 235, 180, 255};
constexpr Color kFlashEnd = {255, 140, 50, 0};
constexpr Color kFlashGlow = {255, 175, 80, 255};
constexpr Color kFlashGlowEnd = {255, 110, 30, 0};
constexpr Color kSpark = {255, 210, 120, 255};
constexpr Color kSparkEnd = {255, 80, 20, 0};
constexpr Color kGunSmoke = {160, 155, 145, 100};
constexpr Color kGunSmokeEnd = {120, 118, 112, 0};
constexpr Color kBlood = {175, 14, 16, 255};
constexpr Color kBloodEnd = {110, 6, 8, 220};
constexpr Color kBloodMist = {150, 12, 14, 110};
constexpr Color kBloodMistEnd = {70, 4, 6, 0};
constexpr Color kBloodPool = {110, 8, 10, 235};
constexpr Color kBloodStain = {120, 10, 12, 200};
constexpr Color kDust = {150, 138, 118, 170};
constexpr Color kDustEnd = {100, 92, 80, 0};

float randomRange(float min, float max)
{
    return min + (max - min) * static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f;
}

// Direction perturbée aléatoirement autour de axis (amount ≈ ouverture du cône).
Vector3 jitter(Vector3 axis, float amount)
{
    const Vector3 noise = {randomRange(-amount, amount), randomRange(-amount, amount), randomRange(-amount, amount)};
    return Vector3Normalize(Vector3Add(axis, noise));
}

// Direction horizontale normalisée (repli sur +Z si la direction est nulle).
Vector3 flatten(Vector3 v)
{
    const Vector3 flat = {v.x, 0.0f, v.z};
    return Vector3Length(flat) > 1e-4f ? Vector3Normalize(flat) : Vector3{0.0f, 0.0f, 1.0f};
}

Color lerpColor(Color a, Color b, float t)
{
    const auto mix = [t](unsigned char x, unsigned char y) {
        return static_cast<unsigned char>(static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t);
    };
    return {mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a)};
}

void vertex(Vector3 v)
{
    rlVertex3f(v.x, v.y, v.z);
}

// Quad texturé : axis = haut de la texture, side = droite.
void emitQuad(Vector3 center, Vector3 axis, Vector3 side, float halfLength, float halfWidth, Color color)
{
    const Vector3 a = Vector3Scale(axis, halfLength);
    const Vector3 s = Vector3Scale(side, halfWidth);
    rlBegin(RL_QUADS);
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlTexCoord2f(0.0f, 0.0f);
    vertex(Vector3Subtract(Vector3Add(center, a), s));
    rlTexCoord2f(0.0f, 1.0f);
    vertex(Vector3Subtract(Vector3Subtract(center, a), s));
    rlTexCoord2f(1.0f, 1.0f);
    vertex(Vector3Add(Vector3Subtract(center, a), s));
    rlTexCoord2f(1.0f, 0.0f);
    vertex(Vector3Add(Vector3Add(center, a), s));
    rlEnd();
}
}

Particles::Particles()
{
    for (int i = 0; i < kTexCount; ++i) {
        textures_[i] = LoadTexture(assetPath(std::string("textures/particles/") + kTextureFiles[i]).c_str());
        GenTextureMipmaps(&textures_[i]);
        SetTextureFilter(textures_[i], TEXTURE_FILTER_TRILINEAR);
    }
    particles_.reserve(kMaxParticles);
    decals_.assign(kMaxDecals, Decal{});
}

Particles::~Particles()
{
    for (Texture2D& texture : textures_) UnloadTexture(texture);
}

void Particles::clear()
{
    particles_.clear();
    for (Decal& d : decals_) d.life = 0.0f;
    dustTimer_ = 0.0f;
}

Particles::Particle& Particles::spawn()
{
    if (particles_.size() < kMaxParticles) {
        particles_.push_back(Particle{});
        return particles_.back();
    }
    // Pool plein : on recycle les particules à tour de rôle.
    Particle& p = particles_[nextOverwrite_];
    nextOverwrite_ = (nextOverwrite_ + 1) % kMaxParticles;
    p = Particle{};
    return p;
}

void Particles::addDecal(Vector3 position, float size, float life, Color color, Tex texture)
{
    // Emplacement libre, sinon la tache la plus proche de sa fin : les flaques
    // des morts survivent aux gouttes d'une rafale.
    size_t slot = 0;
    for (size_t i = 1; i < decals_.size(); ++i) {
        if (decals_[i].life - decals_[i].age < decals_[slot].life - decals_[slot].age) slot = i;
    }
    decals_[slot] = {{position.x, kDecalHeight, position.z}, randomRange(0.0f, 2.0f * PI), size, 0.0f, life, color,
                     texture};
}

void Particles::update(float dt)
{
    for (size_t i = 0; i < particles_.size();) {
        Particle& p = particles_[i];
        p.age += dt;
        p.velocity.y -= p.gravity * dt;
        p.velocity = Vector3Scale(p.velocity, std::max(0.0f, 1.0f - p.drag * dt));
        p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
        p.rotation += p.spin * dt;

        bool dead = p.age >= p.life;
        if (p.gravity > 0.0f && p.position.y <= kGroundY && p.velocity.y < 0.0f) {
            if (p.landing == Landing::Stain) {
                addDecal(p.position, randomRange(0.25f, 0.5f), kStainLife, kBloodStain,
                         GetRandomValue(0, 1) ? kSplat1 : kSplat2);
            }
            if (p.landing != Landing::Bounce) {
                dead = true;
            } else {
                // Rebond amorti des étincelles.
                p.position.y = kGroundY;
                p.velocity = {p.velocity.x * 0.5f, -p.velocity.y * 0.3f, p.velocity.z * 0.5f};
            }
        }
        if (dead) {
            p = particles_.back();
            particles_.pop_back();
        } else {
            ++i;
        }
    }
    for (Decal& d : decals_) {
        if (d.age < d.life) d.age += dt;
    }
}

void Particles::draw(const Camera3D& camera) const
{
    // Les particules testent la profondeur mais ne l'écrivent pas : elles se
    // superposent sans se masquer entre elles.
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    drawDecals();
    drawParticles(camera, false);
    BeginBlendMode(BLEND_ADDITIVE);
    drawParticles(camera, true);
    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}

void Particles::drawDecals() const
{
    for (const Decal& d : decals_) {
        if (d.age >= d.life) continue;
        const float spread = std::min(1.0f, d.age / kDecalSpread);
        const float size = d.size * (1.0f - 0.6f * (1.0f - spread) * (1.0f - spread));
        Color color = d.color;
        color.a = static_cast<unsigned char>(color.a * std::clamp((d.life - d.age) / kDecalFade, 0.0f, 1.0f));
        const Vector3 axis = {std::sin(d.rotation), 0.0f, std::cos(d.rotation)};
        const Vector3 side = {std::cos(d.rotation), 0.0f, -std::sin(d.rotation)};
        rlSetTexture(textures_[d.texture].id);
        emitQuad(d.position, axis, side, size * 0.5f, size * 0.5f, color);
    }
    rlSetTexture(0);
}

void Particles::drawParticles(const Camera3D& camera, bool additive) const
{
    const Matrix view = GetCameraMatrix(camera);
    const Vector3 right = {view.m0, view.m4, view.m8};
    const Vector3 up = {view.m1, view.m5, view.m9};

    // Regroupées par texture pour limiter les appels de dessin.
    for (int t = 0; t < kTexCount; ++t) {
        bool bound = false;
        for (const Particle& p : particles_) {
            if (p.additive != additive || p.texture != t) continue;
            if (!bound) {
                rlSetTexture(textures_[t].id);
                bound = true;
            }
            const float k = p.age / p.life;
            const float size = Lerp(p.sizeStart, p.sizeEnd, k);
            const Color color = lerpColor(p.colorStart, p.colorEnd, k);
            const float c = std::cos(p.rotation);
            const float s = std::sin(p.rotation);

            switch (p.shape) {
            case Shape::Billboard:
                emitQuad(p.position, Vector3Subtract(Vector3Scale(up, c), Vector3Scale(right, s)),
                         Vector3Add(Vector3Scale(right, c), Vector3Scale(up, s)), size * 0.5f, size * 0.5f, color);
                break;
            case Shape::Stretched: {
                // Allongée dans le sens du mouvement, tournée vers la caméra.
                const float speed = Vector3Length(p.velocity);
                const Vector3 axis = speed > 1e-3f ? Vector3Scale(p.velocity, 1.0f / speed) : up;
                Vector3 side = Vector3CrossProduct(axis, Vector3Subtract(camera.position, p.position));
                side = Vector3Length(side) > 1e-4f ? Vector3Normalize(side) : right;
                emitQuad(p.position, axis, side, (size + speed * p.stretch) * 0.5f, size * 0.5f, color);
                break;
            }
            case Shape::Flat:
                emitQuad(p.position, {s, 0.0f, c}, {c, 0.0f, -s}, size * 0.5f, size * 0.5f, color);
                break;
            }
        }
        if (bound) rlSetTexture(0);
    }
}

void Particles::muzzleFlash(Vector3 muzzle, Vector3 forward, float strength)
{
    const Vector3 dir = flatten(forward);
    const float yaw = std::atan2(dir.x, dir.z);

    // Flamme à plat, orientée vers l'avant (la texture pointe vers le haut).
    Particle& flame = spawn();
    const float flameSize = 1.0f * strength;
    flame.position = Vector3Add(muzzle, Vector3Scale(dir, flameSize * 0.35f));
    flame.life = 0.06f;
    flame.sizeStart = flameSize;
    flame.sizeEnd = flameSize * 1.2f;
    flame.rotation = yaw;
    flame.colorStart = kFlashCore;
    flame.colorEnd = kFlashEnd;
    flame.texture = static_cast<Tex>(kMuzzle1 + GetRandomValue(0, 3));
    flame.shape = Shape::Flat;
    flame.additive = true;

    Particle& glow = spawn();
    glow.position = Vector3Add(muzzle, Vector3Scale(dir, 0.15f));
    glow.life = 0.07f;
    glow.sizeStart = 2.6f * strength;
    glow.sizeEnd = 3.0f * strength;
    glow.colorStart = kFlashGlow;
    glow.colorEnd = kFlashGlowEnd;
    glow.texture = kFlare;
    glow.shape = Shape::Billboard;
    glow.additive = true;

    const int sparks = 2 + static_cast<int>(2.0f * strength);
    for (int i = 0; i < sparks; ++i) {
        Particle& p = spawn();
        p.position = muzzle;
        p.velocity = Vector3Scale(jitter(dir, 0.35f), randomRange(6.0f, 12.0f));
        p.gravity = 6.0f;
        p.drag = 2.0f;
        p.life = randomRange(0.12f, 0.25f);
        p.sizeStart = 0.12f;
        p.sizeEnd = 0.06f;
        p.stretch = 0.05f;
        p.colorStart = kSpark;
        p.colorEnd = kSparkEnd;
        p.texture = kGlow;
        p.shape = Shape::Stretched;
        p.additive = true;
    }

    for (int i = 0; i < 2; ++i) {
        Particle& p = spawn();
        p.position = Vector3Add(muzzle, Vector3Scale(dir, 0.2f));
        p.velocity = Vector3Add(Vector3Scale(jitter(dir, 0.4f), randomRange(0.5f, 1.5f)), {0.0f, 0.4f, 0.0f});
        p.drag = 2.5f;
        p.life = randomRange(0.6f, 1.0f);
        p.sizeStart = 0.25f;
        p.sizeEnd = 1.0f * strength;
        p.rotation = randomRange(0.0f, 2.0f * PI);
        p.spin = randomRange(-1.0f, 1.0f);
        p.colorStart = kGunSmoke;
        p.colorEnd = kGunSmokeEnd;
        p.texture = kSmoke;
        p.shape = Shape::Billboard;
    }
}

void Particles::bloodSpray(Vector3 position, Vector3 direction, int drops, float force)
{
    const Vector3 dir = flatten(direction);
    const Vector3 launch = Vector3Normalize(Vector3Add(dir, {0.0f, 0.45f, 0.0f}));
    for (int i = 0; i < drops; ++i) {
        Particle& p = spawn();
        p.position = position;
        p.velocity = Vector3Scale(jitter(launch, 0.55f), force * randomRange(0.35f, 1.0f));
        p.gravity = 14.0f;
        p.drag = 0.8f;
        p.life = 1.5f;
        p.sizeStart = randomRange(0.1f, 0.2f);
        p.sizeEnd = p.sizeStart * 0.7f;
        p.stretch = 0.03f;
        p.colorStart = kBlood;
        p.colorEnd = kBloodEnd;
        p.texture = kGlow;
        p.shape = Shape::Stretched;
        p.landing = GetRandomValue(1, kStainChance) == 1 ? Landing::Stain : Landing::Vanish;
    }

    for (int i = 0; i < 2; ++i) {
        Particle& p = spawn();
        p.position = position;
        p.velocity = Vector3Scale(jitter(dir, 0.3f), randomRange(1.0f, 2.0f));
        p.drag = 3.0f;
        p.life = 0.35f;
        p.sizeStart = 0.3f;
        p.sizeEnd = 0.9f;
        p.rotation = randomRange(0.0f, 2.0f * PI);
        p.colorStart = kBloodMist;
        p.colorEnd = kBloodMistEnd;
        p.texture = kSmoke;
        p.shape = Shape::Billboard;
    }
}

void Particles::bloodHit(Vector3 position, Vector3 direction)
{
    bloodSpray(position, direction, 10, 5.0f);
}

void Particles::bloodKill(Vector3 position, Vector3 direction)
{
    bloodSpray(position, direction, 26, 7.0f);
    const Vector3 dir = flatten(direction);
    const Vector3 ground = {position.x + dir.x * 0.4f, 0.0f, position.z + dir.z * 0.4f};
    addDecal(ground, randomRange(1.6f, 2.2f), kPoolLife, kBloodPool, GetRandomValue(0, 1) ? kSplat1 : kSplat2);
    addDecal(Vector3Add(ground, Vector3Scale(dir, 0.6f)), randomRange(1.8f, 2.4f), kPoolLife, kBloodStain, kSplash);
}

void Particles::impact(Vector3 position, Vector3 normal)
{
    const Vector3 n = Vector3Normalize(normal);
    for (int i = 0; i < 8; ++i) {
        Particle& p = spawn();
        p.position = position;
        p.velocity = Vector3Scale(jitter(n, 0.8f), randomRange(3.0f, 8.0f));
        p.gravity = 9.8f;
        p.drag = 1.0f;
        p.life = randomRange(0.25f, 0.5f);
        p.sizeStart = 0.1f;
        p.sizeEnd = 0.05f;
        p.stretch = 0.04f;
        p.colorStart = kSpark;
        p.colorEnd = kSparkEnd;
        p.texture = kGlow;
        p.shape = Shape::Stretched;
        p.additive = true;
    }

    Particle& flash = spawn();
    flash.position = Vector3Add(position, Vector3Scale(n, 0.05f));
    flash.life = 0.06f;
    flash.sizeStart = 1.2f;
    flash.sizeEnd = 0.8f;
    flash.colorStart = kFlashGlow;
    flash.colorEnd = kFlashGlowEnd;
    flash.texture = kFlare;
    flash.shape = Shape::Billboard;
    flash.additive = true;

    for (int i = 0; i < 3; ++i) {
        Particle& p = spawn();
        p.position = position;
        p.velocity = Vector3Add(Vector3Scale(jitter(n, 0.5f), randomRange(0.3f, 1.0f)), {0.0f, 0.3f, 0.0f});
        p.drag = 2.0f;
        p.life = randomRange(0.5f, 0.8f);
        p.sizeStart = 0.2f;
        p.sizeEnd = 0.8f;
        p.rotation = randomRange(0.0f, 2.0f * PI);
        p.spin = randomRange(-1.0f, 1.0f);
        p.colorStart = kDust;
        p.colorEnd = kDustEnd;
        p.texture = kSmoke;
        p.shape = Shape::Billboard;
    }
}

void Particles::sprintDust(Vector3 feet, Vector3 direction, float dt)
{
    const Vector3 dir = flatten(direction);
    dustTimer_ -= dt;
    while (dustTimer_ <= 0.0f) {
        dustTimer_ += kDustInterval;
        Particle& p = spawn();
        p.position = {feet.x + randomRange(-0.2f, 0.2f), 0.05f, feet.z + randomRange(-0.2f, 0.2f)};
        p.velocity = Vector3Add(Vector3Scale(dir, -randomRange(0.3f, 0.9f)), {0.0f, randomRange(0.2f, 0.5f), 0.0f});
        p.drag = 2.0f;
        p.life = randomRange(0.5f, 0.8f);
        p.sizeStart = 0.2f;
        p.sizeEnd = 0.7f;
        p.rotation = randomRange(0.0f, 2.0f * PI);
        p.spin = randomRange(-1.0f, 1.0f);
        p.colorStart = kDust;
        p.colorEnd = kDustEnd;
        p.texture = kSmoke;
        p.shape = Shape::Billboard;
    }
}
