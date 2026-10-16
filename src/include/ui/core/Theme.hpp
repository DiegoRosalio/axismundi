#pragma once

#include <raylib.h>

namespace linalg::ui {

enum class ThemeKind { Dark, Light };

// Color tokens for the whole app: UI surfaces and text, plus the colors of
// 3D objects so they read well on either background.
struct Theme {
    bool dark;

    // Surfaces
    Color background;   // 3D viewport clear color
    Color panel;        // side bar, cards, floating panels
    Color panelAlt;     // input fields, inset areas
    Color hover;        // hovered controls
    Color pressed;      // pressed controls
    Color border;
    Color borderStrong;
    Color overlay;      // dimming behind modal overlays
    Color shadow;

    // Text
    Color text;
    Color textMuted;
    Color textFaint;

    // Accents
    Color accent;
    Color accentHover;
    Color accentSoft;   // selected backgrounds
    Color onAccent;     // text on accent
    Color danger;
    Color success;
    Color warning;

    // 3D scene
    Color grid;
    Color labelBg;
    Color axisX, axisY, axisZ;
    Color basisX, basisY, basisZ;  // transformed basis
    Color vecA, vecB, vecSum, vecCross, vecMA, vecMB;
    Color eigen;
    Color curve;                   // plot curves without heat map
    Color surface;                 // plot surface without heat map
    Color z1, z2, zProduct, euler, helix;
};

const Theme& theme();
ThemeKind themeKind();
void setTheme(ThemeKind kind);
void toggleTheme();

// Readable text color (dark or light) for a given background color
Color contrastText(Color background);

} // namespace linalg::ui
