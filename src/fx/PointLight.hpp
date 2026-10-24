#pragma once

#include "raylib.h"

// Une lumière ponctuelle de la scène (lampadaire, brasero, flash de tir...).
struct PointLight {
    Vector3 position = {0.0f, 0.0f, 0.0f};
    Color color = WHITE;
    float range = 6.0f;      // mètres : au-delà, la lumière n'éclaire plus rien
    float intensity = 1.0f;
    float flicker = 0.0f;    // 0 : stable, 1 : flamme qui vacille fort
    float halo = 0.0f;       // taille du halo dessiné autour de la source (0 : aucun)
};
