#include "game/MysteryBox.hpp"

#include "core/AssetPath.hpp"
#include "core/Fonts.hpp"
#include "game/Economy.hpp"
#include "game/Scale.hpp"
#include "game/Weapon.hpp"
#include "game/World.hpp"
#include "game/Zones.hpp"
#include "ui/UiScale.hpp"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr const char* kChestModel = "models/survival/chest.glb";
constexpr float kChestScale = 1.7f * kKenneyScale;
constexpr float kWeaponScale = 0.8f * kKenneyScale;

// Tirage : l'arme affichée change de plus en plus lentement, puis s'arrête sur le lot.
constexpr float kRollDuration = 3.2f;
constexpr float kTickStart = 0.06f;
constexpr float kTickEnd = 0.38f;
constexpr float kLegendaryChance = 0.2f;
// Part des légendaires qui tombent sur le revolver doré (le revolver normal n'est jamais tiré).
constexpr float kGoldenRevolverShare = 0.3f;
// L'arme reste à prendre quelques secondes, puis redescend dans la boîte.
constexpr float kOfferDuration = 8.0f;
constexpr float kOfferSink = 1.5f;
// L'arme sort de la boîte et flotte au-dessus du couvercle.
constexpr float kWeaponRise = 0.5f;
constexpr float kWeaponFloat = 0.55f;
constexpr float kWeaponBob = 0.08f;
constexpr float kWeaponSpin = 2.5f;
// Départ : la boîte tremble de plus en plus, puis s'envole en rétrécissant.
constexpr float kLeaveDuration = 2.4f;
constexpr float kLiftOff = 0.7f;      // dernières secondes du départ : envol
constexpr float kLiftHeight = 5.0f;
constexpr float kShakeAmplitude = 0.12f;
constexpr float kShakeFrequency = 40.0f;
constexpr float kArriveDuration = 0.9f;

constexpr Color kGoldTint = {255, 200, 70, 255};
constexpr Color kLightColor = {255, 190, 90, 255};
constexpr float kLightHeight = 1.4f;
constexpr float kLightRange = 5.5f;
constexpr float kIdleIntensity = 0.7f;
constexpr float kRollIntensity = 1.5f;
constexpr float kLegendaryIntensity = 2.2f;
constexpr float kIdleHalo = 1.2f;
constexpr float kActiveHalo = 2.2f;

constexpr float kSignHeight = 2.6f;
constexpr float kSignDistance = 14.0f;
constexpr Color kPromptColor = {255, 215, 120, 255};
constexpr Color kSignColor = {230, 200, 120, 200};
constexpr Color kDeniedColor = {235, 90, 80, 255};
constexpr Color kLegendaryColor = {255, 190, 60, 255};
constexpr Color kShadow = {0, 0, 0, 200};

float smoothstep(float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

bool chance(float probability)
{
    return static_cast<float>(GetRandomValue(0, 9999)) < probability * 10000.0f;
}

int weaponCount()
{
    return static_cast<int>(Arsenal::catalog().size());
}

void drawCenteredText(const char* text, Vector2 at, int size, Color color)
{
    const int x = static_cast<int>(at.x) - fonts::measureText(text, size) / 2;
    const int y = static_cast<int>(at.y);
    fonts::drawText(text, x + 2, y + 2, size, kShadow);
    fonts::drawText(text, x, y, size, color);
}
}

MysteryBox::MysteryBox(const std::vector<MysteryBoxDef>& spots, World& world)
    : world_(world)
{
    chest_ = LoadModel(assetPath(kChestModel).c_str());
    for (const WeaponStats& weapon : Arsenal::catalog()) {
        Model golden = LoadModel(assetPath("models/blaster/" + weapon.model).c_str());
        for (int m = 0; m < golden.materialCount; ++m) golden.materials[m].maps[MATERIAL_MAP_DIFFUSE].color = kGoldTint;
        goldenWeapons_.push_back(golden);
    }

    // Chaque emplacement a sa collision, active seulement quand la boîte s'y trouve.
    const BoundingBox bounds = GetModelBoundingBox(chest_);
    const float halfX = (bounds.max.x - bounds.min.x) * 0.5f * kChestScale;
    const float halfZ = (bounds.max.z - bounds.min.z) * 0.5f * kChestScale;
    chestHeight_ = bounds.max.y * kChestScale;
    // L'origine du modèle n'est pas au centre du coffre : on la décale au dessin.
    chestCenter_ = {(bounds.min.x + bounds.max.x) * 0.5f * kChestScale, 0.0f,
                    (bounds.min.z + bounds.max.z) * 0.5f * kChestScale};
    for (const MysteryBoxDef& def : spots) {
        const float yaw = def.yaw * DEG2RAD;
        const float hx = std::fabs(std::cos(yaw)) * halfX + std::fabs(std::sin(yaw)) * halfZ;
        const float hz = std::fabs(std::sin(yaw)) * halfX + std::fabs(std::cos(yaw)) * halfZ;
        const Vector3 p = def.position;
        const int blocker = world_.addBlocker({{p.x - hx, 0.0f, p.z - hz}, {p.x + hx, chestHeight_, p.z + hz}});
        spots_.push_back({p, yaw, blocker});
    }
    reset();
}

MysteryBox::~MysteryBox()
{
    for (Model& model : goldenWeapons_) UnloadModel(model);
    UnloadModel(chest_);
}

void MysteryBox::applyLighting(const Lighting& lighting)
{
    lighting.apply(chest_);
    for (Model& model : goldenWeapons_) lighting.apply(model);
}

void MysteryBox::reset(int spot)
{
    spot_ = std::clamp(spot, 0, static_cast<int>(spots_.size()) - 1);
    for (size_t i = 0; i < spots_.size(); ++i) {
        world_.setBlockerActive(spots_[i].blocker, static_cast<int>(i) == spot_);
    }
    uses_ = 0;
    state_ = State::Idle;
    stateTime_ = 0.0f;
    prize_ = Prize{};
}

bool MysteryBox::usable(const Zones& zones) const
{
    return state_ == State::Idle && zones.isOpen(zones.zoneAt(position()));
}

bool MysteryBox::inReach(Vector3 player) const
{
    return Vector2Distance({player.x, player.z}, {position().x, position().z}) <= kInteractDistance;
}

int MysteryBox::nearestSpot(Vector3 point) const
{
    int best = 0;
    float bestDistance = 1e9f;
    for (size_t i = 0; i < spots_.size(); ++i) {
        const float d = Vector3Distance(spots_[i].position, point);
        if (d < bestDistance) {
            bestDistance = d;
            best = static_cast<int>(i);
        }
    }
    return best;
}

void MysteryBox::startRoll(EventQueue& events)
{
    prize_.legendary = chance(kLegendaryChance);
    prize_.weapon = prize_.legendary && chance(kGoldenRevolverShare) ? 0 : GetRandomValue(1, weaponCount() - 1);
    shownWeapon_ = GetRandomValue(1, weaponCount() - 1);
    tickTimer_ = kTickStart;
    ++uses_;
    state_ = State::Rolling;
    stateTime_ = 0.0f;
    events.push_back({EventType::MysteryBoxOpened, position()});
}

MysteryBox::Prize MysteryBox::interact(Vector3 player, const Zones& zones, Economy& economy, EventQueue& events)
{
    Prize taken;
    if (!inReach(player)) return taken;
    if (state_ == State::Offering) {
        taken = prize_;
        finishOffer(zones);
    } else if (usable(zones)) {
        if (economy.spend(kPrice)) {
            startRoll(events);
        } else {
            taken.refused = true;
        }
    }
    return taken;
}

void MysteryBox::finishOffer(const Zones& zones)
{
    prize_ = Prize{};
    stateTime_ = 0.0f;
    // Plus aucun autre emplacement ouvert : elle reste, et repart pour un tour.
    const bool canMove = std::any_of(spots_.begin(), spots_.end(), [&](const Spot& s) {
        return &s != &spots_[static_cast<size_t>(spot_)] && zones.isOpen(zones.zoneAt(s.position));
    });
    if (uses_ >= kUsesBeforeMove && canMove) {
        state_ = State::Leaving;
    } else {
        if (uses_ >= kUsesBeforeMove) uses_ = 0;
        state_ = State::Idle;
    }
}

void MysteryBox::moveToAnotherSpot(const Zones& zones)
{
    std::vector<int> candidates;
    for (size_t i = 0; i < spots_.size(); ++i) {
        if (static_cast<int>(i) != spot_ && zones.isOpen(zones.zoneAt(spots_[i].position))) {
            candidates.push_back(static_cast<int>(i));
        }
    }
    if (candidates.empty()) return;
    world_.setBlockerActive(spots_[static_cast<size_t>(spot_)].blocker, false);
    spot_ = candidates[static_cast<size_t>(GetRandomValue(0, static_cast<int>(candidates.size()) - 1))];
    world_.setBlockerActive(spots_[static_cast<size_t>(spot_)].blocker, true);
    uses_ = 0;
}

void MysteryBox::update(float dt, const Zones& zones, EventQueue& events)
{
    time_ += dt;
    stateTime_ += dt;
    switch (state_) {
    case State::Idle: break;
    case State::Rolling:
        tickTimer_ -= dt;
        if (stateTime_ >= kRollDuration) {
            shownWeapon_ = prize_.weapon;
            state_ = State::Offering;
            stateTime_ = 0.0f;
            events.push_back({EventType::MysteryBoxReady, position(), prize_.legendary ? 1.0f : 0.0f});
        } else if (tickTimer_ <= 0.0f) {
            // Jamais deux fois la même arme de suite : on voit bien la roue tourner.
            shownWeapon_ = 1 + (shownWeapon_ - 1 + GetRandomValue(1, weaponCount() - 2)) % (weaponCount() - 1);
            const float progress = stateTime_ / kRollDuration;
            tickTimer_ = kTickStart + (kTickEnd - kTickStart) * progress * progress;
            events.push_back({EventType::MysteryBoxTick, position()});
        }
        break;
    case State::Offering:
        if (stateTime_ >= kOfferDuration) finishOffer(zones);
        break;
    case State::Leaving:
        if (stateTime_ >= kLeaveDuration) {
            moveToAnotherSpot(zones);
            state_ = State::Arriving;
            stateTime_ = 0.0f;
            events.push_back({EventType::MysteryBoxMoved, position()});
        }
        break;
    case State::Arriving:
        if (stateTime_ >= kArriveDuration) {
            state_ = State::Idle;
            stateTime_ = 0.0f;
        }
        break;
    }
}

float MysteryBox::weaponLift() const
{
    if (state_ == State::Rolling) return kWeaponFloat * smoothstep(stateTime_ / kWeaponRise);
    if (state_ != State::Offering) return 0.0f;
    const float sink = std::min(1.0f, (kOfferDuration - stateTime_) / kOfferSink);
    return (kWeaponFloat + kWeaponBob * std::sin(time_ * 3.0f)) * smoothstep(sink);
}

void MysteryBox::draw(const std::vector<Model>& weapons) const
{
    const Spot& spot = spots_[static_cast<size_t>(spot_)];
    Vector3 position = spot.position;
    float scale = kChestScale;
    if (state_ == State::Leaving) {
        // Tremblement croissant, puis envol en rétrécissant.
        const float shake = kShakeAmplitude * smoothstep(stateTime_ / (kLeaveDuration - kLiftOff));
        position.x += shake * std::sin(time_ * kShakeFrequency);
        position.z += shake * std::cos(time_ * kShakeFrequency * 1.3f);
        const float flight = std::max(0.0f, stateTime_ - (kLeaveDuration - kLiftOff)) / kLiftOff;
        position.y += kLiftHeight * flight * flight;
        scale *= 1.0f - smoothstep(flight);
    } else if (state_ == State::Arriving) {
        // Petit rebond à l'apparition.
        const float t = std::clamp(stateTime_ / kArriveDuration, 0.0f, 1.0f);
        scale *= smoothstep(t) * (1.0f + 0.25f * std::sin(t * PI));
    }
    if (scale <= 0.0f) return;
    const Vector3 center = Vector3Scale(Vector3RotateByAxisAngle(chestCenter_, {0.0f, 1.0f, 0.0f}, spot.yaw),
                                        scale / kChestScale);
    DrawModelEx(chest_, Vector3Subtract(position, center), {0.0f, 1.0f, 0.0f}, spot.yaw * RAD2DEG,
                {scale, scale, scale}, WHITE);

    if (state_ != State::Rolling && state_ != State::Offering) return;
    const float lift = weaponLift();
    if (lift <= 0.01f) return;
    const Vector3 at = {position.x, chestHeight_ + lift, position.z};
    const bool golden = state_ == State::Offering && prize_.legendary;
    const Model& model = golden ? goldenWeapons_[static_cast<size_t>(shownWeapon_)]
                                : weapons[static_cast<size_t>(shownWeapon_)];
    DrawModelEx(model, at, {0.0f, 1.0f, 0.0f}, time_ * kWeaponSpin * RAD2DEG, {kWeaponScale, kWeaponScale, kWeaponScale},
                WHITE);
}

void MysteryBox::drawPrompt(const Camera3D& camera, Vector3 player, int gold, const Zones& zones) const
{
    if (state_ == State::Leaving || state_ == State::Arriving) return;
    const float distance = Vector2Distance({player.x, player.z}, {position().x, position().z});
    if (distance > kSignDistance) return;
    const Vector2 screen = ui::fromScreen(GetWorldToScreen(Vector3Add(position(), {0.0f, kSignHeight, 0.0f}), camera));
    if (distance > kInteractDistance) {
        if (state_ == State::Idle) drawCenteredText("BOÎTE MYSTÈRE", screen, 18, kSignColor);
        return;
    }
    if (state_ == State::Offering) {
        const WeaponStats& weapon = Arsenal::catalog()[static_cast<size_t>(prize_.weapon)];
        const std::string& name = prize_.legendary ? weapon.goldenName : weapon.name;
        drawCenteredText(TextFormat("E : prendre %s", name.c_str()), {screen.x, screen.y - 30.0f}, 24,
                         prize_.legendary ? kLegendaryColor : kPromptColor);
        drawCenteredText(TextFormat("%.0f s", std::ceil(kOfferDuration - stateTime_)), screen, 18, kSignColor);
    } else if (state_ == State::Idle) {
        if (!usable(zones)) {
            drawCenteredText("Zone fermée", {screen.x, screen.y - 30.0f}, 24, kDeniedColor);
            return;
        }
        drawCenteredText(TextFormat("E : boîte mystère (%d or)", kPrice), {screen.x, screen.y - 30.0f}, 24,
                         gold >= kPrice ? kPromptColor : kDeniedColor);
        drawCenteredText("Une arme au hasard... peut-être dorée", screen, 18, kSignColor);
    }
}

PointLight MysteryBox::light() const
{
    PointLight light;
    light.position = Vector3Add(position(), {0.0f, kLightHeight, 0.0f});
    light.color = kLightColor;
    light.range = kLightRange;
    light.halo = kIdleHalo;
    switch (state_) {
    case State::Idle:
        light.intensity = kIdleIntensity * (0.8f + 0.2f * std::sin(time_ * 2.0f));
        break;
    case State::Rolling:
        light.intensity = kRollIntensity;
        light.flicker = 0.6f;
        light.halo = kActiveHalo;
        break;
    case State::Offering:
        light.intensity = prize_.legendary ? kLegendaryIntensity : kRollIntensity;
        light.halo = kActiveHalo;
        break;
    case State::Leaving:
        light.intensity = kIdleIntensity * (1.0f - smoothstep(stateTime_ / kLeaveDuration));
        break;
    case State::Arriving:
        light.intensity = kIdleIntensity * smoothstep(stateTime_ / kArriveDuration);
        break;
    }
    return light;
}
