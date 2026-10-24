#pragma once

#include "core/AnimatedModel.hpp"
#include "game/MapFile.hpp"
#include "raylib.h"

// Le marchand : un PNJ immobile derrière son comptoir, qui suit le joueur du regard.
// Il ne vend qu'entre deux vagues ; l'écran d'achat est ui/ShopScreen.
class Merchant {
public:
    static constexpr float kInteractDistance = 2.5f;

    // model doit survivre au marchand (partagé avec d'autres personnages).
    Merchant(const AnimatedModel& model, const MerchantDef& def);

    void update(float dt, Vector3 player);
    // open : boutique ouverte (une pièce tourne au-dessus de sa tête).
    void draw(bool open) const;
    // Enseigne au-dessus du marchand, et invite « E : marchand » à portée.
    void drawPrompt(const Camera3D& camera, Vector3 player, bool open) const;

    Vector3 position() const { return position_; }
    float distanceTo(Vector3 player) const;
    bool inReach(Vector3 player) const { return distanceTo(player) <= kInteractDistance; }

private:
    const AnimatedModel* model_;
    AnimationState anim_;
    Vector3 position_;
    float restYaw_;  // radians : orientation derrière le comptoir
    float yaw_;
    float time_ = 0.0f;
};
