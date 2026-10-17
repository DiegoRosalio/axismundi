#pragma once

#include <raylib.h>
#include <string>

// Resolution-independent UI foundation.
//
// All layout is written in "design units" for a 1600x900 reference window.
// px() converts design units to pixels using the current scale, which follows
// the window size (and an optional user zoom). Fonts are rasterized at the
// exact pixel size they are drawn at, and rebuilt when the scale changes.
namespace linalg::ui {

enum class FontRole {
    Body,        // default UI text
    BodyStrong,  // emphasized text, button labels
    Small,       // secondary text, hints
    Caption,     // section headers (shown in capitals)
    Title,       // panel titles
    Mono,        // numbers, matrices, worked calculations
    Label3D,     // labels next to 3D objects
    Display,     // large headings (help, tutorial)
};

enum class Align { Left, Center, Right };

constexpr float kDesignWidth = 1600.0f;
constexpr float kDesignHeight = 900.0f;

// --- Lifecycle (call after InitWindow / once per frame / before CloseWindow)
void init();
void beginFrame();
void shutdown();

// --- Scale -------------------------------------------------------------------
float scale();
float zoom();
void setZoom(float zoom);   // user multiplier, clamped to [0.7, 2.0]
void zoomIn();
void zoomOut();

inline float px(float design) { return design * scale(); }
// Integer-pixel version, for line widths and borders that must stay crisp
float pxi(float design);
Rectangle rect(float x, float y, float w, float h);  // design units -> pixels
Rectangle inset(Rectangle r, float designPadding);
Rectangle snap(Rectangle r);

// --- Text --------------------------------------------------------------------
const Font& font(FontRole role);
float fontSize(FontRole role);   // pixels
Vector2 measure(const char* text, FontRole role);
float textWidth(const char* text, FontRole role);
void text(const char* text, float x, float y, FontRole role, Color color);
// Draw inside a rectangle, vertically centered, with horizontal alignment
void textIn(Rectangle r, const char* text, FontRole role, Color color, Align align = Align::Left);
// Word-wrapped text; returns the height used (draw = false only measures)
float textWrapped(Rectangle r, const char* text, FontRole role, Color color, bool draw = true,
                  float lineSpacing = 1.35f);
std::string ellipsize(const char* text, FontRole role, float maxWidth);
std::string upper(const char* text);

// --- Primitives (pixel coordinates) ------------------------------------------
void fillRounded(Rectangle r, float radius, Color color);
void strokeRounded(Rectangle r, float radius, float thickness, Color color);
void shadow(Rectangle r, float radius, float spread);
void line(Vector2 a, Vector2 b, float thickness, Color color);

// --- Interaction -------------------------------------------------------------
bool canInteract();               // false while the GUI is locked (camera mode, overlays)
// Widgets only react inside the clip rectangle (set by scroll containers)
void setClip(Rectangle r);
void clearClip();
bool mouseInClip();
// Mouse buttons with presses/releases latched per frame: a touchpad tap can press and release
// within one frame, which raylib's per-frame sampling misses. Use these instead of
// IsMouseButtonPressed/Released. installClickLatch() after InitWindow.
void installClickLatch();
bool mousePressed(int button);
bool mouseReleased(int button);
bool hovered(Rectangle r);        // mouse inside and interaction allowed
bool clicked(Rectangle r);        // left button pressed inside this frame
bool rightClicked(Rectangle r);

// --- Anchors -----------------------------------------------------------------
// Named screen rectangles registered while drawing (e.g. "matrix.grid"), so the
// tutorial can point at a control. findAnchor() returns last frame's rectangle.
void anchor(const char* id, Rectangle r);
bool findAnchor(const std::string& id, Rectangle& out);
Rectangle unite(Rectangle a, Rectangle b);

// --- Tooltips ----------------------------------------------------------------
// Register a tooltip for a rectangle (call while drawing, after the widget).
// The hovered one is shown after a short delay by drawTooltip().
void tooltip(Rectangle r, const char* body, const char* title = nullptr);
void drawTooltip(bool enabled);

} // namespace linalg::ui
