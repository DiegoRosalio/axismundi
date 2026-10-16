#include "ui/HelpOverlay.hpp"
#include <raylib.h>

namespace linalg {

namespace {

struct Entry {
    const char* key;
    const char* action;
};

const Entry kCamera[] = {
    {"TAB", "Toggle camera mode / UI mode"},
    {"Mouse", "Look around (camera mode)"},
    {"W A S D", "Move (camera mode)"},
    {"Space / Ctrl", "Up / down (camera mode)"},
    {"Shift", "Move faster"},
    {"C", "Reset camera"},
    {"1 / 2 / 3", "Matrices / Complex / Plots mode"},
    {"G", "Toggle grid"},
    {"L", "Toggle labels"},
    {"M", "Toggle matrix info panel"},
    {"H", "Toggle this help"},
};

const Entry kEditing[] = {
    {"I", "Identity matrix"},
    {"R", "Rotate 45 deg (Z axis)"},
    {"S", "Scale 2x (UI mode only)"},
    {"X", "Shear X by 0.5"},
    {"F", "Reflect across X axis"},
    {"Ctrl+Z", "Undo (UI mode)"},
    {"Ctrl+Y", "Redo (UI mode)"},
    {"P", "Play / pause animation"},
    {"F5 / F9", "Save / load scene.txt"},
    {"Enter", "Confirm a text field"},
    {"Tab", "Confirm and go to next field"},
    {"Esc", "Cancel the edit (quits otherwise)"},
};

void drawColumn(const char* title, const Entry* entries, int count, int x, int y) {
    DrawText(title, x, y, 30, DARKBLUE);
    y += 46;
    for (int i = 0; i < count; i++) {
        DrawText(entries[i].key, x, y, 24, MAROON);
        DrawText(entries[i].action, x + 180, y, 24, DARKGRAY);
        y += 34;
    }
}

} // namespace

void HelpOverlay::draw() const {
    if (!m_visible) return;

    int w = GetScreenWidth();
    int h = GetScreenHeight();
    DrawRectangle(0, 0, w, h, Fade(BLACK, 0.45f));

    int boxW = 1100 < w - 40 ? 1100 : w - 40;
    int boxH = 640 < h - 40 ? 640 : h - 40;
    int bx = (w - boxW) / 2;
    int by = (h - boxH) / 2;
    DrawRectangle(bx, by, boxW, boxH, Fade(RAYWHITE, 0.97f));
    DrawRectangleLines(bx, by, boxW, boxH, DARKGRAY);

    DrawText("Keyboard Shortcuts", bx + 30, by + 20, 36, BLACK);
    DrawText("Shortcuts are ignored while a text field is being edited.",
             bx + 30, by + 62, 20, GRAY);

    int colY = by + 100;
    drawColumn("View & Camera", kCamera, sizeof(kCamera) / sizeof(kCamera[0]), bx + 30, colY);
    drawColumn("Editing", kEditing, sizeof(kEditing) / sizeof(kEditing[0]), bx + boxW / 2 + 10, colY);

    DrawText("Press H to close", bx + boxW - 220, by + boxH - 36, 22, GRAY);
}

} // namespace linalg
