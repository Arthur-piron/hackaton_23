#include "ui/CardPicker.hpp"

#include "core/Fonts.hpp"
#include "ui/UiScale.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace {
constexpr float kCardWidth = 270.0f;
constexpr float kCardHeight = 410.0f;
constexpr float kCardGap = 44.0f;
constexpr float kRoundness = 0.08f;
constexpr int kSegments = 8;

constexpr float kDimDuration = 0.25f;
constexpr float kIntroDelay = 0.12f;      // décalage entre deux cartes
constexpr float kIntroDuration = 0.4f;
constexpr float kOutroDuration = 0.35f;
constexpr float kHoverSpeed = 12.0f;
constexpr float kHoverScale = 0.07f;
constexpr float kHoverLift = 14.0f;

constexpr Color kDim = {4, 4, 10, 215};
constexpr Color kCardBack = {24, 22, 32, 250};
constexpr Color kCardBackTop = {40, 36, 52, 250};
constexpr Color kText = {238, 232, 218, 255};
constexpr Color kTextDim = {170, 164, 150, 255};
constexpr Color kTitle = {90, 235, 200, 255};
constexpr Color kPipEmpty = {70, 66, 80, 255};

constexpr Color kCommon = {165, 165, 170, 255};
constexpr Color kRare = {80, 150, 255, 255};
constexpr Color kLegendary = {255, 195, 60, 255};

float easeOutBack(float t)
{
    const float c = 1.4f;
    const float u = t - 1.0f;
    return 1.0f + (c + 1.0f) * u * u * u + c * u * u;
}

Color fade(Color c, float alpha)
{
    c.a = static_cast<unsigned char>(std::clamp(static_cast<float>(c.a) * alpha, 0.0f, 255.0f));
    return c;
}

Color mix(Color a, Color b, float t)
{
    const auto lerp = [t](unsigned char x, unsigned char y) {
        return static_cast<unsigned char>(static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t);
    };
    return {lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), lerp(a.a, b.a)};
}

Vector2 measure(const char* text, float size)
{
    return fonts::measureTextEx(text, size);
}

void drawCentered(const char* text, float cx, float y, float size, Color color)
{
    const Vector2 m = measure(text, size);
    fonts::drawTextEx(text, {std::round(cx - m.x / 2.0f), std::round(y)}, size, color);
}

// Coupe un texte en lignes qui tiennent dans maxWidth.
std::vector<std::string> wrap(const std::string& text, float size, float maxWidth)
{
    std::vector<std::string> lines;
    std::string line;
    size_t start = 0;
    while (start < text.size()) {
        size_t end = text.find(' ', start);
        if (end == std::string::npos) end = text.size();
        const std::string word = text.substr(start, end - start);
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && measure(candidate.c_str(), size).x > maxWidth) {
            lines.push_back(line);
            line = word;
        } else {
            line = candidate;
        }
        start = end + 1;
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

// Pictogramme de la famille de la carte, centré en c, de rayon r.
void drawPictogram(CardCategory category, Vector2 c, float r, Color color, Color back)
{
    switch (category) {
    case CardCategory::Weapon: { // balle
        const float w = r * 0.62f;
        DrawRectangleRec({c.x - w / 2.0f, c.y - r * 0.15f, w, r * 0.85f}, color);
        DrawCircleSector({c.x, c.y - r * 0.15f}, w / 2.0f, 180.0f, 360.0f, 16, color);
        DrawRectangleRec({c.x - w / 2.0f - r * 0.06f, c.y + r * 0.55f, w + r * 0.12f, r * 0.16f}, color);
        DrawRectangleRec({c.x - w / 2.0f + r * 0.08f, c.y - r * 0.05f, r * 0.08f, r * 0.5f}, fade(back, 0.5f));
        break;
    }
    case CardCategory::Survival: { // cœur
        const float k = r * 0.36f;
        DrawCircleV({c.x - k * 0.95f, c.y - k * 0.35f}, k, color);
        DrawCircleV({c.x + k * 0.95f, c.y - k * 0.35f}, k, color);
        DrawTriangle({c.x - k * 1.92f, c.y - k * 0.1f}, {c.x, c.y + r * 0.78f}, {c.x + k * 1.92f, c.y - k * 0.1f},
                     color);
        break;
    }
    case CardCategory::Mobility: { // double chevron
        for (int i = 0; i < 2; ++i) {
            const float x = c.x - r * 0.42f + static_cast<float>(i) * r * 0.52f;
            DrawLineEx({x, c.y - r * 0.55f}, {x + r * 0.42f, c.y}, r * 0.2f, color);
            DrawLineEx({x + r * 0.42f, c.y}, {x, c.y + r * 0.55f}, r * 0.2f, color);
        }
        break;
    }
    case CardCategory::Fortune: { // pièce
        DrawCircleV(c, r * 0.72f, color);
        DrawCircleV(c, r * 0.5f, fade(back, 0.45f));
        DrawCircleV(c, r * 0.42f, color);
        DrawCircleV({c.x - r * 0.22f, c.y - r * 0.22f}, r * 0.1f, {255, 250, 225, 230});
        break;
    }
    case CardCategory::Light: { // soleil
        for (int i = 0; i < 8; ++i) {
            const float a = static_cast<float>(i) * PI / 4.0f;
            const Vector2 dir = {std::cos(a), std::sin(a)};
            DrawLineEx({c.x + dir.x * r * 0.52f, c.y + dir.y * r * 0.52f},
                       {c.x + dir.x * r * 0.85f, c.y + dir.y * r * 0.85f}, r * 0.12f, color);
        }
        DrawCircleV(c, r * 0.4f, color);
        break;
    }
    case CardCategory::Occult: { // crâne
        DrawCircleV({c.x, c.y - r * 0.12f}, r * 0.55f, color);
        DrawRectangleRec({c.x - r * 0.32f, c.y + r * 0.2f, r * 0.64f, r * 0.42f}, color);
        DrawCircleV({c.x - r * 0.22f, c.y - r * 0.08f}, r * 0.15f, back);
        DrawCircleV({c.x + r * 0.22f, c.y - r * 0.08f}, r * 0.15f, back);
        for (int i = -1; i <= 1; ++i) {
            DrawRectangleRec({c.x + static_cast<float>(i) * r * 0.17f - r * 0.03f, c.y + r * 0.38f, r * 0.06f, r * 0.24f},
                             back);
        }
        break;
    }
    }
}

const char* rarityName(CardRarity rarity)
{
    switch (rarity) {
    case CardRarity::Common: return "COMMUNE";
    case CardRarity::Rare: return "RARE";
    case CardRarity::Legendary: return "LÉGENDAIRE";
    }
    return "";
}
}

Color CardPicker::rarityColor(CardRarity rarity)
{
    switch (rarity) {
    case CardRarity::Common: return kCommon;
    case CardRarity::Rare: return kRare;
    case CardRarity::Legendary: return kLegendary;
    }
    return kCommon;
}

Color CardPicker::categoryColor(CardCategory category)
{
    switch (category) {
    case CardCategory::Weapon: return {255, 150, 70, 255};
    case CardCategory::Survival: return {235, 70, 80, 255};
    case CardCategory::Mobility: return {120, 220, 110, 255};
    case CardCategory::Fortune: return {255, 205, 90, 255};
    case CardCategory::Light: return {255, 240, 170, 255};
    case CardCategory::Occult: return {185, 120, 255, 255};
    }
    return WHITE;
}

void CardPicker::open(const std::vector<CardId>& choices, const Cards& owned, int level)
{
    slots_.clear();
    for (CardId id : choices) slots_.push_back({id, owned.rank(id) + 1, 0.0f});
    level_ = level;
    hovered_ = -1;
    chosen_ = -1;
    time_ = 0.0f;
    outro_ = 0.0f;
}

bool CardPicker::isSettled() const
{
    const float lastArrival = kIntroDelay * static_cast<float>(slots_.size()) + kIntroDuration;
    return chosen_ < 0 && time_ >= lastArrival;
}

void CardPicker::choose(int index)
{
    if (chosen_ >= 0 || index < 0 || index >= static_cast<int>(slots_.size())) return;
    chosen_ = index;
    outro_ = 0.0f;
}

Rectangle CardPicker::baseRect(int index) const
{
    const float count = static_cast<float>(slots_.size());
    const float total = count * kCardWidth + (count - 1.0f) * kCardGap;
    const float x = (ui::width() - total) / 2.0f + static_cast<float>(index) * (kCardWidth + kCardGap);
    const float y = (ui::height() - kCardHeight) / 2.0f + 30.0f;
    return {x, y, kCardWidth, kCardHeight};
}

Rectangle CardPicker::animatedRect(int index, float& scale, float& alpha) const
{
    Rectangle r = baseRect(index);
    const Slot& slot = slots_[static_cast<size_t>(index)];
    const float t = std::clamp((time_ - kIntroDelay * static_cast<float>(index)) / kIntroDuration, 0.0f, 1.0f);
    const float slide = (1.0f - easeOutBack(t)) * ui::height() * 0.7f;
    alpha = std::min(1.0f, t * 2.5f);
    scale = 1.0f + kHoverScale * slot.hover;
    float lift = kHoverLift * slot.hover;

    if (chosen_ >= 0) {
        const float o = std::clamp(outro_ / kOutroDuration, 0.0f, 1.0f);
        if (index == chosen_) {
            scale += 0.12f * o;
            alpha *= 1.0f - o * o;
        } else {
            lift -= o * o * ui::height() * 0.5f;
            alpha *= 1.0f - o;
        }
    }

    const float w = r.width * scale;
    const float h = r.height * scale;
    r.x -= (w - r.width) / 2.0f;
    r.y -= (h - r.height) / 2.0f + lift - slide;
    r.width = w;
    r.height = h;
    return r;
}

CardPicker::Result CardPicker::update(float dt)
{
    Result result;
    time_ += dt;

    if (chosen_ >= 0) {
        outro_ += dt;
        if (outro_ >= kOutroDuration) result.chosen = chosen_;
        return result;
    }

    // Survol : on teste la position finale, pour que la zone ne bouge pas avec l'agrandissement.
    const Vector2 mouse = ui::mouse();
    int hovered = -1;
    for (int i = 0; i < static_cast<int>(slots_.size()); ++i) {
        if (CheckCollisionPointRec(mouse, baseRect(i))) hovered = i;
    }
    if (hovered != hovered_ && hovered >= 0) result.hoverChanged = true;
    hovered_ = hovered;
    for (int i = 0; i < static_cast<int>(slots_.size()); ++i) {
        Slot& slot = slots_[static_cast<size_t>(i)];
        const float target = i == hovered_ ? 1.0f : 0.0f;
        slot.hover += (target - slot.hover) * (1.0f - std::exp(-kHoverSpeed * dt));
    }

    // Pas de choix tant que les cartes arrivent : évite un clic involontaire hérité du combat.
    if (!isSettled()) return result;
    for (int i = 0; i < static_cast<int>(slots_.size()); ++i) {
        if (IsKeyPressed(KEY_ONE + i) || IsKeyPressed(KEY_KP_1 + i)) choose(i);
    }
    if (hovered_ >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) choose(hovered_);
    result.picked = chosen_ >= 0;
    return result;
}

void CardPicker::drawCard(int index) const
{
    float scale = 1.0f;
    float alpha = 1.0f;
    const Rectangle r = animatedRect(index, scale, alpha);
    if (alpha <= 0.01f) return;
    const Slot& slot = slots_[static_cast<size_t>(index)];
    const CardInfo& card = Cards::info(slot.id);
    const Color rarity = rarityColor(card.rarity);
    const Color category = categoryColor(card.category);
    const float hover = slot.hover;
    const float cx = r.x + r.width / 2.0f;
    const float s = scale;

    // Bordure : grise, bleue, ou dorée qui scintille.
    Color border = rarity;
    if (card.rarity == CardRarity::Legendary) {
        border = mix(rarity, {255, 250, 215, 255}, 0.5f + 0.5f * std::sin(time_ * 4.0f));
    }

    // Halo derrière la carte (plus fort au survol et pour les cartes rares).
    const float glow = (card.rarity == CardRarity::Common ? 0.25f : 0.55f) + 0.45f * hover;
    for (int i = 3; i >= 1; --i) {
        const float g = 6.0f * static_cast<float>(i) * s;
        DrawRectangleRounded({r.x - g, r.y - g, r.width + 2.0f * g, r.height + 2.0f * g}, kRoundness, kSegments,
                             fade(border, alpha * glow * 0.18f));
    }
    DrawRectangleRounded({r.x + 8.0f, r.y + 12.0f, r.width, r.height}, kRoundness, kSegments, fade(BLACK, alpha * 0.5f));
    DrawRectangleRounded(r, kRoundness, kSegments, fade(kCardBack, alpha));
    // Dégradé chaud en haut de la carte, teinté par la famille.
    DrawRectangleGradientV(static_cast<int>(r.x + 6.0f), static_cast<int>(r.y + 6.0f), static_cast<int>(r.width - 12.0f),
                           static_cast<int>(r.height * 0.45f), fade(mix(kCardBackTop, category, 0.18f), alpha),
                           fade(kCardBack, alpha));

    // Reflet qui traverse les cartes légendaires.
    if (card.rarity == CardRarity::Legendary) {
        const Rectangle clip = ui::toScreen(r); // le ciseau travaille en pixels réels
        BeginScissorMode(static_cast<int>(clip.x), static_cast<int>(clip.y), static_cast<int>(clip.width),
                         static_cast<int>(clip.height));
        const float sweep = std::fmod(time_ * 0.6f + static_cast<float>(index) * 0.3f, 1.6f) - 0.3f;
        const float x = r.x + sweep * r.width;
        DrawRectanglePro({x, r.y + r.height / 2.0f, 34.0f * s, r.height * 1.6f}, {17.0f * s, r.height * 0.8f}, 22.0f,
                         fade({255, 240, 190, 255}, alpha * 0.12f));
        EndScissorMode();
    }

    const float thickness = (card.rarity == CardRarity::Common ? 2.5f : 3.5f) * s + 1.5f * hover;
    DrawRectangleRoundedLinesEx(r, kRoundness, kSegments, thickness, fade(border, alpha));

    // Rareté, en haut.
    drawCentered(rarityName(card.rarity), cx, r.y + 16.0f * s, 16.0f * s, fade(rarity, alpha));

    // Emblème : disque de la couleur de la famille et son pictogramme.
    const Vector2 emblem = {cx, r.y + 104.0f * s};
    const float radius = 50.0f * s;
    DrawCircleV(emblem, radius + 4.0f * s, fade(border, alpha * 0.9f));
    DrawCircleV(emblem, radius, fade(mix(kCardBack, category, 0.22f), alpha));
    DrawCircleV(emblem, radius * 0.86f, fade(mix(kCardBack, category, 0.1f), alpha));
    drawPictogram(card.category, emblem, radius * 0.7f, fade(category, alpha), fade(mix(kCardBack, category, 0.1f), alpha));

    // Nom (sur deux lignes si besoin) puis description.
    float y = r.y + 176.0f * s;
    const float nameSize = 26.0f * s;
    for (const std::string& line : wrap(card.name, nameSize, r.width - 28.0f * s)) {
        drawCentered(line.c_str(), cx, y, nameSize, fade(kText, alpha));
        y += nameSize + 4.0f * s;
    }
    y += 6.0f * s;
    DrawRectangleRec({cx - 40.0f * s, y, 80.0f * s, 2.0f * s}, fade(border, alpha * 0.8f));
    y += 14.0f * s;
    const float descSize = 18.0f * s;
    for (const std::string& line : wrap(card.description, descSize, r.width - 36.0f * s)) {
        drawCentered(line.c_str(), cx, y, descSize, fade(kTextDim, alpha));
        y += descSize + 5.0f * s;
    }

    // Rang : pastilles (la nouvelle clignote) et texte.
    const float pipY = r.y + r.height - 46.0f * s;
    const float pipGap = 22.0f * s;
    const float pipStart = cx - pipGap * static_cast<float>(card.maxRank - 1) / 2.0f;
    for (int i = 0; i < card.maxRank; ++i) {
        const Vector2 p = {pipStart + pipGap * static_cast<float>(i), pipY};
        Color pip = kPipEmpty;
        if (i < slot.nextRank - 1) pip = rarity;
        if (i == slot.nextRank - 1) pip = mix(rarity, WHITE, 0.5f + 0.5f * std::sin(time_ * 8.0f));
        DrawCircleV(p, 7.0f * s, fade(pip, alpha));
    }
    const char* rank = card.maxRank == 1 ? "UNIQUE" : TextFormat("RANG %d/%d", slot.nextRank, card.maxRank);
    drawCentered(rank, cx, pipY + 14.0f * s, 15.0f * s, fade(kTextDim, alpha));

    // Touche du clavier, dans le coin.
    const Vector2 key = {r.x + 22.0f * s, r.y + 24.0f * s};
    DrawCircleV(key, 13.0f * s, fade(BLACK, alpha * 0.55f));
    DrawCircleLinesV(key, 13.0f * s, fade(border, alpha));
    drawCentered(TextFormat("%d", index + 1), key.x, key.y - 8.0f * s, 16.0f * s, fade(kText, alpha));

    // Éclair blanc sur la carte choisie.
    if (index == chosen_) {
        const float flash = 1.0f - std::clamp(outro_ / kOutroDuration, 0.0f, 1.0f);
        DrawRectangleRounded(r, kRoundness, kSegments, fade(WHITE, flash * 0.5f));
    }
}

void CardPicker::draw() const
{
    const int w = ui::screenWidth();
    const float dim = std::min(1.0f, time_ / kDimDuration) * (chosen_ >= 0 ? 1.0f - outro_ / kOutroDuration : 1.0f);
    DrawRectangle(0, 0, w, ui::screenHeight(), fade(kDim, dim));

    const float titleAlpha = dim;
    const float top = baseRect(0).y;
    const float cx = static_cast<float>(w) / 2.0f;
    drawCentered(TextFormat("NIVEAU %d", level_), cx, top - 112.0f, 54.0f, fade(kTitle, titleAlpha));
    drawCentered("Choisis une carte", cx, top - 52.0f, 22.0f, fade(kText, titleAlpha * 0.85f));

    for (int i = 0; i < static_cast<int>(slots_.size()); ++i) {
        if (i != hovered_) drawCard(i);
    }
    // La carte survolée passe devant les autres.
    if (hovered_ >= 0) drawCard(hovered_);

    const float hintY = baseRect(0).y + kCardHeight + 36.0f;
    drawCentered("Clic ou touches 1 / 2 / 3", cx, hintY, 18.0f, fade(kTextDim, titleAlpha));

    const Vector2 m = ui::mouse();
    DrawCircleV(m, 6.0f, {255, 230, 170, 230});
    DrawCircleLinesV(m, 10.0f, {255, 230, 170, 160});
}
