#pragma once

// The example gallery: the examples of edu/Examples by category; activating one builds its scene through
// the window's AppApi (one undo step per object it adds) and closes the dialog.

#include <functional>

typedef struct _GtkWidget GtkWidget;

namespace linalg {
class AppApi;
}

namespace linalg::gtk {

// `opened(title)` runs after an example was applied. Returns the dialog (GTK frees it when it closes).
GtkWidget* showGalleryDialog(GtkWidget* parent, AppApi& api, std::function<void(const char*)> opened);

} // namespace linalg::gtk
