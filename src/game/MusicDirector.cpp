#include "game/MusicDirector.hpp"

namespace {
// Combat dès 8 morts en jeu ; retour au calme après 4 s sous 3 morts (ou pendant la pause).
constexpr int kCombatThreshold = 8;
constexpr int kCalmThreshold = 3;
constexpr float kCalmDelay = 4.0f;
// À partir de cette vague, la variante de combat plus intense prend le relais.
constexpr int kIntenseWave = 6;
constexpr int kFinalBossWave = 10;

constexpr float kMenuVolume = 0.45f;
constexpr float kNightVolume = 0.55f;
constexpr float kCombatVolume = 0.5f;
constexpr float kBossVolume = 0.6f;
constexpr float kCricketsVolume = 0.35f;
constexpr float kWindVolume = 0.3f;
}

void MusicDirector::menu(Audio& audio)
{
    setMood(Mood::Menu, audio, 0, 0);
}

void MusicDirector::startNight(Audio& audio)
{
    calmTimer_ = 0.0f;
    setMood(Mood::Calm, audio, 0, 0);
}

void MusicDirector::update(float dt, Audio& audio, int alive, bool intermission, int wave, int bossWave)
{
    if (bossWave > 0) {
        setMood(Mood::Boss, audio, wave, bossWave);
        return;
    }
    if (mood_ != Mood::Combat) {
        if (!intermission && alive >= kCombatThreshold) setMood(Mood::Combat, audio, wave, bossWave);
        return;
    }
    // Hystérésis : on ne retombe au calme qu'après quelques secondes d'accalmie.
    const bool quiet = intermission || alive < kCalmThreshold;
    calmTimer_ = quiet ? calmTimer_ + dt : 0.0f;
    if (calmTimer_ >= kCalmDelay) {
        calmTimer_ = 0.0f;
        setMood(Mood::Calm, audio, wave, bossWave);
    }
}

void MusicDirector::setMood(Mood mood, Audio& audio, int wave, int bossWave)
{
    if (mood == mood_) return;
    mood_ = mood;
    switch (mood) {
    case Mood::Silent:
        audio.playMusic("", 0.0f);
        audio.playAmbience("", 0.0f);
        break;
    case Mood::Menu:
        audio.playMusic("menu.ogg", kMenuVolume);
        audio.playAmbience("", 0.0f);
        break;
    case Mood::Calm:
        audio.playMusic("night_ambience.ogg", kNightVolume);
        audio.playAmbience("crickets.ogg", kCricketsVolume);
        break;
    case Mood::Combat:
        audio.playMusic(wave >= kIntenseWave ? "combat_2.ogg" : "combat.ogg", kCombatVolume);
        audio.playAmbience("wind.ogg", kWindVolume);
        break;
    case Mood::Boss:
        audio.playMusic(bossWave >= kFinalBossWave ? "boss_2.ogg" : "boss_1.ogg", kBossVolume);
        audio.playAmbience("wind.ogg", kWindVolume);
        break;
    }
}
