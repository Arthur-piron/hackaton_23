#include "game/WaveManager.hpp"

#include "core/AssetPath.hpp"
#include "core/Fonts.hpp"
#include "game/FlowField.hpp"
#include "game/World.hpp"
#include "game/Zones.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>

namespace {
constexpr float kFirstIntermission = 4.0f;
constexpr float kIntermission = 15.0f;
// Distance minimale souhaitée entre un point d'apparition et le joueur.
constexpr float kMinSpawnDistance = 14.0f;
constexpr float kSpawnJitter = 0.8f;
constexpr float kSpawnClearance = 0.6f;
constexpr float kSeparationStrength = 4.0f;
// Contournement : un mort qui bute de face (cosinus) longe l'obstacle à cette fraction de sa vitesse.
constexpr float kHeadOnThreshold = 0.5f;
constexpr float kDetourSpeed = 0.8f;
// Cône de la lampe torche, comme dans fx/Lighting.cpp : les vampires le fuient.
constexpr float kFlashlightRange = 14.0f;
constexpr float kFlashlightHalfAngle = 27.0f * DEG2RAD;
// Les vampires évitent le cœur des lumières du décor (fraction de leur portée).
constexpr float kLightAvoidFraction = 0.7f;
// Élites : anneau pulsant au sol, lueur colorée et nom au-dessus de la tête.
constexpr float kEliteRingRadius = 0.75f;
constexpr float kEliteRingHeight = 0.03f;
constexpr float kElitePulseSpeed = 4.0f;
constexpr float kEliteLightHeight = 1.2f;
constexpr float kEliteLightRange = 3.5f;
constexpr float kEliteLightIntensity = 1.4f;
constexpr float kEliteNameHeight = 2.3f;     // au-dessus des pieds, × l'échelle du mort
constexpr float kEliteNameDistance = 14.0f;  // au-delà, le nom n'est pas affiché
constexpr int kEliteNameSize = 16;

// Probabilité d'apparition (en %) d'un type : base + parVague × vague, à partir de premièreVague.
// Le reste des tirages donne des marcheurs.
struct SpawnChance {
    EnemyType type;
    int firstWave;
    int base;
    int perWave;
};
constexpr SpawnChance kSpawnChances[] = {
    {EnemyType::Brute, 4, 4, 1},
    {EnemyType::Ghost, 5, -3, 1},
    {EnemyType::Vampire, 4, -1, 1},
    {EnemyType::Runner, 2, 16, 2},
    {EnemyType::Skeleton, 3, 20, 0},
};

int waveQuota(int wave)
{
    return 6 + wave * 5;
}

int maxAlive(int wave)
{
    return std::min(20 + wave * 6, 110);
}

float spawnInterval(int wave)
{
    return std::max(0.18f, 0.9f - wave * 0.07f);
}

int roll(int max)
{
    return GetRandomValue(1, max);
}
}

WaveManager::WaveManager()
{
    // Plusieurs types partagent le même fichier : chaque fichier n'est chargé qu'une fois.
    std::unordered_map<std::string, std::shared_ptr<AnimatedModel>> loaded;
    for (size_t i = 0; i < models_.size(); ++i) {
        const std::string file = enemyStats(static_cast<EnemyType>(i)).model;
        std::shared_ptr<AnimatedModel>& model = loaded[file];
        if (!model) model = std::make_shared<AnimatedModel>(assetPath("models/graveyard/" + file));
        models_[i] = model;
    }
    reset();
}

WaveManager::~WaveManager() = default;

void WaveManager::setShader(const Shader& shader)
{
    for (const std::shared_ptr<AnimatedModel>& model : models_) {
        Model& m = model->model();
        for (int i = 0; i < m.materialCount; ++i) m.materials[i].shader = shader;
    }
}

void WaveManager::reset()
{
    enemies_.clear();
    phase_ = Phase::Intermission;
    wave_ = 0;
    event_ = WaveEvent::None;
    lastEvent_ = WaveEvent::None;
    timer_ = kFirstIntermission;
    toSpawn_ = 0;
    spawnTimer_ = 0.0f;
    cleared_ = false;
}

int WaveManager::remaining() const
{
    int alive = 0;
    for (const Enemy& e : enemies_) alive += e.isAlive() ? 1 : 0;
    return alive + toSpawn_;
}

void WaveManager::skipIntermission()
{
    if (phase_ == Phase::Intermission && !cleared_) timer_ = std::min(timer_, 0.01f);
}

void WaveManager::spawnRing(Vector3 center, float radius, int count, EnemyType type)
{
    for (int i = 0; i < count; ++i) {
        const float angle = 2.0f * PI * static_cast<float>(i) / static_cast<float>(count);
        const Vector3 p = {center.x + std::cos(angle) * radius, 0.0f, center.z + std::sin(angle) * radius};
        enemies_.emplace_back(nextId_++, type, *models_[static_cast<size_t>(type)], p);
    }
}

void WaveManager::setRules(bool endless, float healthMultiplier, float eliteChance)
{
    endless_ = endless;
    healthMultiplier_ = healthMultiplier;
    eliteChance_ = eliteChance;
}

void WaveManager::startWave(EventQueue& events)
{
    ++wave_;
    phase_ = Phase::Fighting;
    event_ = rollWaveEvent(wave_, lastEvent_);
    if (event_ != WaveEvent::None) lastEvent_ = event_;
    toSpawn_ = waveQuota(wave_);
    if (event_ == WaveEvent::Horde) toSpawn_ *= waveEvents::kHordeQuotaMultiplier;
    spawnTimer_ = 0.0f;
    events.push_back({EventType::WaveStarted, {0.0f, 0.0f, 0.0f}, static_cast<float>(wave_)});
}

EnemyType WaveManager::pickType() const
{
    const int r = roll(100);
    int threshold = 0;
    for (const SpawnChance& chance : kSpawnChances) {
        if (wave_ < chance.firstWave) continue;
        threshold += std::max(0, chance.base + chance.perWave * wave_);
        if (r <= threshold) return chance.type;
    }
    return EnemyType::Walker;
}

void WaveManager::setFlashlight(bool on, Vector3 origin, Vector3 direction)
{
    flashlightOn_ = on;
    flashlightOrigin_ = origin;
    flashlightDirection_ = Vector3Normalize({direction.x, 0.0f, direction.z});
}

bool WaveManager::inFlashlight(Vector3 position) const
{
    if (!flashlightOn_) return false;
    const Vector3 toEnemy = {position.x - flashlightOrigin_.x, 0.0f, position.z - flashlightOrigin_.z};
    const float distance = Vector3Length(toEnemy);
    if (distance > kFlashlightRange) return false;
    if (distance < 1e-3f) return true;
    return Vector3DotProduct(toEnemy, flashlightDirection_) / distance >= std::cos(kFlashlightHalfAngle);
}

Vector3 WaveManager::lightEscape(Vector3 position) const
{
    Vector3 escape = {0.0f, 0.0f, 0.0f};
    // Hors du cône : de côté, en s'éloignant de l'axe de la lampe.
    if (inFlashlight(position)) {
        const Vector3 toEnemy = {position.x - flashlightOrigin_.x, 0.0f, position.z - flashlightOrigin_.z};
        const float along = Vector3DotProduct(toEnemy, flashlightDirection_);
        Vector3 aside = Vector3Subtract(toEnemy, Vector3Scale(flashlightDirection_, along));
        if (Vector3Length(aside) < 1e-3f) aside = {-flashlightDirection_.z, 0.0f, flashlightDirection_.x};
        escape = Vector3Add(Vector3Normalize(aside), Vector3Scale(flashlightDirection_, 0.3f));
    }
    // Hors des flaques de lumière du décor, d'autant plus fort qu'il est près de la source.
    // Pendant une coupure, les lampadaires sont éteints : plus rien à fuir.
    if (event_ == WaveEvent::Blackout) return escape;
    for (const PointLight& light : lights_) {
        const float radius = light.range * kLightAvoidFraction;
        const Vector3 away = {position.x - light.position.x, 0.0f, position.z - light.position.z};
        const float distance = Vector3Length(away);
        if (distance >= radius || distance < 1e-3f) continue;
        escape = Vector3Add(escape, Vector3Scale(away, (1.0f - distance / radius) / distance));
    }
    return escape;
}

Vector3 WaveManager::route(Vector3 from, Vector3 playerPosition, const Zones& zones) const
{
    return flowField_ ? flowField_->waypoint(from, playerPosition) : zones.steer(from, playerPosition);
}

EnemySenses WaveManager::sense(const Enemy& enemy, Vector3 playerPosition, Vector3 playerForward,
                               const Zones& zones) const
{
    EnemySenses senses;
    // Les spectres traversent les grilles : ils vont droit sur le joueur.
    senses.goal = enemy.stats().ethereal ? playerPosition : route(enemy.position(), playerPosition, zones);
    senses.player = playerPosition;
    senses.playerForward = playerForward;
    senses.inFlashlight = false;
    senses.lightEscape = {0.0f, 0.0f, 0.0f};
    senses.haste = waveEventInfo(event_).haste;
    if (enemy.stats().fearsLight && enemy.isAlive()) {
        senses.inFlashlight = inFlashlight(enemy.position());
        senses.lightEscape = lightEscape(enemy.position());
    }
    return senses;
}

void WaveManager::spawnEnemy(Vector3 playerPosition, const World& world, const Zones& zones)
{
    // Horde : surtout des marcheurs affaiblis.
    const bool hordeWalker = event_ == WaveEvent::Horde && roll(100) <= waveEvents::kHordeWalkerPercent;
    const EnemyType type = hordeWalker ? EnemyType::Walker : pickType();
    enemies_.emplace_back(nextId_++, type, *models_[static_cast<size_t>(type)],
                          pickSpawnPoint(playerPosition, world, zones));
    Enemy& enemy = enemies_.back();
    enemy.scaleHealth(hordeWalker ? healthMultiplier_ * waveEvents::kHordeWalkerHealth : healthMultiplier_);
    enemy.makeElite(rollElite(wave_, eliteChance_));
}

Vector3 WaveManager::pickSpawnPoint(Vector3 playerPosition, const World& world, const Zones& zones) const
{
    // Au hasard parmi les entrées assez loin du joueur ; à défaut, la plus éloignée.
    const std::vector<Vector3> points = zones.openSpawnPoints();
    if (points.empty()) return world.playerSpawn();
    std::vector<Vector3> far;
    Vector3 farthest = points.front();
    for (const Vector3& p : points) {
        if (Vector3Distance(p, playerPosition) >= kMinSpawnDistance) far.push_back(p);
        if (Vector3Distance(p, playerPosition) > Vector3Distance(farthest, playerPosition)) farthest = p;
    }
    Vector3 spawn = far.empty() ? farthest : far[static_cast<size_t>(GetRandomValue(0, static_cast<int>(far.size()) - 1))];
    // Léger décalage : les morts d'une même entrée ne sortent pas tous du même point.
    spawn.x += static_cast<float>(GetRandomValue(-100, 100)) / 100.0f * kSpawnJitter;
    spawn.z += static_cast<float>(GetRandomValue(-100, 100)) / 100.0f * kSpawnJitter;
    return world.resolveCircle(spawn, kSpawnClearance);
}

void WaveManager::collide(float dt, Vector3 playerPosition, const World& world, const Zones& zones)
{
    for (Enemy& e : enemies_) {
        // Les spectres traversent le décor.
        if (!e.isAlive() || e.stats().ethereal) continue;
        const Vector3 position = e.position();
        Vector3 resolved = world.resolveCircle(position, e.radius());
        const Vector3 correction = Vector3Subtract(resolved, position);
        const float pushed = Vector3Length(correction);
        if (pushed > 1e-4f) {
            // Le décor l'a repoussé : s'il fonce droit dans l'obstacle, il le longe vers sa cible.
            const Vector3 normal = Vector3Scale(correction, 1.0f / pushed);
            const Vector3 goal = route(position, playerPosition, zones);
            const Vector3 toGoal = Vector3Normalize({goal.x - position.x, 0.0f, goal.z - position.z});
            if (Vector3DotProduct(normal, toGoal) < -kHeadOnThreshold) {
                Vector3 tangent = {-normal.z, 0.0f, normal.x};
                const float side = Vector3DotProduct(tangent, toGoal);
                if (side < 0.0f || (std::fabs(side) < 1e-3f && (e.id() % 2) == 0)) tangent = Vector3Negate(tangent);
                const float step = e.stats().speed * kDetourSpeed * dt;
                resolved = world.resolveCircle(Vector3Add(resolved, Vector3Scale(tangent, step)), e.radius());
            }
        }
        e.push(Vector3Subtract(resolved, position));
    }
}

void WaveManager::separate(float dt)
{
    // Les morts se repoussent entre eux pour ne pas se superposer.
    for (size_t i = 0; i < enemies_.size(); ++i) {
        if (!enemies_[i].isAlive()) continue;
        for (size_t j = i + 1; j < enemies_.size(); ++j) {
            if (!enemies_[j].isAlive()) continue;
            const Vector3 a = enemies_[i].position();
            const Vector3 b = enemies_[j].position();
            const float minDistance = enemies_[i].radius() + enemies_[j].radius();
            const float dx = b.x - a.x;
            const float dz = b.z - a.z;
            const float distSq = dx * dx + dz * dz;
            if (distSq >= minDistance * minDistance || distSq < 1e-6f) continue;
            const float dist = std::sqrt(distSq);
            const float overlap = (minDistance - dist) * std::min(1.0f, kSeparationStrength * dt * 10.0f) * 0.5f;
            const Vector3 n = {dx / dist, 0.0f, dz / dist};
            enemies_[i].push(Vector3Scale(n, -overlap));
            enemies_[j].push(Vector3Scale(n, overlap));
        }
    }
}

float WaveManager::update(float dt, Vector3 playerPosition, Vector3 playerForward, const World& world,
                          const Zones& zones, EventQueue& events)
{
    time_ += dt;
    if (phase_ == Phase::Intermission && !cleared_) {
        timer_ -= dt;
        if (timer_ <= 0.0f) startWave(events);
    }

    if (phase_ == Phase::Fighting && toSpawn_ > 0) {
        spawnTimer_ -= dt;
        int alive = 0;
        for (const Enemy& e : enemies_) alive += e.isAlive() ? 1 : 0;
        if (spawnTimer_ <= 0.0f && alive < maxAlive(wave_)) {
            spawnTimer_ = spawnInterval(wave_);
            if (event_ == WaveEvent::Horde) spawnTimer_ *= waveEvents::kHordeSpawnInterval;
            spawnEnemy(playerPosition, world, zones);
            --toSpawn_;
        }
    }

    float damage = 0.0f;
    // Chaque mort marche vers le joueur, ou vers la grille qui mène à sa zone.
    for (Enemy& e : enemies_) damage += e.update(dt, sense(e, playerPosition, playerForward, zones), events);
    separate(dt);
    collide(dt, playerPosition, world, zones);
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [](const Enemy& e) { return e.isRemovable(); }),
                   enemies_.end());

    if (phase_ == Phase::Fighting && remaining() == 0) {
        events.push_back({EventType::WaveCleared, playerPosition, static_cast<float>(wave_)});
        phase_ = Phase::Intermission;
        event_ = WaveEvent::None;
        timer_ = kIntermission;
        if (!endless_ && wave_ >= kFinalWave) cleared_ = true;
    }
    return damage;
}

void WaveManager::draw() const
{
    // Les spectres, semi-transparents, passent après les morts opaques.
    for (const Enemy& e : enemies_) {
        if (!e.stats().ethereal) e.draw();
    }
    const float pulse = 0.5f + 0.5f * std::sin(time_ * kElitePulseSpeed);
    for (const Enemy& e : enemies_) {
        if (!e.isElite() || !e.isAlive()) continue;
        const Color color = eliteInfo(e.elite()).color;
        const Vector3 ground = {e.position().x, kEliteRingHeight, e.position().z};
        const float radius = kEliteRingRadius * e.stats().scale;
        DrawCylinder(ground, radius, radius, 0.0f, 24, ColorAlpha(color, 0.12f + 0.12f * pulse));
        DrawCircle3D(ground, radius * (1.0f + 0.1f * pulse), {1.0f, 0.0f, 0.0f}, 90.0f, ColorAlpha(color, 0.9f));
    }
    for (const Enemy& e : enemies_) {
        if (e.stats().ethereal) e.draw();
    }
}

std::vector<PointLight> WaveManager::eliteLights() const
{
    std::vector<PointLight> lights;
    for (const Enemy& e : enemies_) {
        if (!e.isElite() || !e.isAlive()) continue;
        PointLight light;
        light.position = Vector3Add(e.position(), {0.0f, kEliteLightHeight, 0.0f});
        light.color = eliteInfo(e.elite()).color;
        light.range = kEliteLightRange * e.stats().scale;
        light.intensity = kEliteLightIntensity;
        lights.push_back(light);
    }
    return lights;
}

void WaveManager::drawEliteNames(const Camera3D& camera, Vector3 playerPosition) const
{
    for (const Enemy& e : enemies_) {
        if (!e.isElite() || !e.isAlive()) continue;
        if (Vector3Distance(e.position(), playerPosition) > kEliteNameDistance) continue;
        const Vector3 above = Vector3Add(e.position(), {0.0f, kEliteNameHeight * e.stats().scale, 0.0f});
        const Vector2 screen = GetWorldToScreen(above, camera);
        const std::string name = eliteName(e.type(), e.elite());
        const int x = static_cast<int>(screen.x) - fonts::measureText(name.c_str(), kEliteNameSize) / 2;
        const int y = static_cast<int>(screen.y);
        fonts::drawText(name.c_str(), x + 1, y + 1, kEliteNameSize, {0, 0, 0, 200});
        fonts::drawText(name.c_str(), x, y, kEliteNameSize, eliteInfo(e.elite()).color);
    }
}
