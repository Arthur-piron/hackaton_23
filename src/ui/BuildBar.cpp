#include "ui/BuildBar.hpp"

#include "core/Fonts.hpp"
#include "ui/UiScale.hpp"

#include <cmath>

namespace {
constexpr float kItemWidth = 190.0f;
constexpr float kItemHeight = 62.0f;
constexpr float kGap = 10.0f;
constexpr float kBottomMargin = 130.0f; // au-dessus des barres de vie et des munitions
constexpr float kTabHeight = 26.0f;

constexpr Color kPanel = {12, 12, 20, 200};
constexpr Color kSelected = {255, 215, 120, 255};
constexpr Color kText = {230, 225, 210, 255};
constexpr Color kDim = {150, 145, 135, 255};
constexpr Color kMissing = {240, 95, 85, 255};

const char* const kCategoryNames[] = {"Défenses", "Tourelles", "Pièges", "Utilitaires"};

void drawCentered(const char* text, float cx, float y, int size, Color color)
{
    fonts::drawText(text, static_cast<int>(std::round(cx)) - fonts::measureText(text, size) / 2,
                    static_cast<int>(y), size, color);
}
}

void BuildBar::draw(const BuildMode& mode, const Economy& economy) const
{
    if (!mode.isActive()) return;
    const float w = ui::width();
    const float bottom = ui::height() - kBottomMargin;

    // Onglets des catégories.
    const int categories = static_cast<int>(BuildCategory::Count);
    const float tabWidth = 130.0f;
    float x = (w - tabWidth * static_cast<float>(categories)) / 2.0f;
    const float tabY = bottom - kItemHeight - kTabHeight - 6.0f;
    for (int i = 0; i < categories; ++i) {
        const bool current = static_cast<int>(mode.category()) == i;
        DrawRectangleRounded({x + 2.0f, tabY, tabWidth - 4.0f, kTabHeight}, 0.3f, 4, kPanel);
        drawCentered(TextFormat("%d  %s", i + 1, kCategoryNames[i]), x + tabWidth / 2.0f, tabY + 5.0f, 15,
                     current ? kSelected : kDim);
        x += tabWidth;
    }

    const std::vector<StructureKind> items = mode.items();
    if (items.empty()) {
        drawCentered("Bientôt disponible", w / 2.0f, bottom - kItemHeight / 2.0f - 8.0f, 18, kDim);
    }
    const float total = static_cast<float>(items.size()) * (kItemWidth + kGap) - kGap;
    x = (w - total) / 2.0f;
    for (StructureKind kind : items) {
        const StructureInfo& s = Structures::info(kind);
        const Rectangle r = {x, bottom - kItemHeight, kItemWidth, kItemHeight};
        DrawRectangleRounded(r, 0.2f, 6, kPanel);
        if (kind == mode.selected()) DrawRectangleRoundedLinesEx(r, 0.2f, 6, 2.0f, kSelected);
        drawCentered(s.name, x + kItemWidth / 2.0f, r.y + 8.0f, 20, kind == mode.selected() ? kSelected : kText);
        // Coût : bois, ferraille, or ; chaque nombre en rouge s'il manque.
        const struct {
            int amount;
            int stock;
            const char* unit;
        } costs[] = {{s.wood, economy.resource(Resource::Wood), "bois"},
                     {s.scrap, economy.resource(Resource::Scrap), "fer"},
                     {s.gold, economy.gold(), "or"}};
        float cx = x + 14.0f;
        for (const auto& c : costs) {
            if (c.amount == 0) continue;
            const char* text = TextFormat("%d %s", c.amount, c.unit);
            fonts::drawText(text, static_cast<int>(cx), static_cast<int>(r.y + 36.0f), 15,
                            c.stock >= c.amount ? kText : kMissing);
            cx += static_cast<float>(fonts::measureText(text, 15)) + 14.0f;
        }
        x += kItemWidth + kGap;
    }

    drawCentered("Clic : poser (glisser : en ligne)   T : tourner   U : améliorer   X : revendre   Retour arrière : annuler   B : quitter",
                 w / 2.0f, bottom + 8.0f, 14, kDim);
}
