#include "ui/Widgets.hpp"
#include "math/Expression.hpp"
#include "ui/core/Theme.hpp"

#include <raygui.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace linalg {

bool parseNumber(const char* text, float& out) {
    Expression expr;
    if (!expr.parse(text)) return false;
    double v = expr.eval(0.0);
    if (!std::isfinite(v)) return false;
    out = static_cast<float>(v);
    return true;
}

void formatNumber(char* buf, size_t size, float value) {
    if (std::fabs(value) < 5e-5f) value = 0.0f;  // avoid "-0.0"
    snprintf(buf, size, "%.4f", value);

    // Trim trailing zeros, keeping one digit after the decimal point
    char* dot = std::strchr(buf, '.');
    if (!dot) return;
    char* end = buf + std::strlen(buf) - 1;
    while (end > dot + 1 && *end == '0') *end-- = '\0';
}

namespace ui {

namespace {

// Identity of a widget for press/drag tracking: its rounded rectangle
unsigned int widgetId(Rectangle r) {
    unsigned int h = 2166136261u;
    int v[4] = {static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width),
                static_cast<int>(r.height)};
    for (int x : v) {
        h ^= static_cast<unsigned int>(x);
        h *= 16777619u;
    }
    return h;
}

unsigned int g_pressedId = 0;   // widget that received the mouse press
unsigned int g_dragId = 0;      // slider being dragged

// Press-then-release-inside behavior shared by clickable widgets
bool clickBehavior(Rectangle r, bool enabled, bool& hover, bool& down) {
    unsigned int id = widgetId(r);
    hover = enabled && hovered(r);
    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) g_pressedId = id;
    down = (g_pressedId == id) && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool fired = false;
    if (g_pressedId == id && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        fired = hover;
        g_pressedId = 0;
    }
    return fired;
}

Color withAlpha(Color c, float a) {
    c.a = static_cast<unsigned char>(c.a * a);
    return c;
}

} // namespace

bool button(Rectangle r, const char* label, ButtonKind kind, bool enabled, Icon icon, const char* hint) {
    const Theme& t = theme();
    bool hover = false, down = false;
    bool fired = clickBehavior(r, enabled, hover, down);

    Color fill = t.panelAlt, border = t.border, ink = t.text;
    switch (kind) {
        case ButtonKind::Primary:
            fill = hover ? t.accentHover : t.accent;
            border = fill;
            ink = t.onAccent;
            break;
        case ButtonKind::Secondary:
            fill = down ? t.pressed : hover ? t.hover : t.panelAlt;
            border = hover ? t.borderStrong : t.border;
            break;
        case ButtonKind::Ghost:
            fill = down ? t.pressed : hover ? t.hover : Color{0, 0, 0, 0};
            border = fill;
            break;
        case ButtonKind::Danger:
            fill = down ? t.pressed : hover ? t.hover : t.panelAlt;
            border = hover ? t.danger : t.border;
            ink = t.danger;
            break;
    }
    float alpha = enabled ? 1.0f : 0.45f;
    float radius = px(kRadius);
    fillRounded(r, radius, withAlpha(fill, alpha));
    if (border.a > 0) strokeRounded(r, radius, pxi(1.0f), withAlpha(border, alpha));

    // Content: [icon] label ... hint
    float pad = px(10.0f);
    Rectangle content = {r.x + pad, r.y, r.width - 2 * pad, r.height};
    float hintW = 0.0f;
    if (hint) {
        hintW = textWidth(hint, FontRole::Small);
        Color hintInk = kind == ButtonKind::Primary ? withAlpha(t.onAccent, 0.7f) : t.textFaint;
        textIn({content.x + content.width - hintW, r.y, hintW, r.height}, hint, FontRole::Small,
               withAlpha(hintInk, alpha), Align::Left);
        content.width -= hintW + px(6.0f);
    }

    float iconSize = px(16.0f);
    float labelW = (label && *label) ? textWidth(label, FontRole::Body) : 0.0f;
    float total = labelW + (icon != Icon::None ? iconSize + (labelW > 0 ? px(6.0f) : 0.0f) : 0.0f);
    float x = hint ? content.x : content.x + std::max(0.0f, (content.width - total) / 2.0f);
    if (icon != Icon::None) {
        drawIcon(icon, {x, r.y + (r.height - iconSize) / 2.0f, iconSize, iconSize}, withAlpha(ink, alpha));
        x += iconSize + px(6.0f);
    }
    if (labelW > 0) {
        std::string shown = ellipsize(label, FontRole::Body, content.x + content.width - x);
        textIn({x, r.y, content.x + content.width - x, r.height}, shown.c_str(), FontRole::Body,
               withAlpha(ink, alpha), Align::Left);
    }
    return fired && enabled;
}

bool iconButton(Rectangle r, Icon icon, bool active, bool enabled) {
    const Theme& t = theme();
    bool hover = false, down = false;
    bool fired = clickBehavior(r, enabled, hover, down);

    Color fill = active ? t.accentSoft : down ? t.pressed : hover ? t.hover : Color{0, 0, 0, 0};
    if (fill.a > 0) fillRounded(r, px(kRadius), fill);
    Color ink = active ? t.accent : hover ? t.text : t.textMuted;
    if (!enabled) ink = withAlpha(ink, 0.4f);
    float size = std::min(r.width, r.height) * 0.55f;
    drawIcon(icon, {r.x + (r.width - size) / 2, r.y + (r.height - size) / 2, size, size}, ink);
    return fired && enabled;
}

bool segmented(Rectangle r, std::initializer_list<const char*> labels, int& active) {
    const Theme& t = theme();
    float radius = px(kRadius);
    fillRounded(r, radius, t.panelAlt);
    strokeRounded(r, radius, pxi(1.0f), t.border);

    int n = static_cast<int>(labels.size());
    float pad = px(3.0f);
    float segW = (r.width - 2 * pad) / n;
    int i = 0;
    bool changed = false;
    for (const char* label : labels) {
        Rectangle seg = {r.x + pad + i * segW, r.y + pad, segW, r.height - 2 * pad};
        bool hover = false, down = false;
        if (clickBehavior(seg, true, hover, down) && active != i) {
            active = i;
            changed = true;
        }
        bool selected = (active == i);
        if (selected) {
            fillRounded(seg, radius - pad / 2, t.accent);
        } else if (hover) {
            fillRounded(seg, radius - pad / 2, t.hover);
        }
        Color ink = selected ? t.onAccent : hover ? t.text : t.textMuted;
        std::string shown = ellipsize(label, FontRole::Body, seg.width - px(8.0f));
        textIn(seg, shown.c_str(), selected ? FontRole::BodyStrong : FontRole::Body, ink, Align::Center);
        i++;
    }
    return changed;
}

bool toggleRow(Rectangle r, const char* label, bool& value, const char* hint) {
    const Theme& t = theme();
    bool hover = false, down = false;
    bool changed = false;
    if (clickBehavior(r, true, hover, down)) {
        value = !value;
        changed = true;
    }
    if (hover) fillRounded(r, px(kRadius), withAlpha(t.hover, 0.6f));

    float trackW = px(34.0f), trackH = px(18.0f);
    Rectangle track = {r.x + r.width - trackW - px(6.0f), r.y + (r.height - trackH) / 2.0f, trackW, trackH};
    fillRounded(track, trackH / 2, value ? t.accent : t.borderStrong);
    float knobR = trackH / 2 - px(2.5f);
    float knobX = value ? track.x + track.width - trackH / 2 : track.x + trackH / 2;
    DrawCircleV({knobX, track.y + trackH / 2}, knobR, value ? t.onAccent : t.panel);

    Rectangle labelRect = {r.x + px(6.0f), r.y, track.x - r.x - px(12.0f), r.height};
    std::string shown = ellipsize(label, FontRole::Body, labelRect.width);
    textIn(labelRect, shown.c_str(), FontRole::Body, t.text, Align::Left);
    if (hint) {
        float lw = textWidth(shown.c_str(), FontRole::Body) + px(8.0f);
        Rectangle hr = {labelRect.x + lw, r.y, labelRect.width - lw, r.height};
        if (hr.width > textWidth(hint, FontRole::Small)) textIn(hr, hint, FontRole::Small, t.textFaint, Align::Left);
    }
    return changed;
}

bool slider(Rectangle r, float& value, float minValue, float maxValue) {
    const Theme& t = theme();
    unsigned int id = widgetId(r);
    bool hover = hovered(r);
    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) g_dragId = id;
    if (g_dragId == id && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) g_dragId = 0;

    float knobR = px(7.0f);
    float x0 = r.x + knobR, x1 = r.x + r.width - knobR;
    bool changed = false;
    if (g_dragId == id && canInteract()) {
        float u = std::clamp((GetMousePosition().x - x0) / std::max(1.0f, x1 - x0), 0.0f, 1.0f);
        float v = minValue + u * (maxValue - minValue);
        if (v != value) {
            value = v;
            changed = true;
        }
    }

    float u = (maxValue > minValue) ? std::clamp((value - minValue) / (maxValue - minValue), 0.0f, 1.0f) : 0.0f;
    float cy = r.y + r.height / 2.0f;
    float trackH = px(4.0f);
    fillRounded({x0, cy - trackH / 2, x1 - x0, trackH}, trackH / 2, t.borderStrong);
    fillRounded({x0, cy - trackH / 2, (x1 - x0) * u, trackH}, trackH / 2, t.accent);
    float kx = x0 + (x1 - x0) * u;
    if (hover || g_dragId == id) DrawCircleV({kx, cy}, knobR + px(4.0f), t.accentSoft);
    DrawCircleV({kx, cy}, knobR, t.accent);
    DrawCircleV({kx, cy}, knobR - px(2.5f), t.onAccent);
    return changed;
}

bool sectionHeader(Rectangle r, const char* title, bool& expanded, const char* rightText) {
    const Theme& t = theme();
    bool hover = false, down = false;
    bool toggled = false;
    if (clickBehavior(r, true, hover, down)) {
        expanded = !expanded;
        toggled = true;
    }
    Color ink = hover ? t.text : t.textMuted;
    float iconSize = px(12.0f);
    drawIcon(expanded ? Icon::ChevronDown : Icon::ChevronRight,
             {r.x, r.y + (r.height - iconSize) / 2, iconSize, iconSize}, ink);
    std::string caps = upper(title);
    textIn({r.x + iconSize + px(6.0f), r.y, r.width, r.height}, caps.c_str(), FontRole::Caption, ink, Align::Left);
    if (rightText) textIn(r, rightText, FontRole::Small, t.textFaint, Align::Right);
    return toggled;
}

bool textField(Rectangle r, char* text, int textSize, int id, int& activeId, bool invalid) {
    const Theme& t = theme();
    bool editing = (activeId == id);
    bool hover = hovered(r);
    float radius = px(kRadius);

    // Our own rounded box; raygui only draws the text and cursor on top
    fillRounded(r, radius, editing ? t.panel : t.panelAlt);
    Color border = invalid ? t.danger : editing ? t.accent : hover ? t.borderStrong : t.border;
    strokeRounded(r, radius, editing || invalid ? pxi(1.5f) : pxi(1.0f), border);
    if (editing) {
        Rectangle ring = {r.x - px(2), r.y - px(2), r.width + px(4), r.height + px(4)};
        strokeRounded(ring, radius + px(2), pxi(2.0f), invalid ? withAlpha(t.danger, 0.3f) : t.accentSoft);
    }

    // Numbers and expressions read best in the monospaced font
    Font previousFont = GuiGetFont();
    int previousSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
    GuiSetFont(font(FontRole::Mono));
    GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(fontSize(FontRole::Mono)));

    // raygui does not know our clip area: keep it from reacting to clicks on
    // the part of a field scrolled out of view
    bool lockHere = !editing && !mouseInClip() && !GuiIsLocked();
    if (lockHere) GuiLock();

    bool finished = false;
    int result = GuiTextBox(r, text, textSize, editing);
    if (lockHere) GuiUnlock();

    GuiSetFont(previousFont);
    GuiSetStyle(DEFAULT, TEXT_SIZE, previousSize);

    if (result) {
        if (editing) {
            activeId = -1;
            finished = true;
        } else {
            activeId = id;
        }
    }
    return finished;
}

void card(Rectangle r, bool withShadow) {
    const Theme& t = theme();
    float radius = px(kCardRadius);
    if (withShadow) shadow(r, radius, px(14.0f));
    fillRounded(r, radius, t.panel);
    strokeRounded(r, radius, pxi(1.0f), t.border);
}

float keycap(float x, float y, const char* label) {
    const Theme& t = theme();
    float h = px(24.0f);
    float w = std::max(h, textWidth(label, FontRole::Small) + px(14.0f));
    Rectangle r = {x, y, w, h};
    fillRounded({r.x, r.y + px(2), r.width, r.height}, px(5), t.border);
    fillRounded(r, px(5), t.panelAlt);
    strokeRounded(r, px(5), pxi(1.0f), t.borderStrong);
    textIn(r, label, FontRole::Small, t.text, Align::Center);
    return w;
}

bool swatch(Rectangle r, Color color, bool selected) {
    const Theme& t = theme();
    bool hover = false, down = false;
    bool fired = clickBehavior(r, true, hover, down);
    float radius = px(kRadius);
    if (selected || hover) {
        Rectangle ring = {r.x - px(3), r.y - px(3), r.width + px(6), r.height + px(6)};
        strokeRounded(ring, radius + px(3), pxi(2.0f), selected ? t.accent : t.borderStrong);
    }
    fillRounded(r, radius, color);
    strokeRounded(r, radius, pxi(1.0f), withAlpha(t.text, 0.25f));
    return fired;
}

} // namespace ui
} // namespace linalg
