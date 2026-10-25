#pragma once

#include "fx/PointLight.hpp"
#include "raylib.h"

#include <utility>
#include <vector>

// Éclairage de nuit : un shader par fragment partagé par tous les modèles
// (ambiance bleutée, lune, lumières ponctuelles, lampe torche, brume et
// brouillard), plus des halos additifs autour des sources. À créer après
// InitWindow et à détruire avant CloseWindow.
class Lighting {
public:
    static constexpr int kMaxLights = 24;
    // Ambiance de la nuit, changée par les événements de vague (fondu de quelques secondes).
    enum class Mood { Night, BloodMoon, Fog };

    Lighting();
    ~Lighting();
    Lighting(const Lighting&) = delete;
    Lighting& operator=(const Lighting&) = delete;

    const Shader& shader() const { return shader_; }
    // Fait passer tous les matériaux du modèle par l'éclairage de nuit.
    void apply(Model& model) const;

    // Lumières fixes du décor.
    void setStaticLights(std::vector<PointLight> lights) { static_ = std::move(lights); }
    // Lumière ajoutée pour l'image en cours seulement (à rappeler à chaque image :
    // lanterne volante, lampe UV...).
    void addLight(const PointLight& light) { dynamic_.push_back(light); }
    // Lumière brève qui s'éteint en `duration` secondes (flash de tir, explosion...).
    void flash(const PointLight& light, float duration);

    // Lampe torche : allumée ou non, intensité 0..1 (batterie faible...).
    void setFlashlight(bool on, float intensity);
    void aimFlashlight(Vector3 position, Vector3 direction);

    void setMood(Mood mood);
    // Coupure de courant : les lampes électriques du décor grésillent puis s'éteignent ; les feux restent.
    void setPowerCut(bool cut) { powerCut_ = cut; }
    // Couleur du fond de l'écran, assortie au brouillard de l'ambiance.
    Color backgroundColor() const;

    // Choisit les lumières proches de focus et envoie les uniforms au shader.
    void update(float dt, Vector3 focus);

    // Halos additifs des sources de lumière (après les objets opaques, dans BeginMode3D).
    void drawHalos(const Camera3D& camera) const;
    // Halo additif isolé, à dessiner entre BeginGlow et EndGlow.
    void drawGlow(const Camera3D& camera, Vector3 position, float size, Color color) const;
    void beginGlow() const;
    void endGlow() const;

private:
    struct Flash {
        PointLight light;
        float duration;
        float left;
    };
    struct Atmosphere {
        Vector3 ambientSky;
        Vector3 ambientGround;
        Vector3 moonColor;
        Vector3 fogColor;
        Vector3 mistColor;
        float fogStart;
        float fogEnd;
    };
    struct ActiveLight {
        PointLight light;
        float strength;  // intensité après vacillement
    };

    float flickerAt(const PointLight& light, int index) const;
    // Intensité d'une lumière du décor pendant une coupure (1 : courant normal).
    float powerAt(const PointLight& light, int index) const;
    static Atmosphere atmosphereOf(Mood mood);
    void sendAtmosphere(float dt);

    Shader shader_{};
    Texture2D glow_{};
    int ambientSkyLoc_ = -1;
    int ambientGroundLoc_ = -1;
    int moonDirectionLoc_ = -1;
    int moonColorLoc_ = -1;
    int lightCountLoc_ = -1;
    int lightPositionsLoc_ = -1;
    int lightColorsLoc_ = -1;
    int spotPositionLoc_ = -1;
    int spotDirectionLoc_ = -1;
    int spotColorLoc_ = -1;
    int spotConeLoc_ = -1;
    int fogColorLoc_ = -1;
    int fogCenterLoc_ = -1;
    int fogRangeLoc_ = -1;
    int mistColorLoc_ = -1;
    int timeLoc_ = -1;

    std::vector<PointLight> static_;
    std::vector<PointLight> dynamic_;
    std::vector<ActiveLight> active_;
    float time_ = 0.0f;
    Atmosphere atmosphere_{};
    Mood mood_ = Mood::Night;
    bool powerCut_ = false;
    float power_ = 1.0f;     // 1 : lampes allumées, 0 : coupure complète
    std::vector<Flash> flashes_;
    bool flashlightOn_ = true;
    float flashlightIntensity_ = 1.0f;
    Vector3 flashlightPosition_ = {0.0f, 1.0f, 0.0f};
    Vector3 flashlightDirection_ = {0.0f, 0.0f, -1.0f};
};
