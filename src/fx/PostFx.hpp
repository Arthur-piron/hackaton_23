#pragma once

#include "raylib.h"

// Post-traitement plein écran : la scène 3D est rendue dans une texture, puis
// recopiée à l'écran à travers un shader (vignette, grain, aberration
// chromatique, désaturation rouge quand la vie baisse, flash à chaque mort).
// Le HUD se dessine après, hors de l'effet.
class PostFx {
public:
    PostFx();
    ~PostFx();
    PostFx(const PostFx&) = delete;
    PostFx& operator=(const PostFx&) = delete;

    // Recrée la cible si la fenêtre a changé de taille et fait retomber les effets.
    // lowHealth : 0 en pleine forme, 1 à l'agonie.
    void update(float dt, float lowHealth);
    void hurt();
    void killFlash();

    // Encadre le rendu de la scène.
    void begin() const;
    void end() const;
    // Dessine la scène traitée sur tout l'écran.
    void draw() const;

private:
    void recreateTarget(int width, int height);

    Shader shader_{};
    RenderTexture2D target_{};
    int timeLoc_ = -1;
    int hurtLoc_ = -1;
    int lowHealthLoc_ = -1;
    int flashLoc_ = -1;

    float time_ = 0.0f;
    float hurt_ = 0.0f;
    float lowHealth_ = 0.0f;
    float flash_ = 0.0f;
};
