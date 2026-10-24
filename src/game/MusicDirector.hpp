#pragma once

#include "core/Audio.hpp"

// Musique qui suit la tension de la partie (docs/ASSETS.md, tâche C2) :
// menu → nuit calme (grillons) → combat quand la horde grossit (vent) → boss,
// puis retour au calme quelques secondes après la fin de la vague.
class MusicDirector {
public:
    enum class Mood { Silent, Menu, Calm, Combat, Boss };

    void menu(Audio& audio);
    // Début de partie : nuit calme, sans attendre la première vague.
    void startNight(Audio& audio);
    // alive : morts-vivants en jeu ; bossWave : 0 sans boss, sinon la vague du boss (5 ou 10).
    void update(float dt, Audio& audio, int alive, bool intermission, int wave, int bossWave);

    Mood mood() const { return mood_; }

private:
    void setMood(Mood mood, Audio& audio, int wave, int bossWave);

    Mood mood_ = Mood::Silent;
    float calmTimer_ = 0.0f; // temps passé sous le seuil de retour au calme
};
