#include "Game.hpp"

#include "core/AssetPath.hpp"
#include "raylib.h"
#include "raymath.h"

#include <algorithm>
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
constexpr float kFootstepInterval = 0.36f;
constexpr float kSprintFootstepInterval = 0.26f;
constexpr int kSmokeWeapon = 2; // mitraillette
constexpr int kSmokeAmmo = 400;
// Taille du flash de bouche : proportionnelle à la secousse de l'arme.
constexpr float kMuzzleBase = 0.8f;
constexpr float kMuzzleShakeScale = 1.5f;
constexpr float kPlayerBloodHeight = 1.1f;

constexpr Color kBackgroundColor = {12, 14, 22, 255};
constexpr Color kDamageColor = {255, 245, 220, 255};
constexpr Color kPlayerDamageColor = {255, 80, 80, 255};
constexpr Color kWaveColor = {230, 200, 120, 255};
constexpr Color kClearColor = {140, 230, 150, 255};
constexpr Color kLevelColor = {90, 235, 200, 255};

// Sons de tir par arme (indices du catalogue d'Arsenal).
const char* const kShotSounds[] = {"gun_pistol", "gun_shotgun", "gun_pistol", "gun_rifle"};

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
}

Game::Game(const Options& options)
    : options_(options)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(kScreenWidth, kScreenHeight, kWindowTitle);
    InitAudioDevice();
    SetTargetFPS(kTargetFps);
    SetExitKey(KEY_NULL); // Échap sert à la pause, pas à quitter.
    HideCursor();         // remplacé par le réticule du HUD

    keeperModel_ = std::make_unique<AnimatedModel>(assetPath("models/graveyard/character-keeper.glb"));
    player_ = std::make_unique<Player>(*keeperModel_);
    waves_ = std::make_unique<WaveManager>();
    for (const WeaponStats& weapon : Arsenal::catalog()) {
        weaponModels_.push_back(LoadModel(assetPath("models/blaster/" + weapon.model).c_str()));
    }
    particles_ = std::make_unique<Particles>();
    postFx_ = std::make_unique<PostFx>();
    loadAudio();

    startNewGame();
    if (options_.smokeFrames > 0) {
        // Test automatique : directement en combat, face à une horde (le pilote automatique la fauche).
        state_ = State::Playing;
        waves_->spawnRing(Vector3Add(player_->position(), {0.0f, 0.0f, -8.0f}), 2.0f, 10, EnemyType::Walker);
        waves_->spawnRing(Vector3Add(player_->position(), {0.0f, 0.0f, -14.0f}), 2.5f, 6, EnemyType::Runner);
        // La mitraillette enchaîne les morts : la capture montre l'or au sol et un combo.
        arsenal_.unlock(kSmokeWeapon);
        arsenal_.addAmmo(kSmokeWeapon, kSmokeAmmo);
        arsenal_.select(kSmokeWeapon);
    } else {
        audio_.playMusic("menu.ogg", 0.45f);
    }
}

Game::~Game()
{
    for (Model& model : weaponModels_) UnloadModel(model);
    postFx_.reset();
    particles_.reset();
    waves_.reset();
    player_.reset();
    keeperModel_.reset();
    CloseAudioDevice();
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
}

void Game::run()
{
    while (!WindowShouldClose()) {
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
    player_->reset({0.0f, 0.0f, 0.0f});
    arsenal_.reset();
    waves_->reset();
    projectiles_.clear();
    economy_.reset();
    pickups_.clear();
    particles_->clear();
    hud_.clear();
    events_.clear();
    cameraRig_.snapTo(player_->position());
    score_ = 0;
    kills_ = 0;
    hitStop_ = 0.0f;
}

Vector3 Game::mouseGroundPoint() const
{
    const Ray ray = GetScreenToWorldRay(GetMousePosition(), cameraRig_.camera());
    if (std::fabs(ray.direction.y) < 1e-4f) return player_->position();
    const float t = -ray.position.y / ray.direction.y;
    return {ray.position.x + ray.direction.x * t, 0.0f, ray.position.z + ray.direction.z * t};
}

Game::Controls Game::readControls() const
{
    Controls c;
    c.move = readMoveKeys();
    c.aim = mouseGroundPoint();
    c.sprint = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    c.fireHeld = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    c.firePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    c.reload = IsKeyPressed(KEY_R);
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
    return c;
}

void Game::update(float dt)
{
    audio_.update(dt);
    hud_.update(dt);
    postFx_->update(dt, 1.0f - player_->health() / player_->maxHealth());

    switch (state_) {
    case State::Menu:
        if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            startNewGame();
            audio_.playMusic("night_ambience.ogg", 0.6f);
            state_ = State::Playing;
        }
        break;
    case State::Playing:
        if (IsKeyPressed(KEY_ESCAPE)) {
            state_ = State::Paused;
            break;
        }
        updatePlaying(dt);
        break;
    case State::Paused:
        if (IsKeyPressed(KEY_ESCAPE)) state_ = State::Playing;
        if (IsKeyPressed(KEY_Q) && IsKeyDown(KEY_LEFT_CONTROL)) {
            audio_.playMusic("menu.ogg", 0.45f);
            state_ = State::Menu;
        }
        break;
    case State::GameOver:
    case State::Victory:
        cameraRig_.update(dt, player_->position());
        if (IsKeyPressed(KEY_ENTER)) {
            startNewGame();
            audio_.playMusic("night_ambience.ogg", 0.6f);
            state_ = State::Playing;
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            audio_.playMusic("menu.ogg", 0.45f);
            state_ = State::Menu;
        }
        break;
    }
}

void Game::updatePlaying(float dt)
{
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

    player_->update(dt, world_, c.aim, alive ? c.move : Vector3{0.0f, 0.0f, 0.0f}, c.sprint);
    if (alive) {
        if (c.selectWeapon >= 0) arsenal_.select(c.selectWeapon);
        if (c.cycleWeapon != 0) arsenal_.cycle(c.cycleWeapon);
        if (arsenal_.update(dt, c.fireHeld, c.firePressed, c.reload, player_->muzzle(), player_->forward(),
                            projectiles_, events_)) {
            player_->notifyShot();
            player_->applyRecoil(Vector3Scale(player_->forward(), -arsenal_.stats().recoil));
        }
    }

    projectiles_.update(dt);
    const float damage = waves_->update(dt, player_->position(), world_, events_);
    resolveProjectiles();
    economy_.update(dt);
    pickups_.update(dt, player_->position(), economy_, events_);
    resolveWallImpacts();
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
        player_->takeDamage(damage, from, events_);
    }

    const Vector3 toAim = Vector3Subtract(c.aim, player_->position());
    const float aimDistance = Vector3Length(toAim);
    const Vector3 lookAhead = aimDistance > 0.01f
        ? Vector3Scale(toAim, std::min(kMaxLookAhead, aimDistance * kLookAhead) / aimDistance)
        : Vector3{0.0f, 0.0f, 0.0f};
    cameraRig_.update(dt, player_->position(), lookAhead);
    audio_.setListener(player_->position());
    updateFootsteps(dt);
    handleEvents();

    if (player_->deathFinished()) {
        state_ = State::GameOver;
    } else if (waves_->allWavesCleared()) {
        state_ = State::Victory;
    }
}

void Game::resolveProjectiles()
{
    for (Projectile& p : projectiles_.projectiles()) {
        if (!p.alive) continue;
        for (Enemy& e : waves_->enemies()) {
            if (!e.isAlive() || e.id() == p.lastHit) continue;
            // Les balles volent à hauteur de la main : on teste dans le plan du torse.
            const Vector3 center = e.hitCenter();
            const Vector3 a = {p.previous.x, center.y, p.previous.z};
            const Vector3 b = {p.position.x, center.y, p.position.z};
            float t = 0.0f;
            if (!segmentHitsSphere(a, b, center, e.radius(), t)) continue;
            if (e.hit(p.damage, p.velocity, p.knockback, events_)) rewardKill(e);
            p.lastHit = e.id();
            if (p.pierce > 0) {
                --p.pierce;
            } else {
                p.alive = false;
                break;
            }
        }
    }
}

void Game::rewardKill(const Enemy& enemy)
{
    const Economy::Reward reward = economy_.addKill(enemy.type());
    pickups_.spawnLoot(enemy.hitCenter(), reward.gold, reward.xp);
    hud_.pulseCombo();
}

void Game::resolveWallImpacts()
{
    const float wall = World::kHalfSize;
    for (Projectile& p : projectiles_.projectiles()) {
        if (!p.alive || (std::fabs(p.position.x) <= wall && std::fabs(p.position.z) <= wall)) continue;
        p.alive = false;
        // Premier mur franchi le long du segment parcouru pendant l'image.
        float t = 1.0f;
        Vector3 normal = {0.0f, 0.0f, 0.0f};
        const auto crossing = [&](float from, float to, Vector3 axis) {
            if (std::fabs(to) <= wall || std::fabs(from) > wall) return;
            const float side = to > 0.0f ? 1.0f : -1.0f;
            const float s = (side * wall - from) / (to - from);
            if (s < t) {
                t = s;
                normal = Vector3Scale(axis, -side);
            }
        };
        crossing(p.previous.x, p.position.x, {1.0f, 0.0f, 0.0f});
        crossing(p.previous.z, p.position.z, {0.0f, 0.0f, 1.0f});
        if (Vector3Length(normal) > 0.0f) particles_->impact(Vector3Lerp(p.previous, p.position, t), normal);
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
            break;
        }
        case EventType::DryFire:
            audio_.play("dry_fire", 0.5f);
            break;
        case EventType::ReloadStart:
            audio_.play("reload", 0.6f);
            break;
        case EventType::EnemyHit:
            audio_.playAt("hit_flesh", ev.position, 0.55f, 0.15f);
            hud_.addDamageNumber(Vector3Add(ev.position, {0.0f, 0.8f, 0.0f}), ev.value, kDamageColor);
            particles_->bloodHit(ev.position, ev.direction);
            break;
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
            break;
        case EventType::WaveCleared:
            audio_.play("ui_confirm", 0.7f, 0.0f);
            hud_.announce("VAGUE TERMINEE", waves_->allWavesCleared() ? "L'aube se leve..." : "Reprends ton souffle",
                          kClearColor);
            break;
        case EventType::GoldPicked:
            audio_.play("pickup", 0.4f, 0.15f);
            hud_.pulseGold();
            break;
        case EventType::XpPicked:
            audio_.play("pickup", 0.25f, 0.1f);
            break;
        case EventType::LevelUp:
            audio_.play("ui_confirm", 0.8f, 0.0f);
            hud_.announce(TextFormat("NIVEAU %d !", static_cast<int>(ev.value)), "Une carte t'attend...", kLevelColor);
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
    const WeaponStats& weapon = arsenal_.stats();
    d.weaponName = weapon.name;
    d.magazine = arsenal_.ammoInMagazine();
    d.magazineSize = weapon.magazine;
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
    return d;
}

void Game::draw() const
{
    BeginDrawing();
    ClearBackground(kBackgroundColor);

    if (state_ == State::Menu) {
        drawMenu();
    } else {
        postFx_->begin();
        ClearBackground(kBackgroundColor);
        drawWorld();
        postFx_->end();
        postFx_->draw();
        hud_.draw(cameraRig_.camera(), hudData());
        if (state_ == State::Paused) drawPauseOverlay();
        if (state_ == State::GameOver) drawEndScreen("TU ES MORT", {220, 50, 50, 255});
        if (state_ == State::Victory) drawEndScreen("L'AUBE SE LEVE", {255, 210, 120, 255});
    }

    DrawFPS(10, 10);
    EndDrawing();
}

void Game::drawWorld() const
{
    BeginMode3D(cameraRig_.camera());
    world_.draw();
    waves_->draw();
    player_->draw(&weaponModels_[static_cast<size_t>(arsenal_.current())]);
    projectiles_.draw();
    pickups_.draw();
    particles_->draw(cameraRig_.camera());
    EndMode3D();
}

void Game::drawMenu() const
{
    const int w = GetScreenWidth();
    const int h = GetScreenHeight();
    const char* title = "LAST LIGHT";
    DrawText(title, (w - MeasureText(title, 96)) / 2, h / 4, 96, {230, 200, 120, 255});
    const char* tagline = "Les morts craignent la lumiere. Tiens jusqu'a l'aube.";
    DrawText(tagline, (w - MeasureText(tagline, 22)) / 2, h / 4 + 110, 22, {180, 170, 150, 255});
    const char* hint = "Entree pour commencer";
    DrawText(hint, (w - MeasureText(hint, 28)) / 2, h / 2 + 20, 28, RAYWHITE);
    const char* controls = "ZQSD : bouger   Maj : sprint   Souris : viser / tirer   R : recharger   1-4 : armes";
    DrawText(controls, (w - MeasureText(controls, 18)) / 2, h - 60, 18, {140, 135, 125, 255});
    DrawCircleV(GetMousePosition(), 5.0f, {255, 230, 170, 200});
}

void Game::drawPauseOverlay() const
{
    const int w = GetScreenWidth();
    const int h = GetScreenHeight();
    DrawRectangle(0, 0, w, h, {0, 0, 0, 150});
    const char* text = "PAUSE";
    const char* hint = "Echap : reprendre   Ctrl+Q : menu";
    DrawText(text, (w - MeasureText(text, 60)) / 2, h / 2 - 60, 60, RAYWHITE);
    DrawText(hint, (w - MeasureText(hint, 20)) / 2, h / 2 + 10, 20, LIGHTGRAY);
}

void Game::drawEndScreen(const char* title, Color color) const
{
    const int w = GetScreenWidth();
    const int h = GetScreenHeight();
    DrawRectangle(0, 0, w, h, {0, 0, 0, 170});
    DrawText(title, (w - MeasureText(title, 80)) / 2, h / 3, 80, color);
    const char* stats = TextFormat("Vague %d   -   %d morts   -   score %d", waves_->wave(), kills_, score_);
    DrawText(stats, (w - MeasureText(stats, 26)) / 2, h / 3 + 110, 26, RAYWHITE);
    const char* hint = "Entree : rejouer   Echap : menu";
    DrawText(hint, (w - MeasureText(hint, 22)) / 2, h / 3 + 170, 22, LIGHTGRAY);
}
