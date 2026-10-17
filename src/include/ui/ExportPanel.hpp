#pragma once

#include "export/LatexExport.hpp"
#include "ui/SceneIO.hpp"
#include <raylib.h>
#include <string>

namespace linalg {

// "Export to LaTeX" panel: settings for the black-and-white PGFPlots figure,
// a live preview of the code, and Copy / Save buttons. Not modal, so the
// scene can be adjusted while it is open.
class ExportPanel {
public:
    struct Actions {
        bool copy = false;  // copy the code to the clipboard
        bool save = false;  // open the Save .tex dialog
    };

    // Returns the content height (pixels). `azimuth`/`elevation` come from the camera.
    float drawContent(Rectangle area, const scene::Space& space, const scene::Selection& selection, double azimuth,
                      double elevation, Actions& actions);

    // The code for the current settings (also used by Copy/Save)
    latex::Result generate(const scene::Space& space, const scene::Selection& selection, double azimuth,
                           double elevation) const;

    const latex::Options& options() const { return m_options; }
    latex::Options& options() { return m_options; }
    bool preview() const { return m_preview; }  // black-and-white print preview of the 3D view
    void setPreview(bool on) { m_preview = on; }

    void saveState(SceneData& data) const;
    void loadState(const SceneData& data);

private:
    latex::Options m_options;
    bool m_preview = false;
    latex::Result m_last;      // last generated code, shown in the panel
    double m_lastTime = -1.0;  // regenerated a few times per second, not every frame
};

} // namespace linalg
