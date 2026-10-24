#pragma once

#include "core/AnimatedModel.hpp"
#include "core/Audio.hpp"
#include "fx/Lighting.hpp"
#include "fx/Particles.hpp"
#include "fx/PostFx.hpp"
#include "game/CameraRig.hpp"
#include "game/CardEffects.hpp"
#include "game/Altar.hpp"
#include "game/BuildMode.hpp"
#include "game/Cards.hpp"
#include "game/Crates.hpp"
#include "game/Economy.hpp"
#include "game/Events.hpp"
#include "game/FlowField.hpp"
#include "game/Flashlight.hpp"
#include "game/FlyingLanterns.hpp"
#include "game/Merchant.hpp"
#include "game/MusicDirector.hpp"
#include "game/MysteryBox.hpp"
#include "game/Shop.hpp"
#include "game/Structures.hpp"
#include "game/Traps.hpp"
#include "game/Turrets.hpp"
#include "game/Pickups.hpp"
#include "game/Player.hpp"
#include "game/Progression.hpp"
#include "game/Projectile.hpp"
#include "game/WaveManager.hpp"
#include "game/Weapon.hpp"
#include "game/World.hpp"
#include "game/Zones.hpp"
#include "ui/BuildBar.hpp"
#include "ui/CardPicker.hpp"
#include "ui/CreditsScreen.hpp"
#include "ui/Hud.hpp"
#include "ui/MainMenu.hpp"
#include "ui/OptionsScreen.hpp"
#include "ui/ShopScreen.hpp"
#include "ui/TalentScreen.hpp"

#include <memory>
#include <string>
#include <vector>

// Machine à états du jeu : possède la fenêtre et tous les systèmes, et consomme
// les événements de gameplay (son, caméra, HUD).
class Game {
public:
    enum class State { Menu, Talents, Options, Credits, Playing, Paused, LevelUp, Shop, GameOver, Victory };

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
        Vector3 cursor = {0.0f, 0.0f, 0.0f}; // point du sol sous la souris (mode construction)
        bool sprint = false;
        bool fireHeld = false;
        bool firePressed = false;
        bool reload = false;
        int selectWeapon = -1;
        int cycleWeapon = 0;
        bool interact = false;
        bool light = false;   // lampe torche (clic droit maintenu)
    };

    void loadAudio();
    void startNewGame();
    // Change le modèle du joueur si le personnage choisi au menu a changé.
    void loadCharacter(CharacterId id);
    void update(float dt);
    void updatePlaying(float dt);
    Controls readControls() const;
    // Pilote automatique du mode --smoke : vise l'ennemi le plus proche et tire.
    Controls autopilot() const;
    void resolveProjectiles();
    // Point d'entrée unique des dégâts infligés aux ennemis (balles, feu, explosions) :
    // une mort passe toujours par rewardKill. Renvoie vrai si l'ennemi meurt.
    // byDefense : coup d'une tourelle (la mort ne fait pas monter le combo, XP ×0,5).
    bool damageEnemy(Enemy& enemy, float damage, Vector3 direction, float knockback, HitKind kind = HitKind::Normal,
                     bool byDefense = false);
    // Fait progresser le combo, lâche l'or et l'XP d'un ennemi qui vient de mourir
    // et déclenche les cartes liées aux morts (crochet onKill).
    void rewardKill(const Enemy& enemy, bool byDefense = false);
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
    // Arme prise à la boîte mystère : débloquée, dorée si légendaire, et en main.
    void takeMysteryPrize(const MysteryBox::Prize& prize);
    // Contenu d'une caisse cassée : bois, ferraille, munitions, bidon, ou explosion du tonneau de poudre.
    void breakCrate(const Crates::Broken& crate);
    // Explosion au sol : dégâts et recul aux morts du rayon, au joueur s'il est dedans (playerDamage > 0).
    // byDefense : explosion d'un piège (mort comptée « par une défense »).
    void explode(Vector3 center, float radius, float damage, float knockback, float playerDamage, Color color,
                 bool byDefense = false);
    // Base (GAME.md §7) : mode construction et structures. Vrai si le joueur construit (arme rangée).
    bool updateBase(float dt, const Controls& c);
    void drawBase() const;
    void updateShop(float dt);
    void updateTalents(float dt);
    // Options (volumes, plein écran) et crédits, ouverts depuis le menu.
    void updateOptions(float dt);
    void applySettings();
    // Fin de partie : ossements gagnés, records et sauvegarde, puis écran de fin.
    void endRun(bool victory);
    void handleEvents();
    void updateFootsteps(float dt);
    // Lampe torche, lumière d'appoint du joueur et envoi des lumières au shader.
    void updateLighting(float dt);

    void draw() const;
    void drawWorld() const;
    // Yeux des morts, qui luisent dans le noir.
    void drawEyes() const;
    void drawPauseOverlay() const;
    void drawEndScreen(const char* title, Color color) const;
    HudData hudData() const;
    // Point visé : le torse du mort sous le curseur, sinon le curseur projeté à hauteur du canon.
    Vector3 mouseAimPoint() const;
    // Curseur projeté sur le plan horizontal à la hauteur donnée.
    Vector3 mouseOnPlane(float height) const;

    Options options_;
    int frameCount_ = 0;
    State state_ = State::Menu;

    Audio audio_;
    MusicDirector musicDirector_;
    // Les ressources GPU doivent être créées après InitWindow et libérées avant CloseWindow.
    std::unique_ptr<World> world_;
    std::unique_ptr<Zones> zones_; // référence world_ : détruit avant lui
    std::unique_ptr<AnimatedModel> playerModel_; // modèle du personnage joué
    CharacterId character_ = CharacterId::Keeper;
    std::unique_ptr<AnimatedModel> merchantModel_;
    std::unique_ptr<Player> player_;
    std::unique_ptr<Merchant> merchant_; // nul si la carte n'en déclare pas
    std::unique_ptr<MysteryBox> mysteryBox_; // nulle si la carte n'en déclare pas ; référence world_
    std::unique_ptr<Altar> altar_;           // nul si la carte n'en déclare pas
    std::unique_ptr<Crates> crates_;         // référence world_ : détruit avant lui
    std::unique_ptr<Structures> structures_; // référence world_ : détruit avant lui
    BuildMode buildMode_;
    BuildBar buildBar_;
    Traps traps_;
    std::unique_ptr<Turrets> turrets_; // modèles partagés avec world_
    FlowField flowField_;        // navigation des morts, recalculée toutes les kFlowFieldInterval
    float flowFieldTimer_ = 0.0f;
    std::unique_ptr<Shop> shop_;
    std::unique_ptr<WaveManager> waves_;
    std::vector<Model> weaponModels_;
    Model lanternModel_{};  // carte Lanterne volante
    std::unique_ptr<Particles> particles_;
    std::unique_ptr<PostFx> postFx_;
    std::unique_ptr<Lighting> lighting_;
    Arsenal arsenal_;
    ProjectileSystem projectiles_;
    Economy economy_;
    Pickups pickups_;
    Cards cards_;
    CardEffects cardEffects_;
    Flashlight flashlight_;
    FlyingLanterns flyingLanterns_;
    CameraRig cameraRig_;
    Hud hud_;
    CardPicker cardPicker_;
    ShopScreen shopScreen_;
    float shopFrozenLeft_ = 0.0f;  // temps de pause figée restant chez le marchand
    std::unique_ptr<Progression> progression_;
    TalentScreen talentScreen_;
    OptionsScreen optionsScreen_;
    CreditsScreen creditsScreen_;
    MainMenu mainMenu_;
    GameMode mode_ = GameMode::Night;
    bool quitRequested_ = false;     // « Quitter » dans le menu
    int bonesEarned_ = 0;          // ossements gagnés par la partie qui vient de finir
    EventQueue events_;

    int score_ = 0;
    int kills_ = 0;
    int defenseKills_ = 0; // morts dues aux structures (écran de fin, GAME.md §7.5)
    float hitStop_ = 0.0f;
    float footstepTimer_ = 0.0f;
    float levelUpSlowMo_ = 0.0f;   // ralenti avant l'écran des cartes
    float pocketSunTimer_ = 0.0f;
    bool lastStandReady_ = true;   // Dernier rempart : une fois par vague
    std::vector<Vector3> pendingExplosions_;
};
