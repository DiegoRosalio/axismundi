#pragma once

#include "ui/SceneIO.hpp"
#include <string>

namespace linalg {

// User preferences, kept separate from scenes: theme, interface size,
// tooltips, panel layout, window size... Stored as key = value text in
// $XDG_CONFIG_HOME/axismundi/settings.txt (default ~/.config/axismundi/).
struct Preferences {
    static std::string path();
    static bool load(SceneData& data);
    static bool save(const SceneData& data, std::string& error);
};

// Settings that the settings popover edits
struct UiSettings {
    bool tooltips = true;
    bool showHud = true;
    enum class Surfaces { StyleDefault, Smooth, Faceted };
    Surfaces surfaces = Surfaces::StyleDefault;  // smooth shading or flat cells; default: the view style's
};

} // namespace linalg
