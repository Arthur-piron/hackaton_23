#pragma once

// Crédits (repris de CREDITS.md) : défilent lentement ; Échap, Entrée ou clic pour revenir.
class CreditsScreen {
public:
    void open() { time_ = 0.0f; }
    // Vrai quand le joueur referme l'écran.
    bool update(float dt);
    void draw() const;

private:
    float time_ = 0.0f;
};
