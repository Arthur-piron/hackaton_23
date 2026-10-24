#include "core/Audio.hpp"

#include "core/AssetPath.hpp"
#include "raymath.h"

#include <algorithm>

namespace {
// Au-delà de cette distance, un son positionné n'est plus audible.
constexpr float kHearingRange = 28.0f;
// Écart horizontal donnant un panoramique maximal.
constexpr float kPanRange = 12.0f;
constexpr float kMaxPan = 0.35f;
constexpr float kMusicFadeSeconds = 2.0f;

float randomRange(float min, float max)
{
    return min + (max - min) * static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f;
}
}

Audio::~Audio()
{
    for (auto& [name, group] : groups_) {
        for (auto& voices : group.variants) {
            // Les alias d'abord : ils partagent les données de la première voix.
            for (auto it = voices.rbegin(); it != voices.rend(); ++it) {
                if (it->alias) UnloadSoundAlias(it->sound);
                else UnloadSound(it->sound);
            }
        }
    }
    for (LoopChannel& channel : channels_) {
        unloadMusic(channel.current);
        unloadMusic(channel.fading);
    }
}

void Audio::loadGroup(const std::string& group, const std::vector<std::string>& files, int voices)
{
    if (!IsAudioDeviceReady()) return;
    Group& g = groups_[group];
    for (const std::string& file : files) {
        const Sound source = LoadSound(assetPath("sounds/" + file).c_str());
        if (!IsSoundValid(source)) continue;
        std::vector<Voice> variant;
        variant.push_back({source, false});
        for (int v = 1; v < voices; ++v) variant.push_back({LoadSoundAlias(source), true});
        g.variants.push_back(std::move(variant));
        g.nextVoice.push_back(0);
    }
}

Sound* Audio::pickVoice(const std::string& group)
{
    const auto it = groups_.find(group);
    if (it == groups_.end() || it->second.variants.empty()) return nullptr;
    Group& g = it->second;
    const size_t variant = static_cast<size_t>(GetRandomValue(0, static_cast<int>(g.variants.size()) - 1));
    size_t& next = g.nextVoice[variant];
    Sound* sound = &g.variants[variant][next].sound;
    next = (next + 1) % g.variants[variant].size();
    return sound;
}

void Audio::play(const std::string& group, float volume, float pitchVariation)
{
    Sound* sound = pickVoice(group);
    if (!sound) return;
    SetSoundVolume(*sound, volume);
    SetSoundPitch(*sound, 1.0f + randomRange(-pitchVariation, pitchVariation));
    SetSoundPan(*sound, 0.5f);
    PlaySound(*sound);
}

void Audio::playAt(const std::string& group, Vector3 position, float volume, float pitchVariation)
{
    const float distance = Vector3Distance(position, listener_);
    const float attenuation = 1.0f - std::clamp(distance / kHearingRange, 0.0f, 1.0f);
    if (attenuation <= 0.0f) return;
    Sound* sound = pickVoice(group);
    if (!sound) return;
    SetSoundVolume(*sound, volume * attenuation * attenuation);
    SetSoundPitch(*sound, 1.0f + randomRange(-pitchVariation, pitchVariation));
    // raylib : pan = niveau du canal gauche (1 = tout à gauche, 0.5 = centre).
    const float side = std::clamp((position.x - listener_.x) / kPanRange, -1.0f, 1.0f);
    SetSoundPan(*sound, 0.5f - side * kMaxPan);
    PlaySound(*sound);
}

void Audio::playLoop(Channel channel, const std::string& file, float volume)
{
    LoopChannel& c = channels_[static_cast<size_t>(channel)];
    if (!IsAudioDeviceReady()) return;
    if (file == c.file) {
        c.volume = volume; // même piste : seul le volume change
        return;
    }
    unloadMusic(c.fading);
    c.fading = c.current;
    c.fadingVolume = c.volume * c.fade;
    c.current = file.empty() ? Music{} : LoadMusicStream(assetPath("music/" + file).c_str());
    c.file = file;
    c.volume = volume;
    c.fade = 0.0f;
    if (IsMusicValid(c.current)) {
        c.current.looping = true;
        SetMusicVolume(c.current, 0.0f);
        PlayMusicStream(c.current);
    }
}

void Audio::update(float dt)
{
    for (LoopChannel& c : channels_) {
        c.fade = std::min(1.0f, c.fade + dt / kMusicFadeSeconds);
        if (IsMusicValid(c.current)) {
            SetMusicVolume(c.current, c.volume * c.fade);
            UpdateMusicStream(c.current);
        }
        if (IsMusicValid(c.fading)) {
            if (c.fade >= 1.0f) {
                unloadMusic(c.fading);
            } else {
                SetMusicVolume(c.fading, c.fadingVolume * (1.0f - c.fade));
                UpdateMusicStream(c.fading);
            }
        }
    }
}

void Audio::unloadMusic(Music& music)
{
    if (IsMusicValid(music)) {
        StopMusicStream(music);
        UnloadMusicStream(music);
    }
    music = Music{};
}
