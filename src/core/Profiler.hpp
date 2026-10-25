#pragma once

#include "raylib.h"

#include <map>
#include <string>

// Mesure du temps processeur passé dans chaque section d'une image, moyenné sur
// toute la partie ; le rapport est écrit dans le journal à la fin d'un --smoke.
class Profiler {
public:
    // Chronomètre une section tant qu'il existe.
    class Scope {
    public:
        Scope(Profiler& profiler, const char* section)
            : profiler_(profiler)
            , section_(section)
            , start_(GetTime())
        {
        }
        ~Scope() { profiler_.add(section_, GetTime() - start_); }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

    private:
        Profiler& profiler_;
        const char* section_;
        double start_;
    };

    void add(const char* section, double seconds) { totals_[section] += seconds; }
    void endFrame() { ++frames_; }
    void report() const
    {
        if (frames_ == 0) return;
        TraceLog(LOG_INFO, "PROFIL: moyenne sur %d images (ms par image)", frames_);
        for (const auto& [section, total] : totals_) {
            TraceLog(LOG_INFO, "PROFIL:   %-12s %6.2f", section.c_str(), 1000.0 * total / frames_);
        }
    }

private:
    std::map<std::string, double> totals_;
    int frames_ = 0;
};
