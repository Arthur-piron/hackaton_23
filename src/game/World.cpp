#include "game/World.hpp"

#include "core/AssetPath.hpp"
#include "game/Scale.hpp"
#include "raymath.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>

namespace {
// Sol : un grand plan texturé d'un bruit herbe / terre, centré sur la carte.
constexpr float kGroundSize = 150.0f;
constexpr Vector3 kGroundCenter = {0.0f, 0.0f, -16.0f};
constexpr int kGroundTextureSize = 512;
constexpr float kGroundNoiseScale = 9.0f;
constexpr Color kGrassColor = {30, 44, 30, 255};
constexpr Color kDirtColor = {58, 50, 38, 255};

// Les décors plus loin que ça du point visé par la caméra ne sont pas dessinés.
constexpr float kDrawRadius = 42.0f;
// Les murs fins (grilles) sont épaissis pour que rien ne passe à travers.
constexpr float kMinBlockerThickness = 0.5f;
// Collision « tronc » : fraction de l'emprise au sol de l'arbre.
constexpr float kTrunkFraction = 0.32f;
constexpr int kResolvePasses = 2;

Color lerpColor(Color a, Color b, float t)
{
    return {static_cast<unsigned char>(a.r + (b.r - a.r) * t), static_cast<unsigned char>(a.g + (b.g - a.g) * t),
            static_cast<unsigned char>(a.b + (b.b - a.b) * t), 255};
}

BoundingBox thicken(BoundingBox box)
{
    const float half = kMinBlockerThickness * 0.5f;
    const float cx = (box.min.x + box.max.x) * 0.5f;
    const float cz = (box.min.z + box.max.z) * 0.5f;
    box.min.x = std::min(box.min.x, cx - half);
    box.max.x = std::max(box.max.x, cx + half);
    box.min.z = std::min(box.min.z, cz - half);
    box.max.z = std::max(box.max.z, cz + half);
    return box;
}

BoundingBox trunkOf(BoundingBox box)
{
    const float cx = (box.min.x + box.max.x) * 0.5f;
    const float cz = (box.min.z + box.max.z) * 0.5f;
    const float half = std::min(box.max.x - box.min.x, box.max.z - box.min.z) * kTrunkFraction * 0.5f;
    return {{cx - half, box.min.y, cz - half}, {cx + half, box.max.y, cz + half}};
}

// Intersection segment / boîte (méthode des dalles) ; t = position du point d'entrée (0..1).
bool segmentHitsBox(Vector3 a, Vector3 b, const BoundingBox& box, float& t)
{
    const float origin[3] = {a.x, a.y, a.z};
    const float delta[3] = {b.x - a.x, b.y - a.y, b.z - a.z};
    const float lo[3] = {box.min.x, box.min.y, box.min.z};
    const float hi[3] = {box.max.x, box.max.y, box.max.z};
    float enter = 0.0f;
    float exit = 1.0f;
    for (int axis = 0; axis < 3; ++axis) {
        if (std::fabs(delta[axis]) < 1e-8f) {
            if (origin[axis] < lo[axis] || origin[axis] > hi[axis]) return false;
            continue;
        }
        float t0 = (lo[axis] - origin[axis]) / delta[axis];
        float t1 = (hi[axis] - origin[axis]) / delta[axis];
        if (t0 > t1) std::swap(t0, t1);
        enter = std::max(enter, t0);
        exit = std::min(exit, t1);
        if (enter > exit) return false;
    }
    t = enter;
    return true;
}
}

World::World(const MapData& map)
    : playerSpawn_(map.playerSpawn)
{
    createGround();
    for (const PropDef& prop : map.props) addProp(prop);
    for (const WallDef& wall : map.walls) addWall(wall);
}

World::~World()
{
    for (auto& entry : models_) UnloadModel(entry.second);
    for (auto& entry : textures_) UnloadTexture(entry.second);
    UnloadModel(ground_);
    UnloadTexture(groundTexture_);
}

void World::createGround()
{
    // Bruit de Perlin teinté entre herbe sombre et terre : pas de damier, pas de répétition visible.
    Image noise = GenImagePerlinNoise(kGroundTextureSize, kGroundTextureSize, 0, 0, kGroundNoiseScale);
    Image image = GenImageColor(kGroundTextureSize, kGroundTextureSize, kGrassColor);
    for (int y = 0; y < kGroundTextureSize; ++y) {
        for (int x = 0; x < kGroundTextureSize; ++x) {
            const float n = GetImageColor(noise, x, y).r / 255.0f;
            const float dirt = std::clamp((n - 0.45f) * 2.5f, 0.0f, 1.0f);
            ImageDrawPixel(&image, x, y, lerpColor(kGrassColor, kDirtColor, dirt));
        }
    }
    UnloadImage(noise);
    groundTexture_ = LoadTextureFromImage(image);
    UnloadImage(image);
    GenTextureMipmaps(&groundTexture_);
    SetTextureFilter(groundTexture_, TEXTURE_FILTER_TRILINEAR);

    ground_ = LoadModelFromMesh(GenMeshPlane(kGroundSize, kGroundSize, 1, 1));
    ground_.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = groundTexture_;
}

const Model& World::model(const std::string& file)
{
    auto it = models_.find(file);
    if (it != models_.end()) return it->second;

    Model loaded = LoadModel(assetPath("models/" + file).c_str());
    // Tous les modèles d'un kit partagent Textures/colormap.png : une seule copie par kit.
    // (UnloadModel ne libère pas les textures : elles sont libérées dans le destructeur.)
    const std::string kit = file.substr(0, file.find('/'));
    for (int i = 0; i < loaded.materialCount; ++i) {
        Texture2D& texture = loaded.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture;
        if (texture.id == 0 || texture.id == rlGetTextureIdDefault()) continue;
        auto shared = textures_.find(kit);
        if (shared == textures_.end()) {
            textures_.emplace(kit, texture);
        } else if (shared->second.id != texture.id) {
            UnloadTexture(texture);
            texture = shared->second;
        }
    }
    return models_.emplace(file, loaded).first->second;
}

BoundingBox World::placedBounds(const Model& model, Vector3 position, float yaw, float scale)
{
    const BoundingBox local = GetModelBoundingBox(model);
    const float s = kKenneyScale * scale;
    const Matrix transform = MatrixMultiply(MatrixMultiply(MatrixScale(s, s, s), MatrixRotateY(yaw * DEG2RAD)),
                                            MatrixTranslate(position.x, position.y, position.z));
    BoundingBox box = {{1e9f, 1e9f, 1e9f}, {-1e9f, -1e9f, -1e9f}};
    for (int corner = 0; corner < 8; ++corner) {
        const Vector3 p = Vector3Transform({(corner & 1) ? local.max.x : local.min.x,
                                            (corner & 2) ? local.max.y : local.min.y,
                                            (corner & 4) ? local.max.z : local.min.z},
                                           transform);
        box.min = Vector3Min(box.min, p);
        box.max = Vector3Max(box.max, p);
    }
    return box;
}

Vector3 World::centeredPosition(const Model& model, Vector3 center, float yaw, Vector3 scale)
{
    const BoundingBox box = GetModelBoundingBox(model);
    const Vector3 local = {(box.min.x + box.max.x) * 0.5f * kKenneyScale * scale.x, 0.0f,
                           (box.min.z + box.max.z) * 0.5f * kKenneyScale * scale.z};
    const Vector3 offset = Vector3Transform(local, MatrixRotateY(yaw * DEG2RAD));
    return {center.x - offset.x, center.y, center.z - offset.z};
}

void World::addProp(const PropDef& prop)
{
    const Model& m = model(prop.model);
    props_.push_back({&m, prop.position, prop.yaw, prop.scale});
    if (prop.collision == PropCollision::None) return;
    const BoundingBox box = placedBounds(m, prop.position, prop.yaw, prop.scale);
    addBlocker(prop.collision == PropCollision::Trunk ? trunkOf(box) : thicken(box));
}

void World::addWall(const WallDef& wall)
{
    const Model& m = model(wall.model);
    const BoundingBox local = GetModelBoundingBox(m);
    const float pieceLength = (local.max.x - local.min.x) * kKenneyScale * wall.scale;
    const Vector2 delta = Vector2Subtract(wall.to, wall.from);
    const float length = Vector2Length(delta);
    if (length < 1e-3f || pieceLength < 1e-3f) return;

    // Les pièces Kenney s'étendent le long de leur axe X local.
    const float yaw = std::atan2(-delta.y, delta.x) * RAD2DEG;
    const int count = std::max(1, static_cast<int>(std::lround(length / pieceLength)));
    const Vector2 dir = Vector2Scale(delta, 1.0f / length);
    const float spacing = length / static_cast<float>(count);
    for (int i = 0; i < count; ++i) {
        const Vector2 c = Vector2Add(wall.from, Vector2Scale(dir, spacing * (static_cast<float>(i) + 0.5f)));
        const Vector3 position = centeredPosition(m, {c.x, 0.0f, c.y}, yaw, {wall.scale, wall.scale, wall.scale});
        props_.push_back({&m, position, yaw, wall.scale});
        addBlocker(thicken(placedBounds(m, position, yaw, wall.scale)));
    }
}

int World::addBlocker(BoundingBox box)
{
    blockers_.push_back({box, true});
    return static_cast<int>(blockers_.size()) - 1;
}

void World::setBlockerActive(int id, bool active)
{
    blockers_[static_cast<size_t>(id)].active = active;
}

void World::draw(const Camera3D& camera) const
{
    DrawModel(ground_, kGroundCenter, 1.0f, WHITE);
    const float radiusSq = kDrawRadius * kDrawRadius;
    for (const Placed& p : props_) {
        const float dx = p.position.x - camera.target.x;
        const float dz = p.position.z - camera.target.z;
        if (dx * dx + dz * dz > radiusSq) continue;
        const float s = kKenneyScale * p.scale;
        DrawModelEx(*p.model, p.position, {0.0f, 1.0f, 0.0f}, p.yaw, {s, s, s}, WHITE);
    }
}

Vector3 World::resolveCircle(Vector3 position, float radius) const
{
    for (int pass = 0; pass < kResolvePasses; ++pass) {
        for (const Blocker& blocker : blockers_) {
            if (!blocker.active) continue;
            const BoundingBox& b = blocker.box;
            if (position.x + radius <= b.min.x || position.x - radius >= b.max.x ||
                position.z + radius <= b.min.z || position.z - radius >= b.max.z) {
                continue;
            }
            const float dx = position.x - std::clamp(position.x, b.min.x, b.max.x);
            const float dz = position.z - std::clamp(position.z, b.min.z, b.max.z);
            const float distSq = dx * dx + dz * dz;
            if (distSq >= radius * radius) continue;
            if (distSq > 1e-8f) {
                // Poussée selon la normale au point le plus proche : le cercle glisse le long de la boîte.
                const float dist = std::sqrt(distSq);
                const float push = (radius - dist) / dist;
                position.x += dx * push;
                position.z += dz * push;
                continue;
            }
            // Centre dans la boîte : sortie par le côté le plus proche.
            const float left = position.x - b.min.x;
            const float right = b.max.x - position.x;
            const float back = position.z - b.min.z;
            const float front = b.max.z - position.z;
            const float nearest = std::min({left, right, back, front});
            if (nearest == left) position.x = b.min.x - radius;
            else if (nearest == right) position.x = b.max.x + radius;
            else if (nearest == back) position.z = b.min.z - radius;
            else position.z = b.max.z + radius;
        }
    }
    return position;
}

bool World::segmentBlocked(Vector3 a, Vector3 b, Vector3& hit) const
{
    float best = 2.0f;
    for (const Blocker& blocker : blockers_) {
        float t = 0.0f;
        if (blocker.active && segmentHitsBox(a, b, blocker.box, t) && t < best) best = t;
    }
    if (best > 1.0f) return false;
    hit = Vector3Lerp(a, b, best);
    return true;
}
