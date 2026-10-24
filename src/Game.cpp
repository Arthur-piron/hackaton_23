#include "Game.hpp"

#include "core/AssetPath.hpp"
#include "core/Fonts.hpp"
#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <cstddef>
#include <cmath>

namespace {
constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;
constexpr int kTargetFps = 144;
// Au-delà, on ralentit la simulation plutôt que de laisser traverser les murs.
constexpr float kMaxFrameTime = 0.05f;
constexpr float kSmokeFrameTime = 1.0f / 60.0f;
constexpr const char* kWindowTitle = "LAST LIGHT";

constexpr float kKillHitStop = 0.035f;
constexpr float kKillShake = 0.06f;
constexpr float kHurtShake = 0.45f;
constexpr float kDeathShake = 0.8f;
constexpr float kLookAhead = 0.25f;   // fraction de la distance de visée ajoutée au cadrage
constexpr float kMaxLookAhead = 3.0f;
constexpr float kAimAssistScale = 1.4f; // le survol d'un mort compte un peu au-delà de son corps
constexpr float kFootstepInterval = 0.36f;
constexpr float kSprintFootstepInterval = 0.26f;
constexpr int kSmokeWeapon = 2; // mitraillette
constexpr int kSmokeAmmo = 400;
// Taille du flash de bouche : proportionnelle à la secousse de l'arme.
constexpr float kMuzzleBase = 0.8f;
constexpr float kMuzzleShakeScale = 1.5f;
constexpr float kPlayerBloodHeight = 1.1f;
// En --smoke, une montée de niveau est forcée à cette distance de la fin : la capture montre les cartes.
constexpr int kSmokeShowcaseFrames = 80;

// Marchand : la pause entre vagues est figée tant que l'écran est ouvert, dans cette limite.
constexpr float kShopMaxFrozen = 45.0f;

// Cartes
constexpr float kLevelUpSlowMo = 0.4f;     // durée réelle du ralenti avant l'écran des cartes
constexpr float kSlowMoScale = 0.25f;
constexpr float kCritMultiplier = 3.0f;
constexpr float kReaperThreshold = 0.15f;  // fraction de la vie max
constexpr float kRicochetRange = 8.0f;
constexpr float kRicochetLifeMargin = 0.2f;
constexpr float kFireChainRadius = 5.0f;
constexpr int kFireChainTargets = 3;
constexpr float kNecroticRadius = 2.5f;
constexpr float kNecroticDamage = 40.0f;
constexpr float kNecroticKnockback = 5.0f;
constexpr float kPocketSunInterval = 20.0f;
constexpr float kPocketSunRadius = 8.0f;
constexpr float kPocketSunDamage = 60.0f;
constexpr float kLastStandRadius = 7.0f;
constexpr float kLastStandDamage = 10.0f;
constexpr float kLastStandKnockback = 16.0f;
constexpr int kFortuneComboMultiplier = 5;
constexpr int kFortuneCoins = 50;
constexpr int kFortuneGoldPerCoin = 5;
constexpr float kExplosionShake = 0.35f;
constexpr float kFrostSlow = 0.3f;
constexpr float kFrostDuration = 1.5f;

constexpr Color kBackgroundColor = {12, 14, 22, 255};
constexpr Color kDamageColor = {255, 245, 220, 255};
constexpr Color kCritColor = {255, 225, 40, 255};
constexpr Color kBurnColor = {255, 140, 50, 255};
constexpr Color kLightBurnColor = {180, 220, 255, 255};
constexpr Color kNecroticColor = {150, 255, 90, 200};
constexpr Color kSunColor = {255, 230, 150, 220};
constexpr Color kLastStandColor = {120, 200, 255, 220};
constexpr Color kPlayerDamageColor = {255, 80, 80, 255};
constexpr Color kWaveColor = {230, 200, 120, 255};
constexpr Color kClearColor = {140, 230, 150, 255};
constexpr Color kLevelColor = {90, 235, 200, 255};
constexpr Color kGateColor = {255, 215, 120, 255};
constexpr Color kBoneColor = {235, 225, 195, 255};
constexpr Color kRecordColor = {255, 205, 90, 255};
constexpr int kArmoryWeapon = 1; // talent Arsenal : fusil à pompe (indice dans Arsenal::catalog)
constexpr float kGateShake = 0.3f;
constexpr Color kAltarHealColor = {170, 225, 255, 255};
// Caisses (GAME.md §7.1) : 2 + nombre de zones ouvertes à chaque pause.
constexpr int kCratesPerPause = 2;
constexpr int kCrateChunks = 12;
constexpr Color kCrateWoodColor = {150, 105, 60, 255};
constexpr float kBlastCrateDamage = 60.0f;
constexpr float kPowderRadius = 3.0f;
constexpr float kPowderDamage = 60.0f;
constexpr float kPowderPlayerDamage = 20.0f;
constexpr Color kPowderColor = {255, 140, 50, 255};
// Base : navigation et dégâts des morts aux structures (GAME.md §7.4).
constexpr float kFlowFieldInterval = 0.25f;
constexpr float kStructureReach = 0.35f;   // au-delà de son rayon, un mort frappe la structure
constexpr float kStructureDestroyShake = 0.15f;
constexpr float kTurretKnockback = 2.0f;

// Éclairage de nuit : lampe torche tenue à hauteur de poitrine, légèrement
// inclinée vers le sol, et faible lumière d'appoint pour garder le joueur lisible.
constexpr float kFlashlightHeight = 1.25f;
constexpr float kFlashlightForward = 0.35f;
constexpr float kFlashlightTilt = 0.16f;
constexpr Color kPlayerFillColor = {150, 175, 230, 255};
constexpr float kPlayerFillHeight = 2.2f;
constexpr float kPlayerFillRange = 4.5f;
constexpr float kPlayerFillIntensity = 0.7f;
// Yeux des morts : un cœur vif dans un halo rouge orangé.
constexpr Color kEyeHaloColor = {255, 70, 20, 200};
constexpr Color kEyeCoreColor = {255, 200, 120, 255};
constexpr float kEyeHaloSize = 0.55f;
constexpr float kEyeCoreSize = 0.14f;
// Lumières brèves : flash de tir chaud, explosion nécrotique verdâtre.
constexpr Color kMuzzleLightColor = {255, 196, 120, 255};
constexpr float kMuzzleLightRange = 8.0f;
constexpr float kMuzzleLightIntensity = 2.6f;
constexpr float kMuzzleLightDuration = 0.07f;
constexpr Color kExplosionLightColor = {170, 255, 120, 255};
constexpr float kExplosionLightIntensity = 3.0f;
constexpr float kExplosionLightDuration = 0.35f;
constexpr float kExplosionLightHeight = 1.0f;

// Sons de tir par arme (indices du catalogue d'Arsenal).
const char* const kShotSounds[] = {"gun_pistol", "gun_shotgun", "gun_pistol", "gun_rifle"};

bool chance(float probability)
{
    return static_cast<float>(GetRandomValue(0, 9999)) < probability * 10000.0f;
}

Vector3 flatDirection(Vector3 from, Vector3 to)
{
    return Vector3Normalize({to.x - from.x, 0.0f, to.z - from.z});
}

float flatDistance(Vector3 a, Vector3 b)
{
    return Vector2Distance({a.x, a.z}, {b.x, b.z});
}

// raylib nomme les touches d'après leur position sur un clavier QWERTY :
// KEY_W/A/S/D correspondent physiquement à Z/Q/S/D sur un clavier AZERTY.
Vector3 readMoveKeys()
{
    Vector3 dir = {0.0f, 0.0f, 0.0f};
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) dir.z -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) dir.z += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) dir.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dir.x += 1.0f;
    return Vector3Length(dir) > 0.0f ? Vector3Normalize(dir) : dir;
}

std::string characterModelPath(CharacterId id)
{
    return assetPath(std::string("models/graveyard/") + characterInfo(id).model);
}
}

void Game::loadCharacter(CharacterId id)
{
    // Le joueur passe sur le nouveau modèle avant que l'ancien ne soit libéré.
    auto model = std::make_unique<AnimatedModel>(characterModelPath(id));
    lighting_->apply(model->model());
    player_->setModel(*model);
    playerModel_ = std::move(model);
    character_ = id;
}

Game::Game(const Options& options)
    : options_(options)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(kScreenWidth, kScreenHeight, kWindowTitle);
    fonts::load();
    InitAudioDevice();
    SetTargetFPS(kTargetFps);
    SetExitKey(KEY_NULL); // Échap sert à la pause, pas à quitter.
    HideCursor();         // remplacé par le réticule du HUD

    // En --smoke, la vraie sauvegarde n'est ni lue ni écrite.
    progression_ = std::make_unique<Progression>(SaveFile::defaultPath(), options_.smokeFrames == 0);
    const MapData map = loadMapFile(assetPath("maps/graveyard.txt"));
    world_ = std::make_unique<World>(map);
    zones_ = std::make_unique<Zones>(map, *world_);
    character_ = progression_->selectedCharacter();
    playerModel_ = std::make_unique<AnimatedModel>(characterModelPath(character_));
    player_ = std::make_unique<Player>(*playerModel_);
    waves_ = std::make_unique<WaveManager>();
    for (const WeaponStats& weapon : Arsenal::catalog()) {
        weaponModels_.push_back(LoadModel(assetPath("models/blaster/" + weapon.model).c_str()));
    }
    particles_ = std::make_unique<Particles>();
    postFx_ = std::make_unique<PostFx>();
    lighting_ = std::make_unique<Lighting>();
    lighting_->setStaticLights(world_->lights());
    waves_->setLights(world_->lights());
    world_->setShader(lighting_->shader());
    waves_->setShader(lighting_->shader());
    lighting_->apply(playerModel_->model());
    for (Model& model : weaponModels_) lighting_->apply(model);
    lanternModel_ = LoadModel(assetPath("models/town/lantern.glb").c_str());
    lighting_->apply(lanternModel_);
    shop_ = std::make_unique<Shop>(economy_, arsenal_, *player_);
    if (!map.merchants.empty()) {
        merchantModel_ = std::make_unique<AnimatedModel>(assetPath("models/graveyard/character-vampire.glb"));
        lighting_->apply(merchantModel_->model());
        merchant_ = std::make_unique<Merchant>(*merchantModel_, map.merchants.front());
    }
    crates_ = std::make_unique<Crates>(*world_);
    structures_ = std::make_unique<Structures>(*world_);
    waves_->setFlowField(&flowField_);
    turrets_ = std::make_unique<Turrets>(*world_);
    if (!map.altars.empty()) altar_ = std::make_unique<Altar>(map.altars.front());
    if (!map.mysteryBoxes.empty()) {
        mysteryBox_ = std::make_unique<MysteryBox>(map.mysteryBoxes, *world_);
        mysteryBox_->applyLighting(*lighting_);
    }
    loadAudio();

    startNewGame();
    if (options_.smokeFrames > 0) {
        // Test automatique : directement en combat dans la cour, face à une horde
        // (le pilote automatique la fauche).
        state_ = State::Playing;
        waves_->spawnRing(Vector3Add(player_->position(), {0.0f, 0.0f, -4.5f}), 1.8f, 10, EnemyType::Walker);
        waves_->spawnRing(Vector3Add(player_->position(), {-2.0f, 0.0f, 4.0f}), 1.5f, 6, EnemyType::Runner);
        // La mitraillette enchaîne les morts : la capture montre l'or au sol et un combo.
        arsenal_.unlock(kSmokeWeapon);
        arsenal_.addAmmo(kSmokeWeapon, kSmokeAmmo);
        arsenal_.select(kSmokeWeapon);
        // Quelques cartes actives : flammes, rebonds et critiques visibles sur la capture.
        for (CardId card : {CardId::Incendiary, CardId::Ricochet, CardId::CriticalHit, CardId::Piercing,
                            CardId::FlyingLantern}) {
            cards_.take(card);
        }
        applyCardModifiers();
        // La boîte mystère, posée dans la cour, tourne pendant la capture.
        if (mysteryBox_) {
            mysteryBox_->reset(mysteryBox_->nearestSpot(player_->position()));
            economy_.addGold(MysteryBox::kPrice);
            mysteryBox_->interact(mysteryBox_->position(), *zones_, economy_, events_);
        }
        crates_->spawnWave(Crates::kMaxCrates, *zones_, player_->position());
        // Une palissade, une barricade et une porte devant le joueur.
        for (int i = 0; i < static_cast<int>(StructureKind::Count); ++i) {
            structures_->place(static_cast<StructureKind>(i),
                               Vector3Add(player_->position(), {2.0f + 2.0f * static_cast<float>(i), 0.0f, 2.5f}), false);
        }
        // Les quatre pièges sur le chemin de la horde ; la flaque est déjà en feu.
        const Vector3 me = player_->position();
        structures_->place(StructureKind::Crossbow, Vector3Add(me, {-2.5f, 0.0f, 1.5f}), false);
        structures_->place(StructureKind::Spikes, Vector3Add(me, {-1.5f, 0.0f, -2.5f}), false);
        structures_->place(StructureKind::Mine, Vector3Add(me, {0.5f, 0.0f, -3.5f}), false);
        structures_->place(StructureKind::BearTrap, Vector3Add(me, {1.5f, 0.0f, -2.5f}), false);
        structures_->place(StructureKind::OilSlick, Vector3Add(me, {-3.5f, 0.0f, -3.5f}), false);
        traps_.igniteAt(Vector3Add(me, {-3.5f, 0.0f, -3.5f}), 0.5f);
        // Des morts derrière la palissade : ils la frappent pour passer (barre de PV sur la capture).
        waves_->spawnRing(Vector3Add(player_->position(), {2.0f, 0.0f, 4.0f}), 0.6f, 3, EnemyType::Walker);
        musicDirector_.startNight(audio_); // vérifie aussi le chargement des pistes
    } else {
        musicDirector_.menu(audio_);
    }
}

Game::~Game()
{
    for (Model& model : weaponModels_) UnloadModel(model);
    UnloadModel(lanternModel_);
    lighting_.reset();
    postFx_.reset();
    particles_.reset();
    waves_.reset();
    shop_.reset();
    merchant_.reset();
    merchantModel_.reset();
    mysteryBox_.reset();
    crates_.reset();
    structures_.reset();
    player_.reset();
    playerModel_.reset();
    zones_.reset();
    world_.reset();
    CloseAudioDevice();
    fonts::unload();
    CloseWindow();
}

void Game::loadAudio()
{
    audio_.loadGroup("gun_pistol", {"gun_pistol.ogg"}, 8);
    audio_.loadGroup("gun_shotgun", {"gun_shotgun.ogg"}, 4);
    audio_.loadGroup("gun_rifle", {"gun_rifle.ogg"}, 4);
    audio_.loadGroup("dry_fire", {"ui_error.ogg"}, 2);
    audio_.loadGroup("reload", {"reload_000.ogg", "reload_001.ogg", "reload_002.ogg"}, 2);
    audio_.loadGroup("hit_flesh", {"hit_flesh_000.ogg", "hit_flesh_001.ogg", "hit_flesh_002.ogg",
                                   "hit_flesh_003.ogg", "hit_flesh_004.ogg"}, 4);
    audio_.loadGroup("hit_wood", {"hit_wood_000.ogg", "hit_wood_001.ogg", "hit_wood_002.ogg", "hit_wood_003.ogg",
                                  "hit_wood_004.ogg"}, 4);
    audio_.loadGroup("hit_heavy", {"hit_heavy_000.ogg", "hit_heavy_001.ogg", "hit_heavy_002.ogg"}, 2);
    audio_.loadGroup("body_fall", {"body_fall_000.ogg", "body_fall_001.ogg", "body_fall_002.ogg"}, 4);
    audio_.loadGroup("zombie_groan", {"zombie_groan_0.ogg", "zombie_groan_1.ogg", "zombie_moan_0.ogg",
                                      "zombie_moan_1.ogg", "zombie_moan_2.ogg", "zombie_moan_3.ogg"}, 3);
    audio_.loadGroup("zombie_runner", {"zombie_runner.ogg"}, 3);
    audio_.loadGroup("zombie_attack", {"zombie_attack.ogg"}, 3);
    audio_.loadGroup("footstep", {"footstep_grass_000.ogg", "footstep_grass_001.ogg", "footstep_grass_002.ogg",
                                  "footstep_grass_003.ogg", "footstep_grass_004.ogg"}, 2);
    audio_.loadGroup("wave_bell", {"wave_bell.ogg"}, 1);
    audio_.loadGroup("ui_confirm", {"ui_confirm.ogg"}, 1);
    audio_.loadGroup("pickup", {"pickup.ogg"}, 6);
    audio_.loadGroup("ui_click", {"ui_click.ogg"}, 2);
}

void Game::run()
{
    while (!WindowShouldClose() && !quitRequested_) {
        // En --smoke, la simulation avance à pas fixe : même durée simulée quelle que soit la machine.
        const float dt = options_.smokeFrames > 0 ? kSmokeFrameTime : std::min(GetFrameTime(), kMaxFrameTime);
        update(dt);
        draw();

        ++frameCount_;
        if (options_.smokeFrames > 0 && frameCount_ >= options_.smokeFrames) {
            if (!options_.smokeScreenshot.empty()) TakeScreenshot(options_.smokeScreenshot.c_str());
            break;
        }
    }
}

void Game::startNewGame()
{
    if (progression_->selectedCharacter() != character_) loadCharacter(progression_->selectedCharacter());
    // Les bonus des cartes sont remis à zéro d'abord : la vie et les chargeurs en dépendent.
    cards_.reset();
    cards_.setRarityBonus(progression_->rarityBonusPercent());
    applyCardModifiers();
    cardEffects_.clear();
    pendingExplosions_.clear();
    flashlight_.reset();
    flyingLanterns_.reset();
    levelUpSlowMo_ = 0.0f;
    pocketSunTimer_ = kPocketSunInterval;
    lastStandReady_ = true;
    player_->reset(world_->playerSpawn());
    zones_->reset();
    if (mysteryBox_) mysteryBox_->reset();
    if (altar_) altar_->reset();
    crates_->clear();
    structures_->clear();
    traps_.clear();
    turrets_->clear();
    buildMode_.reset();
    flowFieldTimer_ = 0.0f;
    arsenal_.reset();
    if (progression_->startsWithShotgun()) arsenal_.unlock(kArmoryWeapon);
    waves_->reset();
    waves_->setRules(gameModeRules(mode_).endless, gameModeRules(mode_).enemyHealth);
    projectiles_.clear();
    economy_.reset();
    economy_.addGold(progression_->startGold());
    for (int i = 0; i < progression_->freeCardRerolls(); ++i) economy_.addCardReroll();
    pickups_.clear();
    particles_->clear();
    hud_.clear();
    events_.clear();
    cameraRig_.snapTo(player_->position());
    score_ = 0;
    kills_ = 0;
    hitStop_ = 0.0f;
    bonesEarned_ = 0;
}

Vector3 Game::mouseAimPoint() const
{
    const Ray ray = GetScreenToWorldRay(GetMousePosition(), cameraRig_.camera());
    // Un mort sous le curseur : on vise son torse, pas le sol derrière lui.
    float nearest = 1e9f;
    Vector3 target = {0.0f, 0.0f, 0.0f};
    bool onEnemy = false;
    for (const Enemy& e : waves_->enemies()) {
        if (!e.isAlive()) continue;
        const RayCollision hit = GetRayCollisionSphere(ray, e.hitCenter(), e.radius() * kAimAssistScale);
        if (hit.hit && hit.distance < nearest) {
            nearest = hit.distance;
            target = e.hitCenter();
            onEnemy = true;
        }
    }
    if (onEnemy) return target;
    // Sinon, le curseur est projeté à hauteur du canon : les balles volent là, pas au ras du sol.
    return mouseOnPlane(player_->muzzle().y);
}

Vector3 Game::mouseOnPlane(float height) const
{
    const Ray ray = GetScreenToWorldRay(GetMousePosition(), cameraRig_.camera());
    if (std::fabs(ray.direction.y) < 1e-4f) return player_->position();
    const float t = (height - ray.position.y) / ray.direction.y;
    return {ray.position.x + ray.direction.x * t, height, ray.position.z + ray.direction.z * t};
}

Game::Controls Game::readControls() const
{
    Controls c;
    c.move = readMoveKeys();
    c.aim = mouseAimPoint();
    c.cursor = mouseOnPlane(0.0f);
    c.sprint = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    c.fireHeld = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    c.firePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    c.reload = IsKeyPressed(KEY_R);
    c.interact = IsKeyPressed(KEY_E);
    c.light = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    for (int i = 0; i < 4; ++i) {
        if (IsKeyPressed(KEY_ONE + i)) c.selectWeapon = i;
    }
    const float wheel = GetMouseWheelMove();
    if (wheel > 0.0f) c.cycleWeapon = -1;
    if (wheel < 0.0f) c.cycleWeapon = 1;
    return c;
}

Game::Controls Game::autopilot() const
{
    Controls c;
    const Vector3 me = player_->position();
    c.aim = Vector3Add(me, player_->forward());
    float best = 1e9f;
    for (const Enemy& e : waves_->enemies()) {
        if (!e.isAlive()) continue;
        const float d = Vector3DistanceSqr(e.position(), me);
        if (d < best) {
            best = d;
            c.aim = e.position();
        }
    }
    c.fireHeld = true;
    c.firePressed = (frameCount_ % 12) == 0;
    c.light = true;
    return c;
}

void Game::update(float dt)
{
    audio_.update(dt);
    hud_.update(dt);
    postFx_->update(dt, 1.0f - player_->health() / player_->maxHealth());

    // --smoke : un niveau gagné juste avant la fin, pour que la capture montre l'écran des cartes.
    if (options_.smokeFrames > 0 && frameCount_ == options_.smokeFrames - kSmokeShowcaseFrames) {
        economy_.addXp(economy_.xpToNextLevel() - economy_.xp(), events_);
    }

    switch (state_) {
    case State::Menu: {
        const MainMenu::Result menu = mainMenu_.update(dt, *progression_);
        if (menu.moved) audio_.play("ui_click", 0.35f, 0.1f);
        if (menu.refused) audio_.play("dry_fire", 0.6f, 0.0f);
        if (menu.quit) quitRequested_ = true;
        if (menu.selectCharacter >= 0) progression_->select(static_cast<CharacterId>(menu.selectCharacter));
        if (menu.buyCharacter >= 0) {
            const bool bought = progression_->buy(static_cast<CharacterId>(menu.buyCharacter));
            audio_.play(bought ? "ui_confirm" : "dry_fire", 0.8f, 0.0f);
        }
        if (menu.startMode >= 0) {
            mode_ = static_cast<GameMode>(menu.startMode);
            audio_.play("ui_confirm", 0.8f, 0.0f);
            startNewGame();
            musicDirector_.startNight(audio_);
            state_ = State::Playing;
        } else if (menu.openTalents) {
            talentScreen_.open();
            audio_.play("ui_click", 0.8f, 0.0f);
            state_ = State::Talents;
        }
        break;
    }
    case State::Talents:
        updateTalents(dt);
        break;
    case State::Playing:
        if (IsKeyPressed(KEY_ESCAPE)) {
            state_ = State::Paused;
            break;
        }
        updatePlaying(dt);
        break;
    case State::LevelUp:
        updateLevelUp(dt);
        break;
    case State::Shop:
        updateShop(dt);
        break;
    case State::Paused:
        if (IsKeyPressed(KEY_ESCAPE)) state_ = State::Playing;
        if (IsKeyPressed(KEY_Q) && IsKeyDown(KEY_LEFT_CONTROL)) {
            musicDirector_.menu(audio_);
            state_ = State::Menu;
        }
        break;
    case State::GameOver:
    case State::Victory:
        cameraRig_.update(dt, player_->position());
        if (IsKeyPressed(KEY_ENTER)) {
            startNewGame();
            musicDirector_.startNight(audio_);
            state_ = State::Playing;
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            musicDirector_.menu(audio_);
            state_ = State::Menu;
        }
        break;
    }
    updateLighting(dt);
}

void Game::updateLighting(float dt)
{
    const Vector3 position = player_->position();
    const Vector3 forward = player_->forward();
    const Vector3 lamp = {position.x + forward.x * kFlashlightForward, position.y + kFlashlightHeight,
                          position.z + forward.z * kFlashlightForward};
    lighting_->setFlashlight(flashlight_.isOn(), flashlight_.intensity());
    lighting_->aimFlashlight(lamp, {forward.x, -kFlashlightTilt, forward.z});
    waves_->setFlashlight(flashlight_.isLit(), lamp, forward);

    PointLight fill;
    fill.position = Vector3Add(position, {0.0f, kPlayerFillHeight, 0.0f});
    fill.color = kPlayerFillColor;
    fill.range = kPlayerFillRange;
    fill.intensity = kPlayerFillIntensity;
    lighting_->addLight(fill);
    for (const PointLight& lantern : flyingLanterns_.lights()) lighting_->addLight(lantern);
    if (mysteryBox_) lighting_->addLight(mysteryBox_->light());
    if (altar_) lighting_->addLight(altar_->light());
    for (const PointLight& fire : traps_.lights(*structures_)) lighting_->addLight(fire);
    lighting_->update(dt, cameraRig_.camera().target);
}

void Game::updatePlaying(float dt)
{
    // Montée de niveau : court ralenti, puis l'écran des cartes fige la partie.
    if (levelUpSlowMo_ > 0.0f) {
        levelUpSlowMo_ -= dt;
        if (levelUpSlowMo_ <= 0.0f) {
            openCardPicker();
            return;
        }
        dt *= kSlowMoScale;
    }

    // Hit-stop : la simulation se fige un court instant sur un coup important.
    if (hitStop_ > 0.0f) {
        hitStop_ -= dt;
        return;
    }

    const Controls c = options_.smokeFrames > 0 ? autopilot() : readControls();
    const bool alive = !player_->isDead();

    if (alive && waves_->phase() == WaveManager::Phase::Intermission && IsKeyPressed(KEY_ENTER)) {
        waves_->skipIntermission();
    }

    player_->update(dt, *world_, c.aim, alive ? c.move : Vector3{0.0f, 0.0f, 0.0f}, c.sprint);
    const bool building = updateBase(dt, c);
    flashlight_.update(dt, alive && c.light && !building, player_->position(), events_);
    if (alive && !building) {
        if (c.selectWeapon >= 0) arsenal_.select(c.selectWeapon);
        if (c.cycleWeapon != 0) arsenal_.cycle(c.cycleWeapon);
        if (arsenal_.update(dt, c.fireHeld, c.firePressed, c.reload, player_->muzzle(), player_->forward(),
                            projectiles_, events_)) {
            player_->notifyShot();
            player_->applyRecoil(Vector3Scale(player_->forward(), -arsenal_.stats().recoil));
        }
    }

    const bool intermission = waves_->phase() == WaveManager::Phase::Intermission;
    if (merchant_) merchant_->update(dt, player_->position());
    if (alive && c.interact) {
        // Le marchand passe avant les grilles quand il est à portée.
        if (merchant_ && merchant_->inReach(player_->position())) {
            if (intermission) {
                openShop();
                return;
            }
            hud_.announce("FERME", "Revenez entre les vagues", kWaveColor);
        } else if (mysteryBox_ && mysteryBox_->inReach(player_->position())) {
            takeMysteryPrize(mysteryBox_->interact(player_->position(), *zones_, economy_, events_));
        } else if (traps_.igniteNear(player_->position(), *structures_)) {
            // E près d'une flaque d'huile : elle s'enflamme.
        } else if (altar_ && altar_->inReach(player_->position())) {
            altar_->tryUse(player_->position(), player_->health() < player_->maxHealth(), *zones_, events_);
        } else {
            zones_->tryOpenNearest(player_->position(), economy_, events_);
        }
    }
    zones_->update(dt);
    if (mysteryBox_) mysteryBox_->update(dt, *zones_, events_);
    if (altar_) altar_->update(dt);
    for (const Crates::Broken& crate : crates_->update(dt, events_)) breakCrate(crate);

    projectiles_.update(dt);
    const float damage = waves_->update(dt, player_->position(), player_->forward(), *world_, *zones_, events_);
    resolveProjectiles();
    for (const Flashlight::LightTick& tick :
         flashlight_.illuminate(dt, player_->position(), player_->forward(), waves_->enemies(), *world_)) {
        damageEnemy(*tick.enemy, tick.damage, {0.0f, 0.0f, 0.0f}, 0.0f, HitKind::Light);
    }
    flyingLanterns_.update(dt, player_->position());
    for (const FlyingLanterns::Burn& burn : flyingLanterns_.burn(dt, waves_->enemies())) {
        damageEnemy(*burn.enemy, burn.damage, {0.0f, 0.0f, 0.0f}, 0.0f, HitKind::Light);
    }
    updateCardEffects(dt);
    economy_.update(dt);
    pickups_.update(dt, player_->position(), economy_, events_);
    particles_->update(dt);
    if (alive && c.sprint && Vector3Length(c.move) > 0.0f && player_->stamina() > 0.0f) {
        particles_->sprintDust(player_->position(), c.move, dt);
    }
    if (damage > 0.0f && alive) {
        // Le coup vient du mort le plus proche.
        Vector3 from = player_->position();
        float best = 1e9f;
        for (const Enemy& e : waves_->enemies()) {
            const float d = Vector3DistanceSqr(e.position(), player_->position());
            if (e.isAlive() && d < best) {
                best = d;
                from = e.position();
            }
        }
        hurtPlayer(damage, from);
    }

    const Vector3 toAim = {c.aim.x - player_->position().x, 0.0f, c.aim.z - player_->position().z};
    const float aimDistance = Vector3Length(toAim);
    const Vector3 lookAhead = aimDistance > 0.01f
        ? Vector3Scale(toAim, std::min(kMaxLookAhead, aimDistance * kLookAhead) / aimDistance)
        : Vector3{0.0f, 0.0f, 0.0f};
    cameraRig_.update(dt, player_->position(), lookAhead);
    audio_.setListener(player_->position());
    const int aliveEnemies = static_cast<int>(std::count_if(waves_->enemies().begin(), waves_->enemies().end(),
                                                              [](const Enemy& e) { return e.isAlive(); }));
    musicDirector_.update(dt, audio_, aliveEnemies, waves_->phase() == WaveManager::Phase::Intermission,
                          waves_->wave(), 0);
    updateFootsteps(dt);
    handleEvents();

    if (player_->deathFinished()) {
        endRun(false);
    } else if (waves_->allWavesCleared()) {
        endRun(true);
    } else if (!player_->isDead() && economy_.pendingLevelUps() > 0 && levelUpSlowMo_ <= 0.0f) {
        levelUpSlowMo_ = kLevelUpSlowMo;
    }
}

void Game::resolveProjectiles()
{
    const CardModifiers& m = cards_.modifiers();
    // Les rebonds sont ajoutés après la boucle : on ne modifie pas le tableau qu'on parcourt.
    std::vector<Projectile> rebounds;
    for (Projectile& p : projectiles_.projectiles()) {
        if (!p.alive) continue;
        // Le décor arrête la balle : seuls les morts touchés avant l'obstacle comptent.
        Vector3 impact = p.position;
        const bool blocked = world_->segmentBlocked(p.previous, p.position, impact);
        const float travel = Vector3Distance(p.previous, p.position);
        const float wallT = blocked && travel > 0.0f ? Vector3Distance(p.previous, impact) / travel : 1.0f;
        for (Enemy& e : waves_->enemies()) {
            if (!e.isAlive() || e.id() == p.lastHit) continue;
            // Les balles volent à hauteur de la main : on teste dans le plan du torse.
            const Vector3 center = e.hitCenter();
            const Vector3 a = {p.previous.x, center.y, p.previous.z};
            const Vector3 b = {p.position.x, center.y, p.position.z};
            float t = 0.0f;
            if (!segmentHitsSphere(a, b, center, e.radius(), t) || t > wallT) continue;

            // Crochet onHit : critique, Faucheuse, feu.
            float damage = p.damage;
            HitKind kind = HitKind::Normal;
            if (chance(m.critChance)) {
                damage *= kCritMultiplier;
                kind = HitKind::Critical;
            }
            if (m.reaper && e.health() - damage < e.stats().health * kReaperThreshold) {
                damage = std::max(damage, e.health());
            }
            const bool chained = p.rebound && m.fireChain;
            if (damageEnemy(e, damage, p.velocity, p.knockback, kind)) {
                if (p.ricochets > 0) {
                    if (const Enemy* next = nearestEnemy(center, kRicochetRange)) {
                        Projectile r = p;
                        const float speed = Vector3Length(p.velocity);
                        const Vector3 dir = flatDirection(center, next->hitCenter());
                        r.position = {center.x, p.position.y, center.z};
                        r.previous = r.position;
                        r.velocity = Vector3Scale(dir, speed);
                        r.life = kRicochetRange / speed + kRicochetLifeMargin;
                        r.pierce = 0;
                        r.ricochets = p.ricochets - 1;
                        r.rebound = true;
                        r.lastHit = e.id();
                        rebounds.push_back(r);
                    }
                }
            } else {
                if (p.incendiary || chained) cardEffects_.ignite(e);
                if (m.frost) e.slow(kFrostSlow, kFrostDuration);
            }
            if (chained) spreadFire(e);

            p.lastHit = e.id();
            if (p.pierce > 0) {
                --p.pierce;
            } else {
                p.alive = false;
                break;
            }
        }
        if (p.alive && blocked) {
            crates_->hitAt(impact, p.damage);
            p.alive = false;
            events_.push_back({EventType::BulletImpact, impact, 0.0f, Vector3Normalize(p.velocity)});
        }
    }
    for (const Projectile& r : rebounds) projectiles_.spawn(r);
}

bool Game::damageEnemy(Enemy& enemy, float damage, Vector3 direction, float knockback, HitKind kind, bool byDefense)
{
    if (!enemy.isAlive()) return false;
    const size_t first = events_.size();
    const bool killed = enemy.hit(damage, direction, knockback, events_);
    // Enemy::hit publie d'abord EnemyHit : on y précise la nature du coup pour le HUD et le son.
    if (events_.size() > first && events_[first].type == EventType::EnemyHit) events_[first].hit = kind;
    if (killed) rewardKill(enemy, byDefense);
    return killed;
}

Enemy* Game::nearestEnemy(Vector3 from, float maxDistance)
{
    Enemy* best = nullptr;
    float bestDistance = maxDistance;
    for (Enemy& e : waves_->enemies()) {
        if (!e.isAlive()) continue;
        const float d = flatDistance(e.position(), from);
        if (d < bestDistance) {
            bestDistance = d;
            best = &e;
        }
    }
    return best;
}

void Game::spreadFire(const Enemy& source)
{
    std::vector<std::pair<float, Enemy*>> around;
    for (Enemy& e : waves_->enemies()) {
        if (!e.isAlive() || e.id() == source.id()) continue;
        const float d = flatDistance(e.position(), source.position());
        if (d <= kFireChainRadius) around.emplace_back(d, &e);
    }
    const size_t count = std::min(around.size(), static_cast<size_t>(kFireChainTargets));
    std::partial_sort(around.begin(), around.begin() + static_cast<std::ptrdiff_t>(count), around.end(),
                      [](const auto& a, const auto& b) { return a.first < b.first; });
    for (size_t i = 0; i < count; ++i) cardEffects_.ignite(*around[i].second);
}

void Game::updateCardEffects(float dt)
{
    const CardModifiers& m = cards_.modifiers();
    for (const CardEffects::BurnTick& tick : cardEffects_.update(dt, waves_->enemies())) {
        damageEnemy(*tick.enemy, tick.damage, {0.0f, 0.0f, 0.0f}, 0.0f, HitKind::Burn);
    }

    if (m.pocketSun && !player_->isDead()) {
        pocketSunTimer_ -= dt;
        if (pocketSunTimer_ <= 0.0f) {
            pocketSunTimer_ = kPocketSunInterval;
            const Vector3 me = player_->position();
            cardEffects_.addBlast(Vector3Add(me, {0.0f, 1.0f, 0.0f}), kPocketSunRadius, kSunColor);
            events_.push_back({EventType::Explosion, me, kPocketSunRadius});
            for (Enemy& e : waves_->enemies()) {
                if (!e.isAlive() || flatDistance(e.position(), me) > kPocketSunRadius) continue;
                if (!damageEnemy(e, kPocketSunDamage, flatDirection(me, e.position()), kNecroticKnockback)) {
                    cardEffects_.ignite(e);
                }
            }
        }
    }

    // Explosions nécrotiques : une explosion peut en déclencher d'autres, traitées dans la même image.
    while (!pendingExplosions_.empty()) {
        const Vector3 center = pendingExplosions_.back();
        pendingExplosions_.pop_back();
        cardEffects_.addBlast(center, kNecroticRadius, kNecroticColor);
        events_.push_back({EventType::Explosion, center, kNecroticRadius});
        for (Enemy& e : waves_->enemies()) {
            if (!e.isAlive() || flatDistance(e.position(), center) > kNecroticRadius + e.radius()) continue;
            damageEnemy(e, kNecroticDamage, flatDirection(center, e.position()), kNecroticKnockback);
        }
    }
}

void Game::hurtPlayer(float damage, Vector3 from)
{
    // Dernier rempart : le coup mortel laisse à 1 PV et repousse tous les morts proches.
    const bool lethal = damage >= player_->health() && !player_->isInvulnerable();
    if (lethal && cards_.modifiers().lastStand && lastStandReady_) {
        lastStandReady_ = false;
        damage = std::max(0.0f, player_->health() - 1.0f);
        const Vector3 me = player_->position();
        cardEffects_.addBlast(Vector3Add(me, {0.0f, 1.0f, 0.0f}), kLastStandRadius, kLastStandColor);
        events_.push_back({EventType::LastStand, me});
        for (Enemy& e : waves_->enemies()) {
            if (e.isAlive() && flatDistance(e.position(), me) <= kLastStandRadius) {
                damageEnemy(e, kLastStandDamage, flatDirection(me, e.position()), kLastStandKnockback);
            }
        }
        if (damage <= 0.0f) return;
    }
    player_->takeDamage(damage, from, events_);
}

void Game::applyCardModifiers()
{
    // Cartes, talents, personnage et mode de jeu se cumulent.
    const CardModifiers& m = cards_.modifiers();
    const CharacterInfo& c = characterInfo(character_);
    CardModifiers weapons = m;
    weapons.damageMultiplier *= c.damage;
    arsenal_.setModifiers(weapons);
    player_->setSpeedMultiplier(m.moveSpeedMultiplier * c.speed);
    player_->setStaminaMultipliers(m.staminaMultiplier * c.staminaCapacity, m.staminaRegenMultiplier * c.staminaRegen);
    player_->setMaxHealthBonus(m.maxHealthBonus + progression_->startHealthBonus() + c.healthBonus);
    economy_.setGoldMultiplier(m.goldMultiplier * gameModeRules(mode_).gold);
    pickups_.setMagnetMultiplier(m.magnetMultiplier);
    flashlight_.setModifiers(m.batteryMultiplier * progression_->batteryMultiplier() * c.battery,
                             m.lightBonusDamage);
    flyingLanterns_.setCount(m.lanterns);
}

void Game::openShop()
{
    shopScreen_.open(arsenal_.current(), economy_.gold());
    shopFrozenLeft_ = kShopMaxFrozen;
    audio_.play("ui_click", 0.8f, 0.0f);
    state_ = State::Shop;
}

void Game::takeMysteryPrize(const MysteryBox::Prize& prize)
{
    if (prize.refused) audio_.play("dry_fire", 0.6f, 0.0f);
    if (prize.weapon < 0) return;
    arsenal_.unlock(prize.weapon);
    if (prize.legendary) arsenal_.makeLegendary(prize.weapon);
    arsenal_.select(prize.weapon);
    audio_.play("reload", 0.8f, 0.0f);
    hud_.announce(prize.legendary ? "ARME LÉGENDAIRE !" : "BOÎTE MYSTÈRE", arsenal_.displayName(),
                  prize.legendary ? kGateColor : kWaveColor);
}

void Game::breakCrate(const Crates::Broken& crate)
{
    const Vector3 at = crate.position;
    switch (crate.kind) {
    case CrateKind::Wood: pickups_.spawnResource(Resource::Wood, GetRandomValue(3, 5), at); break;
    case CrateKind::Large:
        pickups_.spawnResource(Resource::Scrap, GetRandomValue(2, 4), at);
        pickups_.spawnResource(Resource::Wood, GetRandomValue(1, 2), at);
        break;
    case CrateKind::Ammo:
        // Un chargeur de réserve pour l'arme en main.
        arsenal_.addAmmo(arsenal_.current(), arsenal_.magazineSize());
        pickups_.spawnResource(Resource::Scrap, GetRandomValue(1, 2), at);
        audio_.play("reload", 0.7f, 0.0f);
        break;
    case CrateKind::Barrel:
        if (chance(0.6f)) {
            pickups_.spawnResource(Resource::Fuel, 1, at);
        } else {
            pickups_.spawnLoot(at, GetRandomValue(10, 20), 0);
        }
        break;
    case CrateKind::Powder:
        explode(at, kPowderRadius, kPowderDamage, kNecroticKnockback, kPowderPlayerDamage, kPowderColor);
        break;
    case CrateKind::Count: break;
    }
}

bool Game::updateBase(float dt, const Controls& c)
{
    structures_->update(dt, player_->position());
    flowFieldTimer_ -= dt;
    if (flowFieldTimer_ <= 0.0f) {
        flowFieldTimer_ = kFlowFieldInterval;
        flowField_.rebuild(*world_, *zones_, *structures_, player_->position());
    }
    // Un mort au contact d'une structure qui lui barre la route la frappe.
    for (const Enemy& e : waves_->enemies()) {
        if (!e.isAlive() || e.stats().ethereal) continue;
        const int id = structures_->blockingNear(e.position(), e.radius() + kStructureReach);
        if (id >= 0) structures_->damage(id, Structures::damagePerSecond(e.type()) * dt, events_);
    }
    const Traps::Result trapped = traps_.update(dt, *structures_, waves_->enemies(), events_);
    for (const Traps::Hit& hit : trapped.hits) {
        if (hit.ignite && hit.enemy->isAlive()) cardEffects_.ignite(*hit.enemy);
        damageEnemy(*hit.enemy, hit.damage, {0.0f, 0.0f, 0.0f}, 0.0f, hit.ignite ? HitKind::Burn : HitKind::Normal);
    }
    for (const Traps::Blast& blast : trapped.blasts) {
        explode(blast.center, blast.radius, blast.damage, blast.knockback, 0.0f, kPowderColor);
    }
    for (const Turrets::Hit& hit : turrets_->update(dt, *structures_, waves_->enemies(), *world_, events_)) {
        damageEnemy(*hit.enemy, hit.damage, hit.direction, kTurretKnockback, HitKind::Normal, true);
    }
    if (player_->isDead()) {
        buildMode_.reset();
        return false;
    }
    BuildMode::Surroundings around{*world_, *zones_, player_->position(), {}};
    if (merchant_) around.keepClear.push_back(merchant_->position());
    if (mysteryBox_) around.keepClear.push_back(mysteryBox_->position());
    if (altar_) around.keepClear.push_back(altar_->position());
    buildMode_.update(c.cursor, waves_->phase() == WaveManager::Phase::Intermission, around, *structures_, economy_,
                      events_);
    return buildMode_.isActive();
}

void Game::drawBase() const
{
    structures_->draw();
    traps_.draw(*structures_);
    turrets_->draw(*structures_);
    buildMode_.drawWorld(*structures_);
}

void Game::explode(Vector3 center, float radius, float damage, float knockback, float playerDamage, Color color)
{
    cardEffects_.addBlast(Vector3Add(center, {0.0f, 1.0f, 0.0f}), radius, color);
    events_.push_back({EventType::Explosion, center, radius});
    for (Enemy& e : waves_->enemies()) {
        if (!e.isAlive() || flatDistance(e.position(), center) > radius + e.radius()) continue;
        damageEnemy(e, damage, flatDirection(center, e.position()), knockback);
    }
    if (playerDamage > 0.0f && !player_->isDead() && flatDistance(player_->position(), center) <= radius) {
        hurtPlayer(playerDamage, center);
    }
}

void Game::updateShop(float dt)
{
    audio_.setListener(player_->position());
    shopFrozenLeft_ -= dt;
    const ShopScreen::Result result = shopScreen_.update(dt, *shop_, arsenal_, economy_.gold());
    if (result.moved) audio_.play("ui_click", 0.35f, 0.1f);
    if (result.purchase >= 0) {
        const int weapon = shopScreen_.upgradeWeapon();
        const ShopOffer offer = shop_->offer(result.purchase, weapon);
        const bool bought = shop_->buy(result.purchase, weapon, events_);
        shopScreen_.notifyPurchase(bought, bought ? "Merci, gardien." : Shop::refusalReason(offer));
        audio_.play(bought ? "ui_confirm" : "dry_fire", 0.7f, 0.0f);
    }
    if (result.close || shopFrozenLeft_ <= 0.0f || IsKeyPressed(KEY_ESCAPE)) state_ = State::Playing;
}

void Game::openCardPicker()
{
    if (player_->isDead()) return;
    const std::vector<CardId> choices = cards_.drawChoices(Cards::kChoices);
    if (choices.empty()) {
        // Toutes les cartes sont au rang maximal : le niveau est acquis sans choix.
        while (economy_.pendingLevelUps() > 0) economy_.consumeLevelUp();
        return;
    }
    // Niveau atteint par la montée de niveau en cours (plusieurs peuvent être en attente).
    cardPicker_.open(choices, cards_, economy_.level() - economy_.pendingLevelUps() + 1);
    audio_.play("ui_click", 0.8f, 0.0f);
    state_ = State::LevelUp;
}

void Game::updateLevelUp(float dt)
{
    // --smoke : le pilote automatique prend la première carte, sauf pour la capture finale.
    const bool showcase = frameCount_ >= options_.smokeFrames - kSmokeShowcaseFrames;
    if (options_.smokeFrames > 0 && !showcase && cardPicker_.isSettled()) cardPicker_.choose(0);

    // Relance achetée chez le marchand : trois nouvelles cartes.
    if (IsKeyPressed(KEY_R) && economy_.useCardReroll()) {
        audio_.play("ui_confirm", 0.6f, 0.0f);
        openCardPicker();
        return;
    }

    const CardPicker::Result result = cardPicker_.update(dt);
    if (result.hoverChanged) audio_.play("ui_click", 0.35f, 0.1f);
    if (result.picked) audio_.play("ui_confirm", 0.8f, 0.0f);
    if (result.chosen < 0) return;

    const CardId card = cardPicker_.choice(result.chosen);
    cards_.take(card);
    economy_.consumeLevelUp();
    applyCardModifiers();
    hud_.announce(Cards::info(card).name, Cards::info(card).description, CardPicker::rarityColor(Cards::info(card).rarity));
    state_ = State::Playing;
    // Plusieurs niveaux d'un coup : les choix s'enchaînent.
    if (economy_.pendingLevelUps() > 0) openCardPicker();
}

void Game::updateTalents(float dt)
{
    const TalentScreen::Result result = talentScreen_.update(dt);
    if (result.moved) audio_.play("ui_click", 0.35f, 0.1f);
    if (result.purchase >= 0) {
        const bool bought = progression_->buy(static_cast<TalentId>(result.purchase));
        talentScreen_.notifyPurchase(bought);
        audio_.play(bought ? "ui_confirm" : "dry_fire", 0.8f, 0.0f);
    }
    if (result.close) state_ = State::Menu;
}

void Game::endRun(bool victory)
{
    // Une vague est « survécue » quand elle est terminée : celle en cours ne compte pas.
    const int survived = victory ? waves_->wave() : waves_->wave() - 1;
    bonesEarned_ = progression_->finishRun(mode_, survived, score_, victory);
    state_ = victory ? State::Victory : State::GameOver;
}

void Game::rewardKill(const Enemy& enemy, bool byDefense)
{
    const CardModifiers& m = cards_.modifiers();
    const int multiplierBefore = economy_.comboMultiplier();
    const Economy::Reward reward = byDefense ? economy_.addDefenseKill(enemy.type()) : economy_.addKill(enemy.type());
    pickups_.spawnLoot(enemy.hitCenter(), reward.gold, reward.xp);
    if (chance(Flashlight::batteryDropChance(enemy.type()))) pickups_.spawnBattery(enemy.hitCenter());
    const Economy::Salvage salvage = Economy::rollSalvage(enemy.type());
    pickups_.spawnResource(Resource::Wood, salvage.wood, enemy.hitCenter());
    pickups_.spawnResource(Resource::Scrap, salvage.scrap, enemy.hitCenter());
    hud_.pulseCombo();

    // Crochet onKill : Vampirisme, Explosion nécrotique, Fortune du mort.
    const float lifeSteal = m.lifeSteal + characterInfo(character_).lifeSteal;
    if (lifeSteal > 0.0f) player_->heal(player_->maxHealth() * lifeSteal);
    if (chance(m.explosiveCorpseChance)) pendingExplosions_.push_back(enemy.hitCenter());
    if (m.deadMansFortune && multiplierBefore < kFortuneComboMultiplier &&
        economy_.comboMultiplier() >= kFortuneComboMultiplier) {
        pickups_.spawnCoinRain(player_->position(), kFortuneCoins, kFortuneGoldPerCoin);
    }
}

void Game::handleEvents()
{
    for (const Event& ev : events_) {
        switch (ev.type) {
        case EventType::Shot: {
            const int weapon = static_cast<int>(ev.value);
            audio_.play(kShotSounds[weapon], 0.55f, 0.06f);
            cameraRig_.shake(arsenal_.stats().shake);
            particles_->muzzleFlash(ev.position, ev.direction,
                                    kMuzzleBase + kMuzzleShakeScale * arsenal_.stats().shake);
            lighting_->flash({ev.position, kMuzzleLightColor, kMuzzleLightRange, kMuzzleLightIntensity, 0.0f, 0.0f},
                             kMuzzleLightDuration);
            break;
        }
        case EventType::DryFire:
            audio_.play("dry_fire", 0.5f);
            break;
        case EventType::ReloadStart:
            audio_.play("reload", 0.6f);
            break;
        case EventType::EnemyHit: {
            const Vector3 above = Vector3Add(ev.position, {0.0f, 0.8f, 0.0f});
            if (ev.hit == HitKind::Burn) {
                // Le feu mord en silence, en petits chiffres orange.
                hud_.addDamageNumber(above, ev.value, kBurnColor, 0.7f);
                break;
            }
            if (ev.hit == HitKind::Light) {
                hud_.addDamageNumber(above, ev.value, kLightBurnColor, 0.6f);
                break;
            }
            audio_.playAt("hit_flesh", ev.position, 0.55f, 0.15f);
            if (ev.hit == HitKind::Critical) {
                hud_.addDamageNumber(above, ev.value, kCritColor, 1.45f);
            } else {
                hud_.addDamageNumber(above, ev.value, kDamageColor);
            }
            particles_->bloodHit(ev.position, ev.direction);
            break;
        }
        case EventType::EnemyKilled:
            audio_.playAt("body_fall", ev.position, 0.7f);
            score_ += static_cast<int>(ev.value);
            ++kills_;
            hitStop_ = std::max(hitStop_, kKillHitStop);
            cameraRig_.shake(kKillShake);
            particles_->bloodKill(ev.position, ev.direction);
            postFx_->killFlash();
            break;
        case EventType::EnemyAttack:
            audio_.playAt("zombie_attack", ev.position, 0.8f);
            break;
        case EventType::EnemyGroan:
            audio_.playAt(static_cast<EnemyType>(static_cast<int>(ev.value)) == EnemyType::Runner ? "zombie_runner"
                                                                                                   : "zombie_groan",
                          ev.position, 0.5f, 0.12f);
            break;
        case EventType::PlayerHurt:
            audio_.play("hit_heavy", 0.9f);
            cameraRig_.shake(kHurtShake);
            economy_.onPlayerDamaged();
            hud_.addDamageNumber(Vector3Add(ev.position, {0.0f, 2.0f, 0.0f}), ev.value, kPlayerDamageColor);
            particles_->bloodHit(Vector3Add(ev.position, {0.0f, kPlayerBloodHeight, 0.0f}), ev.direction);
            postFx_->hurt();
            break;
        case EventType::PlayerDied:
            cameraRig_.shake(kDeathShake);
            break;
        case EventType::WaveStarted:
            audio_.play("wave_bell", 0.8f, 0.0f);
            hud_.announce(TextFormat("VAGUE %d", static_cast<int>(ev.value)), "Ils arrivent...", kWaveColor);
            lastStandReady_ = true;
            if (altar_) altar_->recharge();
            buildMode_.reset();
            break;
        case EventType::WaveCleared:
            audio_.play("ui_confirm", 0.7f, 0.0f);
            crates_->spawnWave(kCratesPerPause + static_cast<int>(zones_->openAreas().size()), *zones_,
                               player_->position());
            hud_.announce("VAGUE TERMINÉE", waves_->allWavesCleared() ? "L'aube se lève..." : "Reprends ton souffle",
                          kClearColor);
            break;
        case EventType::GoldPicked:
            audio_.play("pickup", 0.4f, 0.15f);
            hud_.pulseGold();
            break;
        case EventType::XpPicked:
            audio_.play("pickup", 0.25f, 0.1f);
            break;
        case EventType::ItemBought:
            hud_.pulseGold();
            applyCardModifiers(); // une arme achetée ou améliorée doit recevoir les bonus des cartes
            break;
        case EventType::LevelUp:
            audio_.play("ui_confirm", 0.8f, 0.0f);
            hud_.announce(TextFormat("NIVEAU %d !", static_cast<int>(ev.value)), "Une carte t'attend...", kLevelColor);
            break;
        case EventType::GateOpened:
            audio_.playAt("hit_heavy", ev.position, 0.9f, 0.0f);
            cameraRig_.shake(kGateShake);
            hud_.announce("GRILLE OUVERTE", zones_->zoneName(static_cast<int>(ev.value)), kGateColor);
            break;
        case EventType::BulletImpact:
            particles_->impact(ev.position, Vector3Negate(ev.direction));
            audio_.playAt("hit_wood", ev.position, 0.3f, 0.15f);
            break;
        case EventType::Explosion:
            crates_->queueBlast(ev.position, ev.value, kBlastCrateDamage);
            traps_.igniteAt(ev.position, ev.value);
            audio_.playAt("hit_heavy", ev.position, 0.8f, 0.2f);
            cameraRig_.shake(kExplosionShake * std::min(1.0f, ev.value / kNecroticRadius));
            lighting_->flash({Vector3Add(ev.position, {0.0f, kExplosionLightHeight, 0.0f}), kExplosionLightColor,
                              ev.value * 2.0f, kExplosionLightIntensity, 0.0f, 0.0f},
                             kExplosionLightDuration);
            break;
        case EventType::LastStand:
            audio_.play("wave_bell", 0.9f, 0.0f);
            cameraRig_.shake(kDeathShake);
            hud_.announce("DERNIER REMPART", "Pas encore...", kLastStandColor);
            break;
        case EventType::FlashlightSwitched:
            if (ev.value < 0.0f) {
                audio_.play("dry_fire", 0.4f);
            } else {
                audio_.play("ui_click", 0.45f, 0.05f);
            }
            break;
        case EventType::FlashlightFlicker:
            audio_.play("ui_click", 0.2f, 0.3f);
            break;
        case EventType::BatteryPicked:
            flashlight_.recharge(Flashlight::kPickupCharge * ev.value);
            audio_.play("pickup", 0.6f, 0.0f);
            break;
        case EventType::MysteryBoxOpened:
            audio_.playAt("ui_confirm", ev.position, 0.8f, 0.0f);
            break;
        case EventType::MysteryBoxTick:
            audio_.playAt("ui_click", ev.position, 0.5f, 0.25f);
            break;
        case EventType::MysteryBoxReady:
            if (ev.value > 0.0f) {
                audio_.playAt("wave_bell", ev.position, 0.9f, 0.0f);
                cameraRig_.shake(kGateShake);
                lighting_->flash({Vector3Add(ev.position, {0.0f, kExplosionLightHeight, 0.0f}), kGateColor, 8.0f,
                                  kExplosionLightIntensity, 0.0f, 0.0f},
                                 kExplosionLightDuration * 2.0f);
            } else {
                audio_.playAt("ui_confirm", ev.position, 0.8f, 0.1f);
            }
            break;
        case EventType::BuildDenied:
            audio_.play("dry_fire", 0.5f, 0.0f);
            if (waves_->phase() != WaveManager::Phase::Intermission) {
                hud_.announce("PAS PENDANT L'ASSAUT", "Construis entre les vagues (B)", kWaveColor);
            }
            break;
        case EventType::StructurePlaced:
            audio_.playAt("hit_wood", ev.position, 0.8f, 0.15f);
            particles_->debris(ev.position, kCrateWoodColor, 0);
            break;
        case EventType::StructureRemoved:
            audio_.play("pickup", 0.6f, 0.1f);
            particles_->debris(ev.position, kCrateWoodColor, kCrateChunks / 2);
            break;
        case EventType::TrapTriggered:
            audio_.playAt(static_cast<StructureKind>(static_cast<int>(ev.value)) == StructureKind::Mine ? "dry_fire"
                                                                                                         : "hit_heavy",
                          ev.position, 0.6f, 0.2f);
            break;
        case EventType::TurretFired:
            audio_.playAt("reload", ev.position, 0.45f, 0.2f);
            break;
        case EventType::StructureDamaged:
            audio_.playAt("hit_wood", ev.position, 0.5f, 0.2f);
            particles_->debris(Vector3Add(ev.position, {0.0f, 0.6f, 0.0f}), kCrateWoodColor, 5);
            break;
        case EventType::StructureDestroyed:
            audio_.playAt("hit_heavy", ev.position, 0.9f, 0.1f);
            particles_->debris(ev.position, kCrateWoodColor, 20);
            cameraRig_.shake(kStructureDestroyShake);
            break;
        case EventType::ResourcePicked:
            audio_.play("pickup", 0.35f, 0.2f);
            hud_.pulseResources();
            break;
        case EventType::CrateLanded:
            audio_.playAt("body_fall", ev.position, 0.5f, 0.2f);
            particles_->debris(ev.position, kCrateWoodColor, 0);
            break;
        case EventType::CrateBroken:
            audio_.playAt("hit_wood", ev.position, 0.9f, 0.15f);
            particles_->debris(ev.position, kCrateWoodColor, kCrateChunks);
            break;
        case EventType::AltarUsed: {
            const float healed = player_->maxHealth() - player_->health();
            player_->heal(healed);
            audio_.playAt("wave_bell", ev.position, 0.7f, 0.0f);
            audio_.play("pickup", 0.8f, 0.0f);
            hud_.addDamageNumber(Vector3Add(player_->position(), {0.0f, 2.0f, 0.0f}), healed, kAltarHealColor);
            hud_.announce("PRIÈRE EXAUCÉE", "Ta vie est restaurée", kAltarHealColor);
            break;
        }
        case EventType::MysteryBoxMoved:
            audio_.playAt("hit_heavy", ev.position, 0.9f, 0.0f);
            cameraRig_.shake(kGateShake);
            hud_.announce("LA BOÎTE S'EST ENVOLÉE", TextFormat("Elle t'attend : %s",
                          zones_->zoneName(zones_->zoneAt(ev.position)).c_str()), kGateColor);
            break;
        }
    }
    events_.clear();
}

void Game::updateFootsteps(float dt)
{
    if (player_->isDead()) return;
    const bool moving = IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D) ||
                        IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT);
    if (!moving) {
        footstepTimer_ = 0.0f;
        return;
    }
    footstepTimer_ -= dt;
    if (footstepTimer_ <= 0.0f) {
        const bool sprint = IsKeyDown(KEY_LEFT_SHIFT) && player_->stamina() > 0.0f;
        footstepTimer_ = sprint ? kSprintFootstepInterval : kFootstepInterval;
        audio_.play("footstep", 0.25f, 0.1f);
    }
}

HudData Game::hudData() const
{
    HudData d;
    d.health = player_->health();
    d.maxHealth = player_->maxHealth();
    d.stamina = player_->stamina();
    d.battery = flashlight_.battery();
    d.flashlightOn = flashlight_.isOn();
    d.batteryLow = flashlight_.isLow();
    const WeaponStats& weapon = arsenal_.stats();
    d.weaponName = arsenal_.displayName();
    d.magazine = arsenal_.ammoInMagazine();
    d.magazineSize = arsenal_.magazineSize();
    d.reserve = arsenal_.reserve();
    d.infiniteReserve = weapon.infiniteReserve;
    d.reloading = arsenal_.isReloading();
    d.reloadProgress = arsenal_.reloadProgress();
    d.wave = waves_->wave();
    d.intermission = waves_->phase() == WaveManager::Phase::Intermission;
    d.intermissionLeft = waves_->intermissionLeft();
    d.remaining = waves_->remaining();
    d.score = score_;
    d.kills = kills_;
    d.hurtFlash = player_->hurtFlash();
    d.gold = economy_.gold();
    d.level = economy_.level();
    d.xpProgress = economy_.xpProgress();
    d.combo = economy_.combo();
    d.comboMultiplier = economy_.comboMultiplier();
    d.comboTimeLeft = economy_.comboTimeLeft() / Economy::kComboWindow;
    d.wood = economy_.resource(Resource::Wood);
    d.scrap = economy_.resource(Resource::Scrap);
    d.fuel = economy_.resource(Resource::Fuel);
    for (CardId id : cards_.owned()) {
        const CardInfo& card = Cards::info(id);
        d.cards.push_back({card.shortName, CardPicker::categoryColor(card.category),
                           CardPicker::rarityColor(card.rarity), cards_.rank(id), card.maxRank});
    }
    return d;
}

void Game::draw() const
{
    BeginDrawing();
    ClearBackground(kBackgroundColor);

    if (state_ == State::Menu) {
        mainMenu_.draw(*progression_);
    } else if (state_ == State::Talents) {
        talentScreen_.draw(*progression_);
    } else {
        postFx_->begin();
        ClearBackground(kBackgroundColor);
        drawWorld();
        postFx_->end();
        postFx_->draw();
        const bool intermission = waves_->phase() == WaveManager::Phase::Intermission;
        if (state_ == State::Playing && !player_->isDead()) {
            zones_->drawPrompt(cameraRig_.camera(), player_->position(), economy_.gold());
            if (merchant_) merchant_->drawPrompt(cameraRig_.camera(), player_->position(), intermission);
            if (mysteryBox_) mysteryBox_->drawPrompt(cameraRig_.camera(), player_->position(), economy_.gold(), *zones_);
            if (altar_) {
                altar_->drawPrompt(cameraRig_.camera(), player_->position(),
                                   player_->health() < player_->maxHealth(), *zones_);
            }
        }
        hud_.draw(cameraRig_.camera(), hudData());
        if (state_ == State::Playing) {
            structures_->drawHealthBars(cameraRig_.camera(), player_->position());
            buildMode_.drawPrompt(cameraRig_.camera());
            buildBar_.draw(buildMode_, economy_);
        }
        if (state_ == State::LevelUp) cardPicker_.draw();
        if (state_ == State::Shop) {
            shopScreen_.draw(*shop_, arsenal_, economy_.gold(), shopFrozenLeft_, waves_->intermissionLeft());
        }
        if (state_ == State::Paused) drawPauseOverlay();
        if (state_ == State::GameOver) drawEndScreen("TU ES MORT", {220, 50, 50, 255});
        if (state_ == State::Victory) drawEndScreen("L'AUBE SE LÈVE", {255, 210, 120, 255});
    }

    DrawFPS(10, 10);
    EndDrawing();
}

void Game::drawWorld() const
{
    BeginMode3D(cameraRig_.camera());
    world_->draw(cameraRig_.camera());
    zones_->draw();
    if (merchant_) merchant_->draw(waves_->phase() == WaveManager::Phase::Intermission);
    if (mysteryBox_) mysteryBox_->draw(weaponModels_);
    crates_->draw();
    drawBase();
    waves_->draw();
    cardEffects_.draw(waves_->enemies());
    const int weapon = arsenal_.current();
    player_->draw(mysteryBox_ && arsenal_.isLegendary(weapon) ? &mysteryBox_->goldenWeapon(weapon)
                                                              : &weaponModels_[static_cast<size_t>(weapon)]);
    projectiles_.draw();
    drawEyes();
    flyingLanterns_.draw(lanternModel_);
    pickups_.draw();
    particles_->draw(cameraRig_.camera());
    lighting_->drawHalos(cameraRig_.camera());
    EndMode3D();
}

void Game::drawEyes() const
{
    const Camera3D& camera = cameraRig_.camera();
    lighting_->beginGlow();
    for (const Enemy& enemy : waves_->enemies()) {
        const float glow = enemy.eyeGlow();
        if (glow <= 0.0f) continue;
        Vector3 eyes[2];
        enemy.eyePositions(eyes[0], eyes[1]);
        for (const Vector3& eye : eyes) {
            lighting_->drawGlow(camera, eye, kEyeHaloSize, ColorAlpha(kEyeHaloColor, glow * kEyeHaloColor.a / 255.0f));
            lighting_->drawGlow(camera, eye, kEyeCoreSize, ColorAlpha(kEyeCoreColor, glow));
        }
    }
    lighting_->endGlow();
}

void Game::drawPauseOverlay() const
{
    const int w = GetScreenWidth();
    const int h = GetScreenHeight();
    DrawRectangle(0, 0, w, h, {0, 0, 0, 150});
    const char* text = "PAUSE";
    const char* hint = "Échap : reprendre   Ctrl+Q : menu";
    fonts::drawText(text, (w - fonts::measureText(text, 60)) / 2, h / 2 - 60, 60, RAYWHITE);
    fonts::drawText(hint, (w - fonts::measureText(hint, 20)) / 2, h / 2 + 10, 20, LIGHTGRAY);
}

void Game::drawEndScreen(const char* title, Color color) const
{
    const int w = GetScreenWidth();
    const int h = GetScreenHeight();
    DrawRectangle(0, 0, w, h, {0, 0, 0, 170});
    fonts::drawText(title, (w - fonts::measureText(title, 80)) / 2, h / 3, 80, color);
    const char* stats = TextFormat("%s   -   vague %d   -   %d morts   -   score %d", gameModeRules(mode_).name,
                                   waves_->wave(), kills_, score_);
    fonts::drawText(stats, (w - fonts::measureText(stats, 26)) / 2, h / 3 + 110, 26, RAYWHITE);
    const char* bones = TextFormat("+%d ossements   (total %d, record vague %d)", bonesEarned_, progression_->bones(),
                                   progression_->bestWave());
    fonts::drawText(bones, (w - fonts::measureText(bones, 22)) / 2, h / 3 + 150, 22, kBoneColor);
    const int rank = progression_->lastRank();
    if (rank >= 0) {
        const char* place = rank == 0 ? "NOUVEAU RECORD !" : TextFormat("%de au tableau des scores", rank + 1);
        fonts::drawText(place, (w - fonts::measureText(place, 24)) / 2, h / 3 + 186, 24, kRecordColor);
    }
    const char* hint = "Entrée : rejouer   Échap : menu";
    fonts::drawText(hint, (w - fonts::measureText(hint, 22)) / 2, h / 3 + 236, 22, LIGHTGRAY);
}
