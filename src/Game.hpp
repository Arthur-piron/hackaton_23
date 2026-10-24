#pragma once

#include "core/AnimatedModel.hpp"
#include "core/Audio.hpp"
#include "fx/Particles.hpp"
#include "fx/PostFx.hpp"
#include "game/CameraRig.hpp"
#include "game/Economy.hpp"
#include "game/Events.hpp"
#include "game/Pickups.hpp"
#include "game/Player.hpp"
#include "game/Projectile.hpp"
#include "game/WaveManager.hpp"
#include "game/Weapon.hpp"
#include "game/World.hpp"
#include "ui/Hud.hpp"

#include <memory>
#include <string>
#include <vector>

// Machine à états du jeu : possède la fenêtre et tous les systèmes, et consomme
// les événements de gameplay (son, caméra, HUD).
class Game {
public:
    enum class State { Menu, Playing, Paused, GameOver, Victory };

    struct Options {
        int smokeFrames = 0;         // > 0 : mode test automatique
        std::string smokeScreenshot; // capture enregistrée à la fin du test
    };

    explicit Game(const Options& options);
    ~Game();
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    void run();

private:
    struct Controls {
        Vector3 move = {0.0f, 0.0f, 0.0f};
        Vector3 aim = {0.0f, 0.0f, 0.0f};
        bool sprint = false;
        bool fireHeld = false;
        bool firePressed = false;
        bool reload = false;
        int selectWeapon = -1;
        int cycleWeapon = 0;
    };

    void loadAudio();
    void startNewGame();
    void update(float dt);
    void updatePlaying(float dt);
    Controls readControls() const;
    // Pilote automatique du mode --smoke : vise l'ennemi le plus proche et tire.
    Controls autopilot() const;
    void resolveProjectiles();
    // Fait progresser le combo et lâche l'or et l'XP d'un ennemi qui vient de mourir.
    void rewardKill(const Enemy& enemy);
    // Balles arrêtées par les murs d'enceinte (étincelles).
    void resolveWallImpacts();
    void handleEvents();
    void updateFootsteps(float dt);

    void draw() const;
    void drawWorld() const;
    void drawMenu() const;
    void drawPauseOverlay() const;
    void drawEndScreen(const char* title, Color color) const;
    HudData hudData() const;
    // Point du sol (y = 0) sous le curseur de la souris.
    Vector3 mouseGroundPoint() const;

    Options options_;
    int frameCount_ = 0;
    State state_ = State::Menu;

    World world_;
    Audio audio_;
    // Les ressources GPU doivent être créées après InitWindow et libérées avant CloseWindow.
    std::unique_ptr<AnimatedModel> keeperModel_;
    std::unique_ptr<Player> player_;
    std::unique_ptr<WaveManager> waves_;
    std::vector<Model> weaponModels_;
    std::unique_ptr<Particles> particles_;
    std::unique_ptr<PostFx> postFx_;
    Arsenal arsenal_;
    ProjectileSystem projectiles_;
    Economy economy_;
    Pickups pickups_;
    CameraRig cameraRig_;
    Hud hud_;
    EventQueue events_;

    int score_ = 0;
    int kills_ = 0;
    float hitStop_ = 0.0f;
    float footstepTimer_ = 0.0f;
};
