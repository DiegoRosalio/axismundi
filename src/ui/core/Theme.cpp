#include "ui/core/Theme.hpp"

namespace linalg::ui {

namespace {

const Theme kDark = {
    true,
    // Surfaces
    {22, 24, 30, 255},     // background
    {31, 34, 42, 255},     // panel
    {40, 44, 54, 255},     // panelAlt
    {50, 55, 68, 255},     // hover
    {60, 66, 82, 255},     // pressed
    {56, 61, 74, 255},     // border
    {80, 87, 104, 255},    // borderStrong
    {8, 9, 12, 150},       // overlay
    {0, 0, 0, 110},        // shadow
    // Text
    {231, 234, 241, 255},  // text
    {152, 160, 176, 255},  // textMuted
    {104, 111, 127, 255},  // textFaint
    // Accents
    {104, 142, 255, 255},  // accent
    {130, 163, 255, 255},  // accentHover
    {104, 142, 255, 48},   // accentSoft
    {255, 255, 255, 255},  // onAccent
    {240, 92, 92, 255},    // danger
    {84, 199, 132, 255},   // success
    {240, 183, 70, 255},   // warning
    // 3D scene
    {255, 255, 255, 34},   // grid
    {18, 20, 26, 215},     // labelBg
    {238, 86, 86, 255},    // axisX
    {92, 204, 120, 255},   // axisY
    {88, 146, 255, 255},   // axisZ
    {255, 152, 64, 255},   // basisX
    {182, 240, 92, 255},   // basisY
    {110, 214, 255, 255},  // basisZ
    {255, 214, 74, 255},   // vecA
    {236, 100, 206, 255},  // vecB
    {255, 152, 68, 255},   // vecSum
    {86, 204, 255, 255},   // vecCross
    {255, 236, 160, 255},  // vecMA
    {255, 176, 226, 255},  // vecMB
    {178, 132, 255, 255},  // eigen
    {120, 170, 255, 255},  // curve
    {104, 160, 255, 225},  // surface
    {255, 214, 74, 255},   // z1
    {236, 100, 206, 255},  // z2
    {255, 152, 68, 255},   // zProduct
    {120, 170, 255, 255},  // euler
    {90, 214, 230, 255},   // helix
};

const Theme kLight = {
    false,
    // Surfaces
    {238, 241, 246, 255},  // background
    {255, 255, 255, 255},  // panel
    {242, 244, 248, 255},  // panelAlt
    {230, 234, 241, 255},  // hover
    {216, 222, 232, 255},  // pressed
    {214, 219, 228, 255},  // border
    {178, 186, 200, 255},  // borderStrong
    {20, 24, 32, 110},     // overlay
    {16, 24, 40, 50},      // shadow
    // Text
    {27, 31, 40, 255},     // text
    {96, 105, 121, 255},   // textMuted
    {140, 148, 162, 255},  // textFaint
    // Accents
    {44, 98, 232, 255},    // accent
    {30, 80, 205, 255},    // accentHover
    {44, 98, 232, 36},     // accentSoft
    {255, 255, 255, 255},  // onAccent
    {214, 58, 58, 255},    // danger
    {30, 150, 90, 255},    // success
    {196, 132, 10, 255},   // warning
    // 3D scene
    {0, 0, 0, 32},         // grid
    {255, 255, 255, 225},  // labelBg
    {214, 52, 52, 255},    // axisX
    {34, 156, 70, 255},    // axisY
    {40, 100, 225, 255},   // axisZ
    {232, 116, 20, 255},   // basisX
    {110, 175, 20, 255},   // basisY
    {20, 160, 210, 255},   // basisZ
    {214, 160, 0, 255},    // vecA
    {196, 40, 160, 255},   // vecB
    {232, 110, 20, 255},   // vecSum
    {0, 140, 210, 255},    // vecCross
    {176, 136, 20, 255},   // vecMA
    {204, 96, 170, 255},   // vecMB
    {120, 64, 220, 255},   // eigen
    {36, 84, 200, 255},    // curve
    {90, 150, 240, 230},   // surface
    {214, 160, 0, 255},    // z1
    {196, 40, 160, 255},   // z2
    {232, 110, 20, 255},   // zProduct
    {36, 84, 200, 255},    // euler
    {0, 150, 170, 255},    // helix
};

ThemeKind g_kind = ThemeKind::Dark;

} // namespace

const Theme& theme() {
    return g_kind == ThemeKind::Dark ? kDark : kLight;
}

ThemeKind themeKind() {
    return g_kind;
}

void setTheme(ThemeKind kind) {
    g_kind = kind;
}

void toggleTheme() {
    g_kind = (g_kind == ThemeKind::Dark) ? ThemeKind::Light : ThemeKind::Dark;
}

Color contrastText(Color bg) {
    // Perceived luminance (Rec. 601)
    float lum = 0.299f * bg.r + 0.587f * bg.g + 0.114f * bg.b;
    return lum > 150.0f ? Color{20, 22, 28, 255} : Color{255, 255, 255, 255};
}

} // namespace linalg::ui
