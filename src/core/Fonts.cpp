#include "core/Fonts.hpp"

#include "core/AssetPath.hpp"

#include <algorithm>
#include <vector>

namespace {
constexpr const char* kFontFile = "fonts/kenney_future.ttf";
constexpr int kAtlasSize = 64;         // taille de rastérisation : nette jusqu'aux grands titres
constexpr float kSpacingRatio = 0.06f; // espacement entre lettres, proportionnel à la taille
constexpr float kMinSpaceRatio = 0.6f; // largeur minimale de l'espace, en fraction de la taille

Font g_font{};
bool g_owned = false; // faux : police par défaut de raylib (fichier introuvable), à ne pas libérer

std::vector<int> codepoints()
{
    std::vector<int> cps;
    for (int c = 0x20; c <= 0x7E; ++c) cps.push_back(c);
    for (int c = 0xA0; c <= 0xFF; ++c) cps.push_back(c);   // é è ê à ç ù ô î « » ...
    for (int c = 0x2018; c <= 0x201E; ++c) cps.push_back(c); // ‘ ’ “ ” ...
    return cps;
}
}

namespace fonts {

void load()
{
    std::vector<int> cps = codepoints();
    g_font = LoadFontEx(assetPath(kFontFile).c_str(), kAtlasSize, cps.data(), static_cast<int>(cps.size()));
    g_owned = g_font.texture.id != 0 && g_font.texture.id != GetFontDefault().texture.id;
    if (!g_owned) {
        g_font = GetFontDefault();
        return;
    }
    // L'espace de Kenney Future est à peine plus large que l'écart entre deux lettres :
    // « TU ES MORT » se lirait « TUESMORT » en grand.
    for (int codepoint : {0x20, 0xA0}) { // espace et espace insécable (« 15 % »)
        GlyphInfo& space = g_font.glyphs[GetGlyphIndex(g_font, codepoint)];
        space.advanceX = std::max(space.advanceX, static_cast<int>(kAtlasSize * kMinSpaceRatio));
    }
    GenTextureMipmaps(&g_font.texture);
    SetTextureFilter(g_font.texture, TEXTURE_FILTER_TRILINEAR);
}

void unload()
{
    if (g_owned) UnloadFont(g_font);
    g_font = Font{};
    g_owned = false;
}

const Font& ui()
{
    return g_font;
}

void drawText(const char* text, int x, int y, int size, Color color)
{
    drawTextEx(text, {static_cast<float>(x), static_cast<float>(y)}, static_cast<float>(size), color);
}

int measureText(const char* text, int size)
{
    return static_cast<int>(measureTextEx(text, static_cast<float>(size)).x);
}

void drawTextEx(const char* text, Vector2 position, float size, Color color)
{
    DrawTextEx(g_font, text, position, size, size * kSpacingRatio, color);
}

Vector2 measureTextEx(const char* text, float size)
{
    return MeasureTextEx(g_font, text, size, size * kSpacingRatio);
}

} // namespace fonts
