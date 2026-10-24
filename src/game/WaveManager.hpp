#pragma once

#include "core/AnimatedModel.hpp"
#include "game/Enemy.hpp"
#include "game/Events.hpp"

#include <array>
#include <memory>
#include <vector>

class World;
class Zones;

// Enchaîne les vagues : apparition des morts aux entrées des zones ouvertes,
// difficulté croissante, pause entre deux vagues.
class WaveManager {
public:
    enum class Phase { Intermission, Fighting };
    static constexpr int kFinalWave = 10;

    WaveManager();
    ~WaveManager();
    WaveManager(const WaveManager&) = delete;
    WaveManager& operator=(const WaveManager&) = delete;

    void reset();
    // Renvoie les dégâts infligés au joueur pendant cette image.
    float update(float dt, Vector3 playerPosition, const World& world, const Zones& zones, EventQueue& events);
    void draw() const;
    // Fait passer les modèles des morts par ce shader (éclairage de nuit).
    void setShader(const Shader& shader);
    // Passe immédiatement à la vague suivante pendant la pause.
    void skipIntermission();
    // Fait apparaître un cercle d'ennemis autour d'un point (tests automatiques).
    void spawnRing(Vector3 center, float radius, int count, EnemyType type);

    std::vector<Enemy>& enemies() { return enemies_; }
    const std::vector<Enemy>& enemies() const { return enemies_; }
    int wave() const { return wave_; }
    Phase phase() const { return phase_; }
    float intermissionLeft() const { return phase_ == Phase::Intermission ? timer_ : 0.0f; }
    int remaining() const;
    bool allWavesCleared() const { return cleared_; }

private:
    void startWave(EventQueue& events);
    EnemyType pickType() const;
    Vector3 pickSpawnPoint(Vector3 playerPosition, const World& world, const Zones& zones) const;
    void separate(float dt);
    // Collisions avec le décor, et contournement quand un mort bute de face sur un obstacle.
    void collide(float dt, Vector3 playerPosition, const World& world, const Zones& zones);

    std::array<std::shared_ptr<AnimatedModel>, static_cast<size_t>(EnemyType::Count)> models_;
    std::vector<Enemy> enemies_;
    Phase phase_ = Phase::Intermission;
    int wave_ = 0;
    float timer_ = 0.0f;
    int toSpawn_ = 0;
    float spawnTimer_ = 0.0f;
    bool cleared_ = false;
    unsigned nextId_ = 1;
};
