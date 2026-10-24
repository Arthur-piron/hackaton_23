#pragma once

#include "core/AnimatedModel.hpp"
#include "core/Audio.hpp"
#include "fx/Lighting.hpp"
#include "fx/Particles.hpp"
#include "fx/PostFx.hpp"
#include "game/CameraRig.hpp"
#include "game/CardEffects.hpp"
#include "game/Cards.hpp"
#include "game/Economy.hpp"
#include "game/Events.hpp"
#include "game/Merchant.hpp"
#include "game/Shop.hpp"
#include "game/Pickups.hpp"
#include "game/Player.hpp"
#include "game/Projectile.hpp"
#include "game/WaveManager.hpp"
#include "game/Weapon.hpp"
#include "game/World.hpp"
#include "game/Zones.hpp"
#include "ui/CardPicker.hpp"
#include "ui/Hud.hpp"
#include "ui/ShopScreen.hpp"

#include <memory>
#include <string>
#include <vector>

// Machine à états du jeu : possède la fenêtre et tous les systèmes, et consomme
// les événements de gameplay (son, caméra, HUD).
class Game {
public:
    enum class State { Menu, Playing, Paused, LevelUp, Shop, GameOver, Victory };

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
        bool interact = false;
    };

    void loadAudio();
    void startNewGame();
    void update(float dt);
    void updatePlaying(float dt);
    Controls readControls() const;
    // Pilote automatique du mode --smoke : vise l'ennemi le plus proche et tire.
    Controls autopilot() const;
    void resolveProjectiles();
    // Point d'entrée unique des dégâts infligés aux ennemis (balles, feu, explosions) :
    // une mort passe toujours par rewardKill. Renvoie vrai si l'ennemi meurt.
    bool damageEnemy(Enemy& enemy, float damage, Vector3 direction, float knockback, HitKind kind = HitKind::Normal);
    // Fait progresser le combo, lâche l'or et l'XP d'un ennemi qui vient de mourir
    // et déclenche les cartes liées aux morts (crochet onKill).
    void rewardKill(const Enemy& enemy);
    // Dégâts reçus par le joueur (crochet onDamaged : Dernier rempart).
    void hurtPlayer(float damage, Vector3 from);
    // Brûlures, explosions nécrotiques en attente, Soleil de poche.
    void updateCardEffects(float dt);
    // Enflamme les 3 ennemis les plus proches de source (Chaîne de feu).
    void spreadFire(const Enemy& source);
    // Ennemi vivant le plus proche de from, à moins de maxDistance (nul sinon).
    Enemy* nearestEnemy(Vector3 from, float maxDistance);
    // Recalcule les bonus des cartes et les transmet aux systèmes.
    void applyCardModifiers();
    // Tire 3 cartes et passe à l'écran de choix (reste en jeu si plus aucune carte n'est disponible).
    void openCardPicker();
    void updateLevelUp(float dt);
    // Marchand : ouvert entre les vagues, il fige la pause (au plus kShopMaxFrozen secondes).
    void openShop();
    void updateShop(float dt);
    void handleEvents();
    void updateFootsteps(float dt);
    // Lampe torche, lumière d'appoint du joueur et envoi des lumières au shader.
    void updateLighting(float dt);

    void draw() const;
    void drawWorld() const;
    // Yeux des morts, qui luisent dans le noir.
    void drawEyes() const;
    void drawMenu() const;
    void drawPauseOverlay() const;
    void drawEndScreen(const char* title, Color color) const;
    HudData hudData() const;
    // Point du sol (y = 0) sous le curseur de la souris.
    Vector3 mouseGroundPoint() const;

    Options options_;
    int frameCount_ = 0;
    State state_ = State::Menu;

    Audio audio_;
    // Les ressources GPU doivent être créées après InitWindow et libérées avant CloseWindow.
    std::unique_ptr<World> world_;
    std::unique_ptr<Zones> zones_; // référence world_ : détruit avant lui
    std::unique_ptr<AnimatedModel> keeperModel_;
    std::unique_ptr<AnimatedModel> merchantModel_;
    std::unique_ptr<Player> player_;
    std::unique_ptr<Merchant> merchant_; // nul si la carte n'en déclare pas
    std::unique_ptr<Shop> shop_;
    std::unique_ptr<WaveManager> waves_;
    std::vector<Model> weaponModels_;
    std::unique_ptr<Particles> particles_;
    std::unique_ptr<PostFx> postFx_;
    std::unique_ptr<Lighting> lighting_;
    Arsenal arsenal_;
    ProjectileSystem projectiles_;
    Economy economy_;
    Pickups pickups_;
    Cards cards_;
    CardEffects cardEffects_;
    CameraRig cameraRig_;
    Hud hud_;
    CardPicker cardPicker_;
    ShopScreen shopScreen_;
    float shopFrozenLeft_ = 0.0f;  // temps de pause figée restant chez le marchand
    EventQueue events_;

    int score_ = 0;
    int kills_ = 0;
    float hitStop_ = 0.0f;
    float footstepTimer_ = 0.0f;
    float levelUpSlowMo_ = 0.0f;   // ralenti avant l'écran des cartes
    float pocketSunTimer_ = 0.0f;
    bool lastStandReady_ = true;   // Dernier rempart : une fois par vague
    std::vector<Vector3> pendingExplosions_;
};
