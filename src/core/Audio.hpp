#pragma once

#include "raylib.h"

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

    // Musique en boucle (relative à assets/music/), avec fondu entre deux pistes.
    void playMusic(const std::string& file, float volume = 0.5f);
    void update(float dt);

private:
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

    Music music_{};
    Music fadingMusic_{};
    std::string musicFile_;
    float musicVolume_ = 0.5f;
    float musicFade_ = 1.0f; // 0 → 1 : fondu d'entrée de music_, de sortie de fadingMusic_
};
