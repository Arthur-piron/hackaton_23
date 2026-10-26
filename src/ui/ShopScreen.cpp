#include "ui/ShopScreen.hpp"

#include "core/Fonts.hpp"

#include "ui/CardPicker.hpp"
#include "ui/UiScale.hpp"

#include <algorithm>
#include <cmath>

namespace {
// Mise en page (écran de 1280 x 720, recentrée si la fenêtre change de taille).
constexpr float kMarginX = 36.0f;
constexpr float kColumnGap = 20.0f;
constexpr float kMaxColumnWidth = 400.0f;
constexpr float kHeaderHeight = 86.0f;
constexpr float kRowHeight = 124.0f;
constexpr float kRowGap = 12.0f;
constexpr float kRowInset = 12.0f;
constexpr float kColumnHeight = kHeaderHeight + 3.0f * kRowHeight + 2.0f * kRowGap + kRowInset;
constexpr float kRoundness = 0.12f;
constexpr int kSegments = 8;
constexpr float kIconRadius = 28.0f;
constexpr float kTextLeft = 84.0f;   // début du texte d'une ligne, après l'icône

// Animations.
constexpr float kDimDuration = 0.25f;
constexpr float kIntroDuration = 0.35f;
constexpr float kIntroColumnDelay = 0.07f;
constexpr float kIntroRowDelay = 0.05f;
constexpr float kInputDelay = 0.2f;   // ignore les entrées juste après l'ouverture
constexpr float kHoverSpeed = 14.0f;
constexpr float kFlashDuration = 0.4f;
constexpr float kShakeDuration = 0.35f;
constexpr float kShakeAmplitude = 7.0f;
constexpr float kGoldCountSpeed = 9.0f;
constexpr float kGoldPulseDecay = 3.0f;
constexpr float kMessageDuration = 1.8f;
constexpr float kMouseMoveThreshold = 0.5f;

// Même palette que ui/CardPicker.
constexpr Color kDim = {4, 4, 10, 215};
constexpr Color kPanel = {24, 22, 32, 250};
constexpr Color kRowBack = {40, 36, 52, 250};
constexpr Color kText = {238, 232, 218, 255};
constexpr Color kTextDim = {170, 164, 150, 255};
constexpr Color kTitle = {90, 235, 200, 255};
constexpr Color kPipEmpty = {70, 66, 80, 255};
constexpr Color kGold = {255, 205, 90, 255};
constexpr Color kGoldDark = {170, 110, 20, 255};
constexpr Color kDenied = {235, 80, 80, 255};
constexpr Color kOwned = {120, 220, 110, 255};

const char* const kColumnTitles[] = {"ARMES", "AMÉLIORATIONS", "CONSOMMABLES"};
const char* const kColumnHints[] = {"Débloquées dans l'arsenal", "", "Effet immédiat"};

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

constexpr float kTextScale = 0.8f;

float easeOutBack(float t)
{
    const float c = 1.4f;
    const float u = t - 1.0f;
    return 1.0f + (c + 1.0f) * u * u * u + c * u * u;
}

Vector2 measure(const char* text, float size)
{
    return fonts::measureTextEx(text, size * kTextScale);
}

void drawText(const char* text, float x, float y, float size, Color color)
{
    // Les tailles ont été réglées pour la police raylib, plus étroite que Kenney Future.
    fonts::drawTextEx(text, {std::round(x), std::round(y)}, size * kTextScale, color);
}

void drawRight(const char* text, float right, float y, float size, Color color)
{
    drawText(text, right - measure(text, size).x, y, size, color);
}

void drawCentered(const char* text, float cx, float y, float size, Color color)
{
    drawText(text, cx - measure(text, size).x / 2.0f, y, size, color);
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

Color columnColor(int column)
{
    switch (static_cast<ShopCategory>(column)) {
    case ShopCategory::Weapons: return CardPicker::categoryColor(CardCategory::Weapon);
    case ShopCategory::Upgrades: return CardPicker::rarityColor(CardRarity::Rare);
    case ShopCategory::Consumables: return CardPicker::categoryColor(CardCategory::Mobility);
    case ShopCategory::Count: break;
    }
    return kText;
}

void drawCoin(Vector2 c, float r)
{
    DrawCircleV(c, r, kGoldDark);
    DrawCircleV(c, r * 0.84f, kGold);
    DrawCircleLinesV(c, r * 0.5f, kGoldDark);
    DrawCircleV({c.x - r * 0.32f, c.y - r * 0.32f}, r * 0.2f, {255, 245, 200, 220});
}

void drawBullet(Vector2 c, float w, float h, Color color)
{
    DrawRectangleRec({c.x - w / 2.0f, c.y - h * 0.2f, w, h * 0.7f}, color);
    DrawCircleSector({c.x, c.y - h * 0.2f}, w / 2.0f, 180.0f, 360.0f, 12, color);
}

// Silhouette d'arme : corps, canon de longueur variable, crosse, chargeur ou lunette.
void drawGun(int weapon, Vector2 c, float r, Color color)
{
    const float barrel = weapon == 3 ? 1.0f : weapon == 1 ? 0.8f : 0.45f;
    const float thick = weapon == 1 ? 0.2f : 0.13f;
    const float bodyLeft = c.x - r * 0.75f;
    DrawRectangleRec({bodyLeft, c.y - r * 0.22f, r * 0.75f, r * 0.34f}, color);                 // corps
    DrawRectangleRec({c.x - r * 0.05f, c.y - r * 0.2f, r * barrel, r * thick}, color);          // canon
    DrawTriangle({bodyLeft, c.y - r * 0.22f}, {bodyLeft - r * 0.25f, c.y + r * 0.42f},
                 {bodyLeft + r * 0.2f, c.y + r * 0.12f}, color);                                // crosse
    DrawRectangleRec({c.x - r * 0.45f, c.y + r * 0.1f, r * 0.16f, r * 0.38f}, color);           // poignée
    if (weapon == 2) DrawRectangleRec({c.x - r * 0.12f, c.y + r * 0.1f, r * 0.16f, r * 0.5f}, color); // chargeur
    if (weapon == 3) DrawRectangleRec({c.x - r * 0.35f, c.y - r * 0.42f, r * 0.45f, r * 0.12f}, color); // lunette
    if (weapon == 1) DrawRectangleRec({c.x + r * 0.15f, c.y - r * 0.02f, r * 0.3f, r * 0.14f}, color); // pompe
}

// Pictogramme d'un article, centré en c, de rayon r.
void drawIcon(const ShopItem& item, int weapon, Vector2 c, float r, Color color, Color back)
{
    switch (item.kind) {
    case ShopItemKind::Weapon:
        drawGun(item.weapon, c, r, color);
        break;
    case ShopItemKind::Upgrade:
        switch (item.upgrade) {
        case WeaponUpgrade::Damage:
            drawBullet({c.x, c.y + r * 0.1f}, r * 0.55f, r * 1.3f, color);
            DrawRectangleRec({c.x - r * 0.33f, c.y + r * 0.55f, r * 0.66f, r * 0.14f}, color);
            break;
        case WeaponUpgrade::FireRate:
            for (int i = 0; i < 2; ++i) {
                const float x = c.x - r * 0.42f + static_cast<float>(i) * r * 0.52f;
                DrawLineEx({x, c.y - r * 0.55f}, {x + r * 0.42f, c.y}, r * 0.2f, color);
                DrawLineEx({x + r * 0.42f, c.y}, {x, c.y + r * 0.55f}, r * 0.2f, color);
            }
            break;
        case WeaponUpgrade::Magazine:
            for (int i = -1; i <= 1; ++i) {
                drawBullet({c.x + static_cast<float>(i) * r * 0.42f, c.y + r * 0.05f}, r * 0.3f, r * 1.0f, color);
            }
            DrawRectangleRec({c.x - r * 0.65f, c.y + r * 0.5f, r * 1.3f, r * 0.14f}, color);
            break;
        case WeaponUpgrade::Count:
            break;
        }
        break;
    case ShopItemKind::Heal: {
        const float k = r * 0.36f;
        DrawCircleV({c.x - k * 0.95f, c.y - k * 0.35f}, k, color);
        DrawCircleV({c.x + k * 0.95f, c.y - k * 0.35f}, k, color);
        DrawTriangle({c.x - k * 1.92f, c.y - k * 0.1f}, {c.x, c.y + r * 0.78f}, {c.x + k * 1.92f, c.y - k * 0.1f},
                     color);
        DrawRectangleRec({c.x - r * 0.08f, c.y - r * 0.38f, r * 0.16f, r * 0.56f}, back);
        DrawRectangleRec({c.x - r * 0.28f, c.y - r * 0.18f, r * 0.56f, r * 0.16f}, back);
        break;
    }
    case ShopItemKind::Ammo:
        for (int i = -1; i <= 1; ++i) {
            drawBullet({c.x + static_cast<float>(i) * r * 0.36f, c.y - r * 0.3f}, r * 0.24f, r * 0.7f, color);
        }
        DrawRectangleRec({c.x - r * 0.7f, c.y - r * 0.02f, r * 1.4f, r * 0.62f}, color);
        DrawRectangleRec({c.x - r * 0.5f, c.y + r * 0.14f, r * 1.0f, r * 0.12f}, back);
        break;
    case ShopItemKind::CardReroll:
        DrawRectanglePro({c.x - r * 0.18f, c.y, r * 0.7f, r * 1.0f}, {r * 0.35f, r * 0.5f}, -14.0f, fade(color, 0.55f));
        DrawRectanglePro({c.x + r * 0.18f, c.y, r * 0.7f, r * 1.0f}, {r * 0.35f, r * 0.5f}, 12.0f, color);
        DrawCircleV({c.x + r * 0.2f, c.y}, r * 0.16f, back);
        break;
    }
    (void)weapon;
}

const char* statusLabel(ShopAvailability availability)
{
    switch (availability) {
    case ShopAvailability::Owned: return "POSSÉDÉ";
    case ShopAvailability::Maxed: return "MAX";
    case ShopAvailability::Full: return "PLEIN";
    case ShopAvailability::Unavailable: return "--";
    case ShopAvailability::Available: break;
    }
    return "";
}
}

void ShopScreen::open(int weapon, int gold)
{
    for (std::vector<int>& list : columns_) list.clear();
    const std::vector<ShopItem>& catalog = Shop::catalog();
    for (size_t i = 0; i < catalog.size(); ++i) {
        columns_[static_cast<size_t>(catalog[i].category)].push_back(static_cast<int>(i));
    }
    hover_.assign(catalog.size(), 0.0f);
    column_ = static_cast<int>(ShopCategory::Weapons);
    row_ = 0;
    weapon_ = weapon;
    requested_ = -1;
    flashItem_ = -1;
    flash_ = 0.0f;
    shakeItem_ = -1;
    shake_ = 0.0f;
    goldShown_ = static_cast<float>(gold);
    goldPulse_ = 0.0f;
    message_.clear();
    messageAge_ = 999.0f;
    time_ = 0.0f;
    lastMouse_ = ui::mouse();
}

void ShopScreen::focus(int item)
{
    for (int c = 0; c < kColumns; ++c) {
        const std::vector<int>& list = column(c);
        const auto it = std::find(list.begin(), list.end(), item);
        if (it == list.end()) continue;
        column_ = c;
        row_ = static_cast<int>(it - list.begin());
    }
}

void ShopScreen::notifyPurchase(bool success, const std::string& message)
{
    message_ = message;
    messageAge_ = 0.0f;
    messageColor_ = success ? kGold : kDenied;
    if (success) {
        flashItem_ = requested_;
        flash_ = kFlashDuration;
        goldPulse_ = 1.0f;
        // Une arme achetée devient la cible des améliorations.
        const ShopItem& item = Shop::catalog()[static_cast<size_t>(requested_)];
        if (item.kind == ShopItemKind::Weapon) weapon_ = item.weapon;
    } else {
        shakeItem_ = requested_;
        shake_ = kShakeDuration;
    }
    requested_ = -1;
}

Rectangle ShopScreen::columnRect(int index) const
{
    const float w = ui::width();
    const float h = ui::height();
    const float width = std::min(kMaxColumnWidth, (w - 2.0f * kMarginX - 2.0f * kColumnGap) / 3.0f);
    const float total = 3.0f * width + 2.0f * kColumnGap;
    const float x = (w - total) / 2.0f + static_cast<float>(index) * (width + kColumnGap);
    const float y = std::max(118.0f, (h - kColumnHeight) / 2.0f + 22.0f);
    return {x, y, width, kColumnHeight};
}

Rectangle ShopScreen::rowRect(int columnIndex, int row) const
{
    const Rectangle c = columnRect(columnIndex);
    return {c.x + kRowInset, c.y + kHeaderHeight + static_cast<float>(row) * (kRowHeight + kRowGap),
            c.width - 2.0f * kRowInset, kRowHeight};
}

Rectangle ShopScreen::selectorRect() const
{
    const Rectangle c = columnRect(static_cast<int>(ShopCategory::Upgrades));
    return {c.x + kRowInset, c.y + 44.0f, c.width - 2.0f * kRowInset, 30.0f};
}

void ShopScreen::cycleWeapon(const Arsenal& arsenal, int direction)
{
    const int count = static_cast<int>(Arsenal::catalog().size());
    for (int step = 1; step < count; ++step) {
        const int candidate = ((weapon_ + direction * step) % count + count) % count;
        if (arsenal.isUnlocked(candidate)) {
            weapon_ = candidate;
            return;
        }
    }
}

ShopScreen::Result ShopScreen::update(float dt, const Shop& shop, const Arsenal& arsenal, int gold)
{
    Result result;
    time_ += dt;
    messageAge_ += dt;
    flash_ = std::max(0.0f, flash_ - dt);
    shake_ = std::max(0.0f, shake_ - dt);
    goldPulse_ = std::max(0.0f, goldPulse_ - kGoldPulseDecay * dt);
    goldShown_ += (static_cast<float>(gold) - goldShown_) * (1.0f - std::exp(-kGoldCountSpeed * dt));
    if (std::fabs(goldShown_ - static_cast<float>(gold)) < 0.5f) goldShown_ = static_cast<float>(gold);
    if (!arsenal.isUnlocked(weapon_)) weapon_ = arsenal.current();

    for (size_t i = 0; i < hover_.size(); ++i) {
        const float target = static_cast<int>(i) == selectedItem() ? 1.0f : 0.0f;
        hover_[i] += (target - hover_[i]) * (1.0f - std::exp(-kHoverSpeed * dt));
    }

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_E)) {
        result.close = true;
        return result;
    }
    if (time_ < kInputDelay) return result;

    const int before = selectedItem();
    const auto pressed = [](int key) { return IsKeyPressed(key) || IsKeyPressedRepeat(key); };
    // raylib nomme les touches en QWERTY : W/A/S/D sont Z/Q/S/D sur un clavier AZERTY.
    if (pressed(KEY_UP) || pressed(KEY_W)) row_ = std::max(0, row_ - 1);
    if (pressed(KEY_DOWN) || pressed(KEY_S)) row_ = std::min(static_cast<int>(column(column_).size()) - 1, row_ + 1);
    if (pressed(KEY_LEFT) || pressed(KEY_A)) column_ = std::max(0, column_ - 1);
    if (pressed(KEY_RIGHT) || pressed(KEY_D)) column_ = std::min(kColumns - 1, column_ + 1);
    row_ = std::min(row_, static_cast<int>(column(column_).size()) - 1);
    if (IsKeyPressed(KEY_TAB)) cycleWeapon(arsenal, 1);
    for (int i = 0; i < static_cast<int>(Arsenal::catalog().size()); ++i) {
        if (IsKeyPressed(KEY_ONE + i) && arsenal.isUnlocked(i)) weapon_ = i;
    }

    // Souris : le survol déplace la sélection seulement si la souris bouge (le clavier garde la main sinon).
    const Vector2 mouse = ui::mouse();
    const bool mouseMoved = Vector2{mouse.x - lastMouse_.x, mouse.y - lastMouse_.y}.x != 0.0f &&
                                std::fabs(mouse.x - lastMouse_.x) > kMouseMoveThreshold ||
                            std::fabs(mouse.y - lastMouse_.y) > kMouseMoveThreshold;
    lastMouse_ = mouse;
    int hoveredColumn = -1;
    int hoveredRow = -1;
    for (int c = 0; c < kColumns; ++c) {
        for (int r = 0; r < static_cast<int>(column(c).size()); ++r) {
            if (CheckCollisionPointRec(mouse, rowRect(c, r))) {
                hoveredColumn = c;
                hoveredRow = r;
            }
        }
    }
    if (hoveredColumn >= 0 && (mouseMoved || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        column_ = hoveredColumn;
        row_ = hoveredRow;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const Rectangle s = selectorRect();
        if (CheckCollisionPointRec(mouse, s)) cycleWeapon(arsenal, mouse.x < s.x + s.width / 2.0f ? -1 : 1);
    }
    result.moved = selectedItem() != before;

    const bool confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE) ||
                         (hoveredColumn >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
    if (confirm) {
        requested_ = selectedItem();
        result.purchase = requested_;
    }
    (void)shop;
    return result;
}

void ShopScreen::drawHeader(int gold, float frozenLeft, float waveIn) const
{
    const float alpha = std::min(1.0f, time_ / kDimDuration);
    const Rectangle first = columnRect(0);
    const Rectangle last = columnRect(kColumns - 1);
    const float cx = ui::width() / 2.0f;
    const float top = first.y - 104.0f;

    drawCentered("LE MARCHAND", cx, top, 50.0f, fade(kTitle, alpha));
    drawCentered("Achète, améliore, survis.", cx, top + 58.0f, 20.0f, fade(kTextDim, alpha));

    // À gauche : le temps figé, puis le compte à rebours de la vague quand il est épuisé.
    if (frozenLeft > 0.0f) {
        drawText("TEMPS FIGÉ", first.x, top + 8.0f, 16.0f, fade(kTextDim, alpha));
        drawText(TextFormat("%d s", static_cast<int>(std::ceil(frozenLeft))), first.x, top + 28.0f, 36.0f,
                 fade(kText, alpha));
    } else {
        const float blink = 0.6f + 0.4f * std::sin(time_ * 8.0f);
        drawText("VAGUE DANS", first.x, top + 8.0f, 16.0f, fade(kDenied, alpha));
        drawText(TextFormat("%d s", static_cast<int>(std::ceil(waveIn))), first.x, top + 28.0f, 36.0f,
                 fade(kDenied, alpha * blink));
    }

    // À droite : l'or restant, en grand.
    const float right = last.x + last.width;
    const float size = 46.0f * (1.0f + 0.12f * goldPulse_);
    const char* amount = TextFormat("%d", static_cast<int>(std::lround(goldShown_)));
    const float textWidth = measure(amount, size).x;
    const Color goldColor = mix(kGold, WHITE, goldPulse_ * 0.6f);
    drawRight("OR", right, top + 8.0f, 16.0f, fade(kTextDim, alpha));
    drawText(amount, right - textWidth + 2.0f, top + 30.0f, size, fade(BLACK, alpha * 0.6f));
    drawText(amount, right - textWidth, top + 28.0f, size, fade(goldColor, alpha));
    drawCoin({right - textWidth - 26.0f, top + 28.0f + size * 0.45f}, 17.0f * (1.0f + 0.2f * goldPulse_));
    (void)gold;
}

void ShopScreen::drawColumn(int index, const Shop& shop, const Arsenal& arsenal, int) const
{
    const float t = std::clamp((time_ - kIntroColumnDelay * static_cast<float>(index)) / kIntroDuration, 0.0f, 1.0f);
    const float alpha = std::min(1.0f, t * 2.0f);
    const Rectangle c = columnRect(index);
    const Color color = columnColor(index);
    const bool active = index == column_;

    DrawRectangleRounded({c.x + 6.0f, c.y + 10.0f, c.width, c.height}, kRoundness * 0.5f, kSegments,
                         fade(BLACK, alpha * 0.45f));
    DrawRectangleRounded(c, kRoundness * 0.5f, kSegments, fade(kPanel, alpha));
    DrawRectangleGradientV(static_cast<int>(c.x + 4.0f), static_cast<int>(c.y + 4.0f), static_cast<int>(c.width - 8.0f),
                           static_cast<int>(kHeaderHeight), fade(mix(kPanel, color, 0.16f), alpha), fade(kPanel, alpha));
    DrawRectangleRoundedLinesEx(c, kRoundness * 0.5f, kSegments, active ? 2.0f : 1.0f,
                                fade(color, alpha * (active ? 0.8f : 0.35f)));

    drawText(kColumnTitles[index], c.x + 18.0f, c.y + 14.0f, 24.0f, fade(color, alpha));
    if (static_cast<ShopCategory>(index) == ShopCategory::Upgrades) {
        // Sélecteur de l'arme à améliorer.
        const Rectangle s = selectorRect();
        DrawRectangleRounded(s, 0.4f, kSegments, fade(kRowBack, alpha));
        DrawRectangleRoundedLinesEx(s, 0.4f, kSegments, 1.0f, fade(color, alpha * 0.6f));
        const float mid = s.y + s.height / 2.0f;
        DrawTriangle({s.x + 22.0f, mid - 7.0f}, {s.x + 12.0f, mid}, {s.x + 22.0f, mid + 7.0f}, fade(color, alpha));
        DrawTriangle({s.x + s.width - 22.0f, mid - 7.0f}, {s.x + s.width - 22.0f, mid + 7.0f},
                     {s.x + s.width - 12.0f, mid}, fade(color, alpha));
        const char* name = Arsenal::catalog()[static_cast<size_t>(weapon_)].name.c_str();
        drawCentered(TextFormat("%d  %s", weapon_ + 1, name), s.x + s.width / 2.0f, mid - 9.0f, 18.0f,
                     fade(kText, alpha));
        drawRight("Tab / 1-4", c.x + c.width - 18.0f, c.y + 20.0f, 14.0f, fade(kTextDim, alpha));
    } else {
        drawText(kColumnHints[index], c.x + 18.0f, c.y + 48.0f, 16.0f, fade(kTextDim, alpha));
    }

    for (int r = 0; r < static_cast<int>(column(index).size()); ++r) drawRow(index, r, shop);
    (void)arsenal;
}

void ShopScreen::drawRow(int columnIndex, int row, const Shop& shop) const
{
    const int itemIndex = column(columnIndex)[static_cast<size_t>(row)];
    const ShopItem& item = Shop::catalog()[static_cast<size_t>(itemIndex)];
    const ShopOffer offer = shop.offer(itemIndex, weapon_);
    const Color color = columnColor(columnIndex);
    const float hover = hover_[static_cast<size_t>(itemIndex)];
    const bool selected = itemIndex == selectedItem();
    const bool available = offer.availability == ShopAvailability::Available;

    // Arrivée en glissant, décalée par colonne et par ligne ; secousse si l'achat est refusé.
    const float delay = kIntroColumnDelay * static_cast<float>(columnIndex) + kIntroRowDelay * static_cast<float>(row);
    const float t = std::clamp((time_ - delay) / kIntroDuration, 0.0f, 1.0f);
    const float alpha = std::min(1.0f, t * 2.5f);
    if (alpha <= 0.01f) return;
    Rectangle r = rowRect(columnIndex, row);
    r.y += (1.0f - easeOutBack(t)) * 60.0f;
    r.x += hover * 4.0f;
    if (itemIndex == shakeItem_ && shake_ > 0.0f) {
        r.x += std::sin(shake_ * 70.0f) * kShakeAmplitude * (shake_ / kShakeDuration);
    }

    const Color back = mix(kRowBack, color, 0.05f + 0.1f * hover);
    DrawRectangleRounded(r, kRoundness, kSegments, fade(back, alpha));
    if (hover > 0.01f) {
        DrawRectangleRoundedLinesEx(r, kRoundness, kSegments, 1.0f + 1.5f * hover, fade(color, alpha * hover));
    }
    // Barre de couleur sur le bord gauche de la ligne choisie.
    DrawRectangleRounded({r.x, r.y + 14.0f, 4.0f, r.height - 28.0f}, 1.0f, 4, fade(color, alpha * (0.25f + 0.75f * hover)));

    const float dimmed = available ? 1.0f : 0.6f;
    const Vector2 icon = {r.x + 44.0f, r.y + 44.0f};
    const Color iconBack = mix(kPanel, color, 0.12f);
    DrawCircleV(icon, kIconRadius + 3.0f, fade(mix(kPanel, color, 0.5f + 0.4f * hover), alpha * dimmed));
    DrawCircleV(icon, kIconRadius, fade(iconBack, alpha));
    drawIcon(item, weapon_, icon, kIconRadius * 0.72f, fade(color, alpha * dimmed), fade(iconBack, alpha));

    const float left = r.x + kTextLeft;
    const float right = r.x + r.width - 14.0f;
    drawText(item.name, left, r.y + 14.0f, 22.0f, fade(available ? kText : kTextDim, alpha));
    drawText(item.tagline, left, r.y + 42.0f, 15.0f, fade(kTextDim, alpha));
    float y = r.y + 64.0f;
    for (const std::string& line : wrap(offer.details, 16.0f, right - left)) {
        drawText(line.c_str(), left, y, 16.0f, fade(mix(color, kText, 0.45f), alpha * dimmed));
        y += 19.0f;
    }

    // Prix (rouge si trop cher) ou état, en haut à droite.
    if (available) {
        const char* price = TextFormat("%d", offer.price);
        const float width = measure(price, 24.0f).x;
        drawText(price, right - width, r.y + 12.0f, 24.0f, fade(offer.affordable ? kGold : kDenied, alpha));
        drawCoin({right - width - 16.0f, r.y + 23.0f}, 9.0f);
    } else {
        const Color status = offer.availability == ShopAvailability::Owned ? kOwned
                           : offer.availability == ShopAvailability::Maxed ? kGold
                                                                          : kTextDim;
        drawRight(statusLabel(offer.availability), right, r.y + 16.0f, 18.0f, fade(status, alpha));
    }

    // Niveau : pastilles (la prochaine clignote sur la ligne choisie).
    const float bottom = r.y + r.height - 22.0f;
    if (offer.maxLevel > 0) {
        for (int i = 0; i < offer.maxLevel; ++i) {
            const Vector2 p = {left + 7.0f + static_cast<float>(i) * 20.0f, bottom + 6.0f};
            Color pip = kPipEmpty;
            if (i < offer.level) pip = color;
            if (i == offer.level && selected && available) pip = mix(kPipEmpty, color, 0.5f + 0.5f * std::sin(time_ * 8.0f));
            DrawCircleV(p, 6.5f, fade(pip, alpha));
        }
        const char* level = item.kind == ShopItemKind::Upgrade ? TextFormat("NIV %d/%d", offer.level, offer.maxLevel)
                                                               : TextFormat("x%d", offer.level);
        drawText(level, left + 7.0f + static_cast<float>(offer.maxLevel) * 20.0f, bottom - 1.0f, 14.0f,
                 fade(kTextDim, alpha));
    }
    if (selected && available) {
        const char* hint = offer.affordable ? "ENTRÉE : acheter" : "Pas assez d'or";
        drawRight(hint, right, bottom - 1.0f, 14.0f, fade(offer.affordable ? kText : kDenied, alpha * 0.85f));
    }

    if (itemIndex == flashItem_ && flash_ > 0.0f) {
        DrawRectangleRounded(r, kRoundness, kSegments, fade(kGold, 0.45f * flash_ / kFlashDuration));
    }
}

void ShopScreen::draw(const Shop& shop, const Arsenal& arsenal, int gold, float frozenLeft, float waveIn) const
{
    const int w = ui::screenWidth();
    const int h = ui::screenHeight();
    DrawRectangle(0, 0, w, h, fade(kDim, std::min(1.0f, time_ / kDimDuration)));

    drawHeader(gold, frozenLeft, waveIn);
    for (int c = 0; c < kColumns; ++c) drawColumn(c, shop, arsenal, gold);

    const Rectangle first = columnRect(0);
    const float cx = static_cast<float>(w) / 2.0f;
    const float below = first.y + first.height;
    if (messageAge_ < kMessageDuration) {
        const float a = std::clamp((kMessageDuration - messageAge_) / 0.4f, 0.0f, 1.0f);
        drawCentered(message_.c_str(), cx, below + 12.0f, 22.0f, fade(messageColor_, a));
    }
    drawCentered("Flèches / ZQSD : choisir    Entrée / clic : acheter    Tab / 1-4 : arme    E / Échap : fermer", cx,
                 static_cast<float>(h) - 30.0f, 16.0f, fade(kTextDim, std::min(1.0f, time_ / kDimDuration)));

    const Vector2 m = ui::mouse();
    DrawCircleV(m, 6.0f, {255, 230, 170, 230});
    DrawCircleLinesV(m, 10.0f, {255, 230, 170, 160});
}
