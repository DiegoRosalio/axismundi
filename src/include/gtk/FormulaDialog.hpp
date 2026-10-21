#pragma once

// The big formula editor (the ⤢ button of a formula row): room for long formulas, with the live result underneath
// (scene::fieldPreview: what the object would get, or the problem). Ctrl+Enter or Apply commits, Esc cancels.

#include "scene/SceneObject.hpp"
#include <functional>
#include <string>

typedef struct _GtkWidget GtkWidget;

namespace linalg {
class Session;
}

namespace linalg::gtk {

// `apply(text)` commits; the field is found again by object id and key while typing (it may disappear)
void showFormulaDialog(GtkWidget* parent, Session& session, scene::SpaceKind kind, scene::ObjectId object,
                       const std::string& key, const std::string& text, std::function<void(const std::string&)> apply);

} // namespace linalg::gtk
