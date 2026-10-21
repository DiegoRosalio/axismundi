#pragma once

#include "ui/SceneIO.hpp"
#include <string>

namespace linalg {

// User preferences, kept separate from scenes: theme, interface size,
// tooltips, panel layout, window size... Stored as key = value text in
// $XDG_CONFIG_HOME/axismundi/settings.txt (default ~/.config/axismundi/); the GTK app uses gtk.txt there.
struct Preferences {
    static std::string path(const std::string& file = "settings.txt");
    static bool load(SceneData& data, const std::string& file = "settings.txt");
    static bool save(const SceneData& data, std::string& error, const std::string& file = "settings.txt");
};

// Settings that the settings popover edits
struct UiSettings {
    bool tooltips = true;
    bool dimUnselected = true;  // fade the other objects while something is selected
    bool showHud = true;
    enum class Surfaces { StyleDefault, Smooth, Faceted };
    Surfaces surfaces = Surfaces::StyleDefault;  // smooth shading or flat cells; default: the view style's
    bool flyCamera = false;  // false: orbit camera (drag, wheel); true: fly camera (Tab, mouse look, WASD)
};

} // namespace linalg
