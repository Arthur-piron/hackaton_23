#pragma once

#include "raylib.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

// Sons et musique. Un « groupe » rassemble plusieurs variantes d'un même son
// (ex. hit_flesh_000..004) : chaque lecture en choisit une au hasard, avec une
// légère variation de hauteur, pour éviter l'effet répétitif.
class Audio {
public:
    Audio() = default;
    ~Audio();
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    // Charge les fichiers (relatifs à assets/sounds/) dans un groupe.
    // voices : nombre de lectures simultanées possibles par variante.
    void loadGroup(const std::string& group, const std::vector<std::string>& files, int voices = 4);

    void play(const std::string& group, float volume = 1.0f, float pitchVariation = 0.08f);
    // Son positionné : volume selon la distance à l'auditeur, panoramique gauche/droite.
    void playAt(const std::string& group, Vector3 position, float volume = 1.0f, float pitchVariation = 0.08f);
    void setListener(Vector3 position) { listener_ = position; }

    // Deux pistes en boucle (relatives à assets/music/) jouées ensemble : la musique et une
    // ambiance (grillons, vent) en dessous. Changer de piste fait un fondu enchaîné ;
    // un nom vide éteint le canal en fondu.
    enum class Channel { Music, Ambience, Count };
    void playMusic(const std::string& file, float volume = 0.5f) { playLoop(Channel::Music, file, volume); }
    void playAmbience(const std::string& file, float volume = 0.3f) { playLoop(Channel::Ambience, file, volume); }
    void playLoop(Channel channel, const std::string& file, float volume);
    void update(float dt);
    // Réglages de l'écran Options (0..1) : volume général, musique et ambiance, effets.
    void setVolumes(float master, float music, float effects);

private:
    // Une piste qui entre en fondu pendant que la précédente sort.
    struct LoopChannel {
        Music current{};
        Music fading{};
        std::string file;
        float volume = 0.5f;
        float fadingVolume = 0.0f; // volume de départ de la piste sortante
        float fade = 1.0f;         // 0 → 1 : entrée de current, sortie de fading
    };

    struct Voice {
        Sound sound{};
        bool alias = false;
    };
    struct Group {
        std::vector<std::vector<Voice>> variants; // variantes × voix
        std::vector<size_t> nextVoice;
    };

    Sound* pickVoice(const std::string& group);
    void unloadMusic(Music& music);

    std::unordered_map<std::string, Group> groups_;
    Vector3 listener_ = {0.0f, 0.0f, 0.0f};
    float musicVolume_ = 1.0f;
    float effectsVolume_ = 1.0f;

    LoopChannel channels_[static_cast<size_t>(Channel::Count)];
};
