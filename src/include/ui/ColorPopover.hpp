#pragma once

#include <raylib.h>
#include <string>

namespace linalg {

// Popup with a color picker, preset chips and a reset button, opened from a
// swatch. The caller applies the result.
class ColorPopover {
public:
    struct Result {
        bool changed = false;  // `color` holds the new color
        bool reset = false;    // go back to the theme color
        Color color = {0, 0, 0, 255};
    };

    void open(int slot, Rectangle anchor, const std::string& title, Color current);
    void close() { m_open = false; }
    bool isOpen() const { return m_open; }
    int slot() const { return m_slot; }
    bool contains(Vector2 p) const { return m_open && CheckCollisionPointRec(p, m_bounds); }

    Result draw();

private:
    bool m_open = false;
    bool m_justOpened = false;
    int m_slot = -1;
    std::string m_title;
    Color m_color = {0, 0, 0, 255};
    Rectangle m_anchor = {0, 0, 0, 0};
    Rectangle m_bounds = {0, 0, 0, 0};
};

} // namespace linalg
