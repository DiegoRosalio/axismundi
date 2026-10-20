#pragma once

// Saving what a widget shows (the whole window, with the 3D view) as a PNG: screenshots for the tour and on
// SIGUSR1, like the raylib app.

#include <string>

typedef struct _GtkWidget GtkWidget;

namespace linalg::gtk {

bool saveWidgetPng(GtkWidget* widget, const std::string& path);

} // namespace linalg::gtk
