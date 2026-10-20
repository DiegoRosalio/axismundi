#include "ui/HelpOverlay.hpp"
#include "app/Version.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <raylib.h>
#include <string>
#include <algorithm>

namespace linalg {

namespace {

struct Entry {
    const char* keys;   // keys separated by '|' are drawn as separate keycaps
    const char* action;
};

const Entry kView[] = {
    {"Drag|Right-drag", "Orbit / pan (orbit camera)"},
    {"Wheel", "Zoom (orbit camera)"},
    {".", "Center on the selection"},
    {"1|3|7", "Front / right / top view (keypad)"},
    {"Tab|Esc", "Capture / free the mouse (fly camera)"},
    {"W|A|S|D", "Move (fly camera, Shift: faster)"},
    {"Space|Ctrl", "Up / down (fly camera)"},
    {"C", "Reset camera"},
    {"1|2", "3D space / complex plane"},
    {"G", "Toggle grid"},
    {"L", "Toggle labels"},
    {"O", "Show only the selection"},
    {"T", "Dark / light theme"},
    {"Ctrl+=|Ctrl+-", "Bigger / smaller UI"},
    {"F11", "Fullscreen"},
    {"H", "Toggle this help"},
};

const Entry kPanels[] = {
    {"F1", "Tutorial"},
    {"F2", "Formula reference"},
    {"Ctrl+E", "Export to LaTeX"},
    {"Ctrl+B", "Hide / show the side bar"},
    {"E", "Example gallery"},
    {"M", "Analysis panel"},
    {"K", "Calculation steps"},
    {"B", "Transformation breakdown"},
    {"F12", "Screenshot (Shift: 3D view only)"},
    {"Ctrl+S|Ctrl+O", "Save / open a scene (F5 / F9)"},
};

const Entry kEdit[] = {
    {"N", "Add an object"},
    {"Del|Ctrl+D", "Delete / duplicate the selection"},
    {"I|R|S|X|F", "Matrix presets, added on top (I resets)"},
    {"Ctrl+Z|Ctrl+Y", "Undo / redo (UI mode)"},
    {"P", "Play / pause the scene clock"},
    {"Click", "Select (Ctrl+click: add to selection)"},
    {"Enter|Tab", "Confirm a field / go to the next"},
    {"Esc", "Cancel the edit, close a popup"},
};

float drawColumn(const char* title, const Entry* entries, int count, float x, float y, float width, float step) {
    const ui::Theme& t = ui::theme();
    ui::text(ui::upper(title).c_str(), x, y, ui::FontRole::Caption, t.accent);
    y += ui::px(26.0f);
    float keyColumn = ui::px(150.0f);
    for (int i = 0; i < count; i++) {
        // Keycaps
        float kx = x;
        std::string keys = entries[i].keys;
        size_t start = 0;
        while (start <= keys.size()) {
            size_t bar = keys.find('|', start);
            std::string key = keys.substr(start, bar == std::string::npos ? std::string::npos : bar - start);
            kx += ui::keycap(kx, y, key.c_str()) + ui::px(4.0f);
            if (bar == std::string::npos) break;
            start = bar + 1;
        }
        Rectangle label = {x + keyColumn, y, width - keyColumn, ui::px(24.0f)};
        ui::textIn(label, entries[i].action, ui::FontRole::Body, t.text);
        y += step;
    }
    return y;
}

} // namespace

void HelpOverlay::draw() const {
    if (!m_visible) return;
    const ui::Theme& t = ui::theme();

    float w = static_cast<float>(GetScreenWidth());
    float h = static_cast<float>(GetScreenHeight());
    DrawRectangle(0, 0, static_cast<int>(w), static_cast<int>(h), t.overlay);

    float boxW = std::min(ui::px(900.0f), w - ui::px(32.0f));
    float boxH = std::min(ui::px(700.0f), h - ui::px(32.0f));
    Rectangle box = {(w - boxW) / 2, (h - boxH) / 2, boxW, boxH};
    ui::card(box);

    float pad = ui::px(28.0f);
    ui::text("Keyboard shortcuts", box.x + pad, box.y + pad - ui::px(4.0f), ui::FontRole::Display, t.text);
    ui::text("Shortcuts are ignored while you type in a field.", box.x + pad, box.y + pad + ui::px(32.0f),
             ui::FontRole::Small, t.textMuted);

    float colY = box.y + pad + ui::px(70.0f);
    float colW = (boxW - 3 * pad) / 2;
    // Rows tighten on small windows so the right column (two groups) ends above the footer
    const int nView = sizeof(kView) / sizeof(kView[0]), nPanels = sizeof(kPanels) / sizeof(kPanels[0]),
              nEdit = sizeof(kEdit) / sizeof(kEdit[0]);
    float available = box.y + boxH - pad - ui::px(26.0f) - colY - 2 * ui::px(26.0f) - ui::px(12.0f);
    float step = std::clamp(available / static_cast<float>(std::max(nView, nPanels + nEdit)), ui::px(25.0f),
                            ui::px(31.0f));
    drawColumn("View and camera", kView, nView, box.x + pad, colY, colW, step);
    float y2 = drawColumn("Panels and tools", kPanels, nPanels, box.x + 2 * pad + colW, colY, colW, step);
    drawColumn("Editing", kEdit, nEdit, box.x + 2 * pad + colW, y2 + ui::px(12.0f), colW, step);

    ui::textIn({box.x + pad, box.y + boxH - pad, boxW - 2 * pad, ui::px(20.0f)},
               (std::string("axismundi ") + kVersion).c_str(), ui::FontRole::Small, t.textFaint);
    ui::textIn({box.x, box.y + boxH - pad, boxW - pad, ui::px(20.0f)}, "Press H or Esc to close",
               ui::FontRole::Small, t.textFaint, ui::Align::Right);
}

} // namespace linalg
