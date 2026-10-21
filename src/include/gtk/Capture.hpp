#pragma once

// Saving what a window shows as a PNG (from the X11 screen when GTK runs on X11, so popovers and menus over it are
// included; through GTK otherwise): screenshots for the tour and on SIGUSR1, like the raylib app.

#include <string>

typedef struct _GtkWidget GtkWidget;

namespace linalg::gtk {

bool saveWidgetPng(GtkWidget* widget, const std::string& path);

} // namespace linalg::gtk
