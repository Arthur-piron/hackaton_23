#pragma once

#include "core/AnimatedModel.hpp"
#include "fx/PointLight.hpp"
#include "game/Enemy.hpp"
#include "game/Events.hpp"
#include "game/WaveEvent.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

class FlowField;
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
    float update(float dt, Vector3 playerPosition, Vector3 playerForward, const World& world, const Zones& zones,
                 EventQueue& events);
    void draw() const;
    // Noms des élites au-dessus de leur tête (2D, après la scène 3D).
    void drawEliteNames(const Camera3D& camera, Vector3 playerPosition) const;
    // Lueur colorée de chaque élite, à ajouter à l'éclairage à chaque image.
    std::vector<PointLight> eliteLights() const;
    // Fait passer les modèles des morts par ce shader (éclairage de nuit).
    void setShader(const Shader& shader);
    // Passe immédiatement à la vague suivante pendant la pause.
    void skipIntermission();
    // Lumières du décor que les vampires évitent.
    void setLights(std::vector<PointLight> lights) { lights_ = std::move(lights); }
    // Lampe torche du joueur, à rappeler à chaque image : les vampires fuient son cône.
    void setFlashlight(bool on, Vector3 origin, Vector3 direction);
    // Navigation par champ de distances (base v3) ; sans lui, les morts passent par les grilles (Zones::steer).
    void setFlowField(const FlowField* field) { flowField_ = field; }
    // Fait apparaître un cercle d'ennemis autour d'un point (tests automatiques).
    void spawnRing(Vector3 center, float radius, int count, EnemyType type);
    // Règles du mode de jeu : endless = pas de victoire à la vague 10 ; vie des morts et chance d'élite multipliées.
    void setRules(bool endless, float healthMultiplier, float eliteChance);

    std::vector<Enemy>& enemies() { return enemies_; }
    const std::vector<Enemy>& enemies() const { return enemies_; }
    int wave() const { return wave_; }
    // Événement de la vague en cours (None pendant les pauses).
    WaveEvent event() const { return event_; }
    int goldMultiplier() const { return waveEventInfo(event_).goldMultiplier; }
    Phase phase() const { return phase_; }
    float intermissionLeft() const { return phase_ == Phase::Intermission ? timer_ : 0.0f; }
    int remaining() const;
    bool allWavesCleared() const { return cleared_; }

private:
    void startWave(EventQueue& events);
    EnemyType pickType() const;
    void spawnEnemy(Vector3 playerPosition, const World& world, const Zones& zones);
    EnemySenses sense(const Enemy& enemy, Vector3 playerPosition, Vector3 playerForward, const Zones& zones) const;
    bool inFlashlight(Vector3 position) const;
    Vector3 lightEscape(Vector3 position) const;
    Vector3 pickSpawnPoint(Vector3 playerPosition, const World& world, const Zones& zones) const;
    void separate(float dt);
    // Collisions avec le décor, et contournement quand un mort bute de face sur un obstacle.
    void collide(float dt, Vector3 playerPosition, const World& world, const Zones& zones);
    // Où marcher pour rejoindre le joueur : FlowField s'il y en a un, sinon Zones::steer.
    Vector3 route(Vector3 from, Vector3 playerPosition, const Zones& zones) const;

    std::array<std::shared_ptr<AnimatedModel>, static_cast<size_t>(EnemyType::Count)> models_;
    std::vector<Enemy> enemies_;
    std::vector<PointLight> lights_;
    bool flashlightOn_ = false;
    const FlowField* flowField_ = nullptr;
    Vector3 flashlightOrigin_ = {0.0f, 0.0f, 0.0f};
    Vector3 flashlightDirection_ = {0.0f, 0.0f, 1.0f};
    Phase phase_ = Phase::Intermission;
    int wave_ = 0;
    WaveEvent event_ = WaveEvent::None;
    WaveEvent lastEvent_ = WaveEvent::None;
    float timer_ = 0.0f;
    int toSpawn_ = 0;
    float spawnTimer_ = 0.0f;
    bool cleared_ = false;
    bool endless_ = false;
    float healthMultiplier_ = 1.0f;
    float eliteChance_ = 1.0f;
    float time_ = 0.0f;           // pulsation des halos d'élite
    unsigned nextId_ = 1;
};
