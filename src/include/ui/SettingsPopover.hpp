#pragma once

#include "ui/Preferences.hpp"
#include <raylib.h>

namespace linalg {

struct SettingsActions {
    bool themeChanged = false;
    bool resetLayout = false;
};

// Small card below the gear button: theme, interface size, tooltips, HUD,
// panel layout reset. Clicking outside closes it.
class SettingsPopover {
public:
    void open(Rectangle anchor) { m_open = true; m_anchor = anchor; m_justOpened = true; }
    void close() { m_open = false; }
    bool isOpen() const { return m_open; }
    bool contains(Vector2 p) const { return m_open && CheckCollisionPointRec(p, m_bounds); }

    SettingsActions draw(UiSettings& settings);

private:
    float m_height = 0.0f;  // of the content, measured on the last frame (pixels)
    bool m_open = false;
    bool m_justOpened = false;
    Rectangle m_anchor = {0, 0, 0, 0};
    Rectangle m_bounds = {0, 0, 0, 0};
    float m_pendingZoom = 0.0f;
};

} // namespace linalg
