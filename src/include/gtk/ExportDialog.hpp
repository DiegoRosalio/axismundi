#pragma once

// Export to LaTeX (Ctrl+E): the settings of the black-and-white PGFPlots figure (what, 2D or 3D, size, axes, legend,
// grid, shading, a complete document), the code as a live preview, Copy, Save .tex and Save PDF (compiled in the
// background with pdflatex or lualatex when one is installed). "Black-and-white preview" shows the 3D view in
// the Paper style while it is on. The settings stay in an ExportSettings the window keeps.

#include "export/LatexExport.hpp"
#include "view/ViewStyle.hpp"
#include <functional>
#include <string>

typedef struct _GtkWidget GtkWidget;

namespace linalg {
class Session;
}

namespace linalg::gtk {

struct ExportSettings {
    latex::Options options;
    bool preview = false;                                       // the 3D view in the Paper style
    view::StyleId styleBefore = view::StyleId::Colorful;        // restored when the preview ends
};

struct ExportCallbacks {
    std::function<void(const std::string&)> toast;
    std::function<void()> viewChanged;  // the preview style changed: redraw
};

void showExportDialog(GtkWidget* parent, Session& session, ExportSettings& settings, ExportCallbacks callbacks);

} // namespace linalg::gtk
