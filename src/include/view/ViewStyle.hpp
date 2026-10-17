#pragma once

#include "ui/core/Theme.hpp"
#include <raylib.h>
#include <string>
#include <vector>

namespace linalg::view {

// How the 3D view is drawn. A style only changes the view; the panels keep the dark/light theme.
enum class StyleId { Colorful, Paper, HighContrast, Chalkboard };
constexpr int kStyleCount = 4;

const char* styleKey(StyleId id);   // file key: colorful, paper, contrast, chalkboard
StyleId styleFromKey(const std::string& key, StyleId fallback = StyleId::Colorful);
const char* styleName(StyleId id);  // "High contrast"
const char* styleDescription(StyleId id);

// Everything the view needs to draw in one style: a new style is data, not a new drawing path
struct ViewStyle {
    StyleId id = StyleId::Colorful;
    bool print = false;  // Paper: the black-and-white print path (the LaTeX export's styles)

    Color background, grid;
    Color axisX, axisY, axisZ;
    Color basisX, basisY, basisZ;  // a matrix's moved basis
    Color text, muted, faint;      // origin dot and matrix labels, projections, unit circle
    std::vector<Color> vectors, plots, complexes;  // palettes (by the object's palette index)
    Color eigen, product, z1, z2, helix;
    Color curve, surface;  // plots without a palette color
    Color wire;            // surface mesh lines
    Color labelBg;         // pill behind 3D labels
    float heatSoften = 0.0f;    // heat maps blended toward `text` (0 = the plain rainbow)
    float lineScale = 1.0f;     // arrows, basis, axes and curve tubes
    bool labelLarge = false;    // bigger 3D labels
    bool smoothDefault = true;  // surfaces: smooth or faceted unless Settings says otherwise
};

ViewStyle styleFor(StyleId id, const ui::Theme& theme);

// The style the view draws with (set by App each frame; read by SceneView and the objects list)
const ViewStyle& activeStyle();
void setActiveStyle(const ViewStyle& style);

} // namespace linalg::view
