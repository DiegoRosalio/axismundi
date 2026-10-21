#pragma once

// The GTK app's preferences (theme, dimming the other objects, how surfaces are shaded), kept in
// ~/.config/axismundi/gtk.txt, and the Preferences dialog (an AdwPreferencesDialog) that edits them.

#include "ui/Preferences.hpp"
#include <functional>
#include <string>

typedef struct _GtkWidget GtkWidget;

namespace linalg::gtk {

struct GtkSettings {
    std::string theme;          // "" follows the system, else "light" or "dark"
    bool dimUnselected = true;  // fade the other objects while something is selected
    UiSettings::Surfaces surfaces = UiSettings::Surfaces::StyleDefault;
    unsigned int lessonsFinished = 0;  // a bit per finished lesson, by position (edu/Tutorial's lessons())

    void load();  // missing keys keep their defaults
    void save() const;
};

// Show the dialog over `parent`; `changed` runs after every edit (the settings are saved then too)
void showSettingsDialog(GtkWidget* parent, GtkSettings& settings, std::function<void()> changed);

} // namespace linalg::gtk
