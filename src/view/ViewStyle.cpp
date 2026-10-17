#include "view/ViewStyle.hpp"

namespace linalg::view {

namespace {

struct StyleInfo {
    StyleId id;
    const char* key;
    const char* name;
    const char* description;
};

const StyleInfo kStyles[kStyleCount] = {
    {StyleId::Colorful, "colorful", "Colorful", "Theme colors, heat maps and shaded surfaces."},
    {StyleId::Paper, "paper", "Paper",
     "Black and white on white: line styles and grays, exactly like the LaTeX export."},
    {StyleId::HighContrast, "contrast", "High contrast",
     "Thick lines, big labels and strong colors, for projectors and slides."},
    {StyleId::Chalkboard, "chalkboard", "Chalkboard", "A green board with chalk-white lines and pastel colors."},
};

const StyleInfo& info(StyleId id) {
    for (const auto& s : kStyles)
        if (s.id == id) return s;
    return kStyles[0];
}

ViewStyle g_active = {};
bool g_activeSet = false;

} // namespace

const char* styleKey(StyleId id) { return info(id).key; }
const char* styleName(StyleId id) { return info(id).name; }
const char* styleDescription(StyleId id) { return info(id).description; }

StyleId styleFromKey(const std::string& key, StyleId fallback) {
    for (const auto& s : kStyles)
        if (key == s.key) return s.id;
    return fallback;
}

ViewStyle styleFor(StyleId id, const ui::Theme& t) {
    // Colorful: today's theme colors; the other styles start from it and change what they need
    ViewStyle s;
    s.id = id;
    s.background = t.background;
    s.grid = t.grid;
    s.axisX = t.axisX, s.axisY = t.axisY, s.axisZ = t.axisZ;
    s.basisX = t.basisX, s.basisY = t.basisY, s.basisZ = t.basisZ;
    s.text = t.text, s.muted = t.textMuted, s.faint = t.textFaint;
    s.vectors = {t.vecA, t.vecB, t.vecSum, t.vecCross, t.vecMA, t.vecMB, t.eigen, t.warning};
    s.plots = {t.curve, t.success, t.z2, t.warning, t.vecSum};
    s.complexes = {t.z1, t.z2, t.euler, t.helix, t.vecSum, t.success};
    s.eigen = t.eigen, s.product = t.zProduct, s.z1 = t.z1, s.z2 = t.z2, s.helix = t.helix;
    s.curve = t.curve, s.surface = t.surface;
    s.wire = Fade(t.dark ? BLACK : WHITE, 0.28f);
    s.labelBg = t.labelBg;

    switch (id) {
        case StyleId::Colorful:
            break;
        case StyleId::Paper: {
            // Drawn by the print path; these colors cover what that path takes from the style
            const Color ink = {20, 20, 20, 255};
            s.print = true;
            s.background = WHITE;
            s.grid = {120, 120, 120, 70};
            s.axisX = s.axisY = s.axisZ = ink;
            s.basisX = s.basisY = s.basisZ = ink;
            s.text = ink, s.muted = {110, 110, 110, 255}, s.faint = {160, 160, 160, 255};
            s.labelBg = {255, 255, 255, 220};
            s.wire = {0, 0, 0, 220};
            s.smoothDefault = false;
            break;
        }
        case StyleId::HighContrast:
            s.lineScale = 1.8f;
            s.labelLarge = true;
            if (t.dark) {
                s.background = {6, 6, 8, 255};
                s.grid = {205, 205, 215, 70};
                s.axisX = {255, 85, 85, 255}, s.axisY = {90, 235, 115, 255}, s.axisZ = {95, 155, 255, 255};
                s.text = WHITE, s.muted = {210, 210, 210, 255}, s.faint = {170, 170, 170, 255};
                s.vectors = {{255, 214, 0, 255},  {0, 205, 255, 255},  {255, 70, 200, 255}, {125, 255, 80, 255},
                             {255, 140, 0, 255},  {185, 125, 255, 255}, {255, 255, 255, 255}, {255, 85, 85, 255}};
                s.labelBg = {0, 0, 0, 220};
                s.wire = {0, 0, 0, 110};
            } else {
                s.background = WHITE;
                s.grid = {30, 30, 40, 60};
                s.axisX = {210, 0, 0, 255}, s.axisY = {0, 150, 40, 255}, s.axisZ = {0, 70, 220, 255};
                s.text = BLACK, s.muted = {60, 60, 60, 255}, s.faint = {110, 110, 110, 255};
                s.vectors = {{0, 90, 205, 255},  {215, 0, 90, 255},  {0, 140, 60, 255}, {150, 65, 0, 255},
                             {125, 0, 185, 255}, {230, 110, 0, 255}, {0, 0, 0, 255},    {200, 0, 0, 255}};
                s.labelBg = {255, 255, 255, 235};
                s.wire = {255, 255, 255, 120};
            }
            s.basisX = s.axisX, s.basisY = s.axisY, s.basisZ = s.axisZ;
            s.plots = {s.vectors[1], s.vectors[3], s.vectors[2], s.vectors[0], s.vectors[4]};
            s.complexes = {s.vectors[0], s.vectors[1], s.vectors[2], s.vectors[3], s.vectors[4], s.vectors[5]};
            s.eigen = s.vectors[5], s.product = s.vectors[4], s.z1 = s.vectors[0], s.z2 = s.vectors[1];
            s.helix = s.vectors[3];
            s.curve = s.vectors[1], s.surface = s.vectors[1];
            break;
        case StyleId::Chalkboard: {
            const Color chalk = {245, 245, 235, 255};
            s.background = {47, 79, 62, 255};
            s.grid = {235, 240, 225, 42};
            s.axisX = {250, 175, 165, 255}, s.axisY = {195, 238, 175, 255}, s.axisZ = {175, 205, 250, 255};
            s.basisX = s.axisX, s.basisY = s.axisY, s.basisZ = s.axisZ;
            s.text = chalk, s.muted = {205, 215, 200, 255}, s.faint = {205, 215, 200, 150};
            s.vectors = {{255, 240, 170, 255}, {170, 220, 255, 255}, {255, 190, 210, 255}, {200, 255, 190, 255},
                         {255, 210, 160, 255}, {220, 200, 255, 255}, chalk,                {255, 170, 160, 255}};
            s.plots = {chalk, s.vectors[3], s.vectors[1], s.vectors[0], s.vectors[2]};
            s.complexes = {s.vectors[0], s.vectors[1], s.vectors[2], s.vectors[3], s.vectors[4], s.vectors[5]};
            s.eigen = s.vectors[5], s.product = s.vectors[4], s.z1 = s.vectors[0], s.z2 = s.vectors[1];
            s.helix = s.vectors[3];
            s.curve = chalk, s.surface = {225, 230, 215, 255};
            s.wire = {40, 60, 50, 120};
            s.labelBg = {28, 48, 38, 205};
            s.lineScale = 1.25f;
            s.heatSoften = 0.45f;
            s.smoothDefault = false;
            break;
        }
    }
    return s;
}

const ViewStyle& activeStyle() {
    if (!g_activeSet) {
        g_active = styleFor(StyleId::Colorful, ui::theme());
        g_activeSet = true;
    }
    return g_active;
}

void setActiveStyle(const ViewStyle& style) {
    g_active = style;
    g_activeSet = true;
}

} // namespace linalg::view
