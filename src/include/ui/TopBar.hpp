#pragma once

#include "ui/AppMode.hpp"

namespace linalg {

// Actions requested from the top bar this frame
struct TopBarActions {
    bool toggleTheme = false;
    bool toggleHelp = false;
};

// Full-width bar at the top: app name, mode tabs and action buttons.
class TopBar {
public:
    static float height();  // pixels

    // Draws the bar; may change `mode`. Returns the buttons clicked.
    TopBarActions draw(AppMode& mode, bool helpVisible);
};

} // namespace linalg
