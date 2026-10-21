#pragma once

// The formula reference (F1): everything you can type in a formula, by section, with an example per entry
// evaluated live against the objects of the current space; a search field, and a field to try any formula.
// The content is math/expr/Reference, shared with the raylib app's reference panel.

typedef struct _GtkWidget GtkWidget;

namespace linalg {
class Session;
}

namespace linalg::gtk {

void showReferenceDialog(GtkWidget* parent, Session& session);

} // namespace linalg::gtk
