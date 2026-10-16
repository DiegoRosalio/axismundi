#pragma once

#include "ui/AppMode.hpp"
#include <raylib.h>

namespace linalg {

// Actions requested from the top bar this frame
struct TopBarActions {
    bool toggleTheme = false;
    bool toggleHelp = false;
    bool toggleSettings = false;
    bool toggleFullscreen = false;
    bool screenshot = false;
};

// Which toggles are currently on (drawn as selected)
struct TopBarState {
    bool helpVisible = false;
    bool settingsOpen = false;
    bool fullscreen = false;
};

// Full-width bar at the top: app name, mode tabs and action buttons.
class TopBar {
public:
    static float height();  // pixels

    // Draws the bar; may change `mode`. Returns the buttons clicked.
    TopBarActions draw(AppMode& mode, const TopBarState& state);

    // Screen rectangle of the settings button (anchor for the popover)
    Rectangle settingsButton() const { return m_settingsButton; }

private:
    Rectangle m_settingsButton = {0, 0, 0, 0};
};

} // namespace linalg
