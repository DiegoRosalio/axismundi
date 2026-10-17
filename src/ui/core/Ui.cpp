// GLFW is available when raylib is built from source (tools/dev/raylib.sh): raylib's own
// static library contains it. With a system raylib the click latch is simply off.
#if __has_include("external/glfw/include/GLFW/glfw3.h")
#include "external/glfw/include/GLFW/glfw3.h"
#define AXIS_CLICK_LATCH 1
#endif
#include "ui/core/Ui.hpp"
#include "ui/core/Theme.hpp"

#include <raygui.h>
#include <algorithm>
#include <map>
#include <cctype>
#include <cmath>
#include <cstring>

#include "../generated/fonts.inc"

namespace linalg::ui {

namespace {

struct RoleSpec {
    const unsigned char* data;
    int size;
    float designSize;
    float minPixels;  // legibility floor on small windows
};

constexpr int kRoleCount = 8;

RoleSpec roleSpec(FontRole role) {
    switch (role) {
        case FontRole::Body:       return {kFontInterRegular, kFontInterRegularSize, 16.0f, 13.0f};
        case FontRole::BodyStrong: return {kFontInterSemiBold, kFontInterSemiBoldSize, 16.0f, 13.0f};
        case FontRole::Small:      return {kFontInterRegular, kFontInterRegularSize, 14.0f, 11.5f};
        case FontRole::Caption:    return {kFontInterSemiBold, kFontInterSemiBoldSize, 12.5f, 10.5f};
        case FontRole::Title:      return {kFontInterSemiBold, kFontInterSemiBoldSize, 18.0f, 14.0f};
        case FontRole::Mono:       return {kFontMonoRegular, kFontMonoRegularSize, 15.0f, 12.0f};
        case FontRole::Label3D:    return {kFontInterSemiBold, kFontInterSemiBoldSize, 14.5f, 12.0f};
        case FontRole::Display:    return {kFontInterSemiBold, kFontInterSemiBoldSize, 26.0f, 20.0f};
    }
    return {kFontInterRegular, kFontInterRegularSize, 16.0f, 13.0f};
}

struct State {
    bool initialized = false;
    float scale = 1.0f;
    float zoom = 1.0f;
    float fontScale = 0.0f;       // scale the atlases were built for
    double pendingSince = -1.0;   // debounce rebuilds while the window is resized
    Font fonts[kRoleCount] = {};
    bool fontLoaded[kRoleCount] = {};
    ThemeKind styledTheme = ThemeKind::Dark;
    float styledScale = 0.0f;
};

State g;

std::map<std::string, Rectangle> g_anchors;      // being registered this frame
std::map<std::string, Rectangle> g_lastAnchors;  // complete set from last frame

int roleIndex(FontRole role) { return static_cast<int>(role); }

void buildFonts(float s) {
    for (int i = 0; i < kRoleCount; i++) {
        RoleSpec spec = roleSpec(static_cast<FontRole>(i));
        int pixelSize = static_cast<int>(std::lround(std::max(spec.minPixels, spec.designSize * s)));
        Font f = LoadFontFromMemory(".ttf", spec.data, spec.size, pixelSize,
                                    const_cast<int*>(kUiCodepoints), kUiCodepointCount);
        if (f.texture.id == 0) continue;  // keep the previous atlas
        SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
        if (g.fontLoaded[i]) UnloadFont(g.fonts[i]);
        g.fonts[i] = f;
        g.fontLoaded[i] = true;
    }
    g.fontScale = s;
}

int colorInt(Color c) { return ColorToInt(c); }

void setControlColors(int control, Color base, Color border, Color text) {
    const Theme& t = theme();
    GuiSetStyle(control, BASE_COLOR_NORMAL, colorInt(base));
    GuiSetStyle(control, BORDER_COLOR_NORMAL, colorInt(border));
    GuiSetStyle(control, TEXT_COLOR_NORMAL, colorInt(text));
    GuiSetStyle(control, BASE_COLOR_FOCUSED, colorInt(t.hover));
    GuiSetStyle(control, BORDER_COLOR_FOCUSED, colorInt(t.borderStrong));
    GuiSetStyle(control, TEXT_COLOR_FOCUSED, colorInt(t.text));
    GuiSetStyle(control, BASE_COLOR_PRESSED, colorInt(t.panelAlt));
    GuiSetStyle(control, BORDER_COLOR_PRESSED, colorInt(t.accent));
    GuiSetStyle(control, TEXT_COLOR_PRESSED, colorInt(t.text));
    GuiSetStyle(control, BASE_COLOR_DISABLED, colorInt(t.panelAlt));
    GuiSetStyle(control, BORDER_COLOR_DISABLED, colorInt(t.border));
    GuiSetStyle(control, TEXT_COLOR_DISABLED, colorInt(t.textFaint));
}

// Map the theme and scale onto raygui, used for text boxes and the color picker
void applyGuiStyle() {
    const Theme& t = theme();
    GuiSetFont(g.fonts[roleIndex(FontRole::Body)]);
    GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(fontSize(FontRole::Body)));
    GuiSetStyle(DEFAULT, TEXT_SPACING, 0);
    GuiSetStyle(DEFAULT, BORDER_WIDTH, static_cast<int>(pxi(1.0f)));
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, colorInt(t.panel));
    GuiSetStyle(DEFAULT, LINE_COLOR, colorInt(t.border));
    setControlColors(DEFAULT, t.panelAlt, t.border, t.text);

    // Text boxes: ui::textField draws the rounded box itself, so raygui only
    // draws text and cursor (no border, transparent fill; cursor = accent)
    const Color clear = {0, 0, 0, 0};
    setControlColors(TEXTBOX, clear, clear, t.text);
    GuiSetStyle(TEXTBOX, BASE_COLOR_FOCUSED, colorInt(clear));
    GuiSetStyle(TEXTBOX, BASE_COLOR_PRESSED, colorInt(clear));
    GuiSetStyle(TEXTBOX, BASE_COLOR_DISABLED, colorInt(clear));
    GuiSetStyle(TEXTBOX, BORDER_COLOR_PRESSED, colorInt(t.accent));
    GuiSetStyle(TEXTBOX, TEXT_COLOR_DISABLED, colorInt(t.textMuted));
    GuiSetStyle(TEXTBOX, TEXT_PADDING, static_cast<int>(px(9.0f)));
    GuiSetStyle(TEXTBOX, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(TEXTBOX, BORDER_WIDTH, 0);
    GuiSetStyle(DEFAULT, TEXT_LINE_SPACING, static_cast<int>(px(18.0f)));

    setControlColors(COLORPICKER, t.panelAlt, t.border, t.text);
    GuiSetStyle(COLORPICKER, COLOR_SELECTOR_SIZE, static_cast<int>(px(8.0f)));
    GuiSetStyle(COLORPICKER, HUEBAR_WIDTH, static_cast<int>(px(18.0f)));
    GuiSetStyle(COLORPICKER, HUEBAR_PADDING, static_cast<int>(px(10.0f)));
    GuiSetStyle(COLORPICKER, HUEBAR_SELECTOR_HEIGHT, static_cast<int>(px(6.0f)));
    GuiSetStyle(COLORPICKER, HUEBAR_SELECTOR_OVERFLOW, static_cast<int>(px(3.0f)));

    g.styledTheme = themeKind();
    g.styledScale = g.scale;
}

} // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void init() {
    g.initialized = true;
    beginFrame();
}

namespace {
int g_pressEvents[3] = {0, 0, 0}, g_releaseEvents[3] = {0, 0, 0};
bool g_pressedLatch[3] = {false, false, false}, g_releasedLatch[3] = {false, false, false};
#ifdef AXIS_CLICK_LATCH
GLFWmousebuttonfun g_previousButtonCallback = nullptr;
void onMouseButton(GLFWwindow* window, int button, int action, int mods) {
    if (button >= 0 && button < 3) {
        if (action == GLFW_PRESS) g_pressEvents[button]++;
        else if (action == GLFW_RELEASE) g_releaseEvents[button]++;
    }
    if (g_previousButtonCallback) g_previousButtonCallback(window, button, action, mods);
}
#endif
} // namespace

void installClickLatch() {
#ifdef AXIS_CLICK_LATCH
    auto* window = static_cast<GLFWwindow*>(GetWindowHandle());
    if (window) g_previousButtonCallback = glfwSetMouseButtonCallback(window, onMouseButton);
#endif
}

bool mousePressed(int button) {
    return IsMouseButtonPressed(button) || (button >= 0 && button < 3 && g_pressedLatch[button]);
}

bool mouseReleased(int button) {
    return IsMouseButtonReleased(button) || (button >= 0 && button < 3 && g_releasedLatch[button]);
}

void beginFrame() {
    // Events of the frame just polled (callbacks ran inside raylib's EndDrawing)
    for (int b = 0; b < 3; b++) {
        g_pressedLatch[b] = g_pressEvents[b] > 0;
        g_releasedLatch[b] = g_releaseEvents[b] > 0;
        g_pressEvents[b] = g_releaseEvents[b] = 0;
    }
    g_lastAnchors.swap(g_anchors);
    g_anchors.clear();

    float w = static_cast<float>(GetScreenWidth());
    float h = static_cast<float>(GetScreenHeight());
    float automatic = std::clamp(std::min(w / kDesignWidth, h / kDesignHeight), 0.75f, 3.0f);
    g.scale = automatic * g.zoom;

    // Layout follows the new scale at once; font atlases are rebuilt once the
    // size settles (meanwhile the old atlas is scaled, which is still legible)
    bool needFonts = !g.fontLoaded[0] || std::fabs(g.scale - g.fontScale) / g.fontScale > 0.02f;
    if (needFonts) {
        double now = GetTime();
        if (g.pendingSince < 0.0) g.pendingSince = now;
        if (!g.fontLoaded[0] || now - g.pendingSince > 0.15) {
            buildFonts(g.scale);
            g.pendingSince = -1.0;
            g.styledScale = 0.0f;  // force restyle
        }
    } else {
        g.pendingSince = -1.0;
    }

    if (g.styledScale != g.scale || g.styledTheme != themeKind()) applyGuiStyle();
}

void shutdown() {
    for (int i = 0; i < kRoleCount; i++) {
        if (g.fontLoaded[i]) UnloadFont(g.fonts[i]);
        g.fontLoaded[i] = false;
    }
}

// ---------------------------------------------------------------------------
// Scale
// ---------------------------------------------------------------------------

float scale() { return g.scale; }
float zoom() { return g.zoom; }
void setZoom(float z) { g.zoom = std::clamp(z, 0.7f, 2.0f); }
void zoomIn() { setZoom(std::round((g.zoom + 0.1f) * 10.0f) / 10.0f); }
void zoomOut() { setZoom(std::round((g.zoom - 0.1f) * 10.0f) / 10.0f); }

float pxi(float design) {
    return std::max(1.0f, std::round(design * g.scale));
}

Rectangle rect(float x, float y, float w, float h) {
    return {px(x), px(y), px(w), px(h)};
}

Rectangle inset(Rectangle r, float d) {
    float p = px(d);
    return {r.x + p, r.y + p, std::max(0.0f, r.width - 2 * p), std::max(0.0f, r.height - 2 * p)};
}

Rectangle snap(Rectangle r) {
    return {std::round(r.x), std::round(r.y), std::round(r.width), std::round(r.height)};
}

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------

const Font& font(FontRole role) {
    int i = roleIndex(role);
    static Font fallback = GetFontDefault();
    return g.fontLoaded[i] ? g.fonts[i] : fallback;
}

float fontSize(FontRole role) {
    RoleSpec spec = roleSpec(role);
    return std::round(std::max(spec.minPixels, spec.designSize * g.scale));
}

Vector2 measure(const char* s, FontRole role) {
    return MeasureTextEx(font(role), s, fontSize(role), 0.0f);
}

float textWidth(const char* s, FontRole role) {
    return measure(s, role).x;
}

void text(const char* s, float x, float y, FontRole role, Color color) {
    DrawTextEx(font(role), s, {std::round(x), std::round(y)}, fontSize(role), 0.0f, color);
}

void textIn(Rectangle r, const char* s, FontRole role, Color color, Align align) {
    Vector2 size = measure(s, role);
    float x = r.x;
    if (align == Align::Center) x = r.x + (r.width - size.x) / 2.0f;
    else if (align == Align::Right) x = r.x + r.width - size.x;
    float y = r.y + (r.height - fontSize(role)) / 2.0f;
    text(s, x, y, role, color);
}

float textWrapped(Rectangle r, const char* s, FontRole role, Color color, bool draw, float lineSpacing) {
    float size = fontSize(role);
    float lineHeight = std::round(size * lineSpacing);
    float y = r.y;
    std::string line;
    std::string word;

    // Leading spaces become an indent for every wrapped line (keeps aligned formulas aligned)
    const char* p = s;
    std::string lead;
    while (*p == ' ') lead += *p++;
    float indent = lead.empty() ? 0.0f : textWidth(lead.c_str(), role);
    r.x += indent;
    r.width = std::max(1.0f, r.width - indent);

    auto flushLine = [&]() {
        if (draw) text(line.c_str(), r.x, y, role, color);
        y += lineHeight;
        line.clear();
    };

    while (true) {
        char c = *p;
        if (c == ' ' || c == '\n' || c == '\0') {
            if (!word.empty()) {
                std::string candidate = line.empty() ? word : line + " " + word;
                if (!line.empty() && textWidth(candidate.c_str(), role) > r.width) {
                    flushLine();
                    line = word;
                } else {
                    line = candidate;
                }
                word.clear();
            }
            if (c == '\n') {
                flushLine();
            }
            if (c == '\0') break;
        } else {
            word += c;
        }
        p++;
    }
    if (!line.empty()) flushLine();
    return y - r.y;
}

std::string ellipsize(const char* s, FontRole role, float maxWidth) {
    std::string str = s;
    if (textWidth(s, role) <= maxWidth) return str;
    const std::string dots = "…";
    while (!str.empty()) {
        // Drop one UTF-8 code point from the end
        size_t cut = str.size() - 1;
        while (cut > 0 && (static_cast<unsigned char>(str[cut]) & 0xC0) == 0x80) cut--;
        str.erase(cut);
        if (textWidth((str + dots).c_str(), role) <= maxWidth) break;
    }
    return str + dots;
}

std::string upper(const char* s) {
    std::string out = s;
    for (auto& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return out;
}

// ---------------------------------------------------------------------------
// Primitives
// ---------------------------------------------------------------------------

namespace {

float roundness(Rectangle r, float radius) {
    float m = std::min(r.width, r.height);
    if (m <= 0.0f) return 0.0f;
    return std::clamp(2.0f * radius / m, 0.0f, 1.0f);
}

int segmentsFor(float radius) {
    return std::clamp(static_cast<int>(radius / 1.5f), 4, 16);
}

} // namespace

void fillRounded(Rectangle r, float radius, Color color) {
    if (radius <= 0.5f) {
        DrawRectangleRec(r, color);
        return;
    }
    DrawRectangleRounded(r, roundness(r, radius), segmentsFor(radius), color);
}

void strokeRounded(Rectangle r, float radius, float thickness, Color color) {
    // Inset by half the thickness so the stroke stays inside the rectangle
    Rectangle in = {r.x + thickness / 2, r.y + thickness / 2, r.width - thickness, r.height - thickness};
    if (radius <= 0.5f) {
        DrawRectangleLinesEx(r, thickness, color);
        return;
    }
    DrawRectangleRoundedLinesEx(in, roundness(in, radius), segmentsFor(radius), thickness, color);
}

void shadow(Rectangle r, float radius, float spread) {
    Color c = theme().shadow;
    // A few expanding, fading layers approximate a soft shadow
    const int layers = 4;
    for (int i = layers; i >= 1; i--) {
        float grow = spread * i / layers;
        Rectangle s = {r.x - grow / 2, r.y - grow / 2 + spread * 0.35f, r.width + grow, r.height + grow};
        Color layer = c;
        layer.a = static_cast<unsigned char>(c.a / (layers + 1.0f) * (layers - i + 1) / layers);
        fillRounded(s, radius + grow / 2, layer);
    }
}

void line(Vector2 a, Vector2 b, float thickness, Color color) {
    DrawLineEx(a, b, thickness, color);
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------

namespace {
bool g_hasClip = false;
Rectangle g_clip = {0, 0, 0, 0};
}

bool canInteract() {
    return !GuiIsLocked();
}

void setClip(Rectangle r) {
    g_clip = r;
    g_hasClip = true;
}

void clearClip() {
    g_hasClip = false;
}

bool mouseInClip() {
    return !g_hasClip || CheckCollisionPointRec(GetMousePosition(), g_clip);
}

bool hovered(Rectangle r) {
    Vector2 m = GetMousePosition();
    if (g_hasClip && !CheckCollisionPointRec(m, g_clip)) return false;
    return canInteract() && CheckCollisionPointRec(m, r);
}

bool clicked(Rectangle r) {
    return hovered(r) && mousePressed(MOUSE_BUTTON_LEFT);
}

bool rightClicked(Rectangle r) {
    return hovered(r) && mousePressed(MOUSE_BUTTON_RIGHT);
}

// ---------------------------------------------------------------------------
// Anchors
// ---------------------------------------------------------------------------

void anchor(const char* id, Rectangle r) {
    g_anchors[id] = r;
}

bool findAnchor(const std::string& id, Rectangle& out) {
    auto it = g_lastAnchors.find(id);
    if (it == g_lastAnchors.end()) return false;
    out = it->second;
    return true;
}

Rectangle unite(Rectangle a, Rectangle b) {
    float x0 = std::min(a.x, b.x), y0 = std::min(a.y, b.y);
    float x1 = std::max(a.x + a.width, b.x + b.width), y1 = std::max(a.y + a.height, b.y + b.height);
    return {x0, y0, x1 - x0, y1 - y0};
}

// ---------------------------------------------------------------------------
// Tooltips
// ---------------------------------------------------------------------------

namespace {

struct TooltipState {
    std::string title, body;          // candidate registered this frame
    std::string shownTitle, shownBody;
    Rectangle rect = {0, 0, 0, 0};
    Rectangle lastRect = {0, 0, 0, 0};
    bool hasCandidate = false;
    double since = 0.0;
};

TooltipState g_tip;

bool sameRect(Rectangle a, Rectangle b) {
    return std::fabs(a.x - b.x) < 1 && std::fabs(a.y - b.y) < 1 && std::fabs(a.width - b.width) < 1 &&
           std::fabs(a.height - b.height) < 1;
}

} // namespace

void tooltip(Rectangle r, const char* body, const char* title) {
    if (!hovered(r)) return;
    // The last registration wins, so inner widgets override their containers
    g_tip.hasCandidate = true;
    g_tip.rect = r;
    g_tip.body = body ? body : "";
    g_tip.title = title ? title : "";
}

void drawTooltip(bool enabled) {
    bool has = g_tip.hasCandidate;
    g_tip.hasCandidate = false;
    if (!has || !enabled || IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        g_tip.lastRect = {0, 0, 0, 0};
        return;
    }
    double now = GetTime();
    if (!sameRect(g_tip.rect, g_tip.lastRect) || g_tip.body != g_tip.shownBody) {
        g_tip.lastRect = g_tip.rect;
        g_tip.shownBody = g_tip.body;
        g_tip.shownTitle = g_tip.title;
        g_tip.since = now;
    }
    if (now - g_tip.since < 0.45) return;

    const Theme& t = theme();
    float maxW = px(290.0f);
    float pad = px(10.0f);
    bool hasTitle = !g_tip.shownTitle.empty();
    float titleH = hasTitle ? fontSize(FontRole::BodyStrong) * 1.4f : 0.0f;
    float bodyW = std::min(maxW, textWidth(g_tip.shownBody.c_str(), FontRole::Small) + 1.0f);
    if (hasTitle) bodyW = std::max(bodyW, std::min(maxW, textWidth(g_tip.shownTitle.c_str(), FontRole::BodyStrong)));
    float bodyH = textWrapped({0, 0, bodyW, 0}, g_tip.shownBody.c_str(), FontRole::Small, t.text, false);

    Vector2 m = GetMousePosition();
    float w = bodyW + 2 * pad, h = titleH + bodyH + 2 * pad - px(2.0f);
    float x = m.x + px(14.0f), y = m.y + px(20.0f);
    float sw = static_cast<float>(GetScreenWidth()), sh = static_cast<float>(GetScreenHeight());
    if (x + w > sw - px(6)) x = sw - w - px(6);
    if (y + h > sh - px(6)) y = m.y - h - px(10.0f);
    Rectangle box = snap({x, y, w, h});

    shadow(box, px(8), px(10));
    fillRounded(box, px(8), t.panel);
    strokeRounded(box, px(8), pxi(1), t.borderStrong);
    float ty = box.y + pad - px(1);
    if (hasTitle) {
        text(g_tip.shownTitle.c_str(), box.x + pad, ty, FontRole::BodyStrong, t.text);
        ty += titleH;
    }
    textWrapped({box.x + pad, ty, bodyW, 0}, g_tip.shownBody.c_str(), FontRole::Small,
                hasTitle ? t.textMuted : t.text);
}

} // namespace linalg::ui
