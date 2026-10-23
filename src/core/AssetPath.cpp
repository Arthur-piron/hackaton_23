#include "core/AssetPath.hpp"

#include "raylib.h"

std::string assetPath(const std::string& relative)
{
    // GetApplicationDirectory() se termine déjà par un séparateur.
    return std::string(GetApplicationDirectory()) + "assets/" + relative;
}
