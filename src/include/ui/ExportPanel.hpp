#pragma once

#include "export/LatexExport.hpp"
#include "export/PdfCompile.hpp"
#include "ui/SceneIO.hpp"
#include <raylib.h>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace linalg {

// "Export to LaTeX" panel: settings for the black-and-white PGFPlots figure,
// a live preview of the code, and Copy / Save buttons. Not modal, so the
// scene can be adjusted while it is open.
class ExportPanel {
public:
    struct Actions {
        bool copy = false;  // copy the code to the clipboard
        bool save = false;  // open the Save .tex dialog
        bool savePdf = false;  // open the Save PDF dialog
    };

    ExportPanel();
    ~ExportPanel();  // waits for a running PDF compile
    ExportPanel(const ExportPanel&) = delete;
    ExportPanel& operator=(const ExportPanel&) = delete;

    // PDF: compile `standaloneCode` to `path` in the background (ignored while one is running)
    bool pdfAvailable() const { return !m_compiler.empty(); }
    bool pdfRunning() const { return m_pdfRunning; }
    void startPdf(const std::string& standaloneCode, const std::string& path);
    // A finished compile (once): true with its result and target path
    bool takePdfResult(latex::PdfResult& result, std::string& path);

    // Returns the content height (pixels). `azimuth`/`elevation` come from the camera.
    float drawContent(Rectangle area, const scene::Space& space, const scene::Selection& selection, double azimuth,
                      double elevation, Actions& actions);

    // The code for the current settings (also used by Copy/Save)
    latex::Result generate(const scene::Space& space, const scene::Selection& selection, double azimuth,
                           double elevation, bool forceStandalone = false) const;

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

    std::string m_compiler;            // pdflatex/lualatex path, "" when missing (found once)
    std::thread m_pdfThread;
    std::atomic<bool> m_pdfRunning{false};
    std::mutex m_pdfMutex;             // guards the fields below
    bool m_pdfDone = false;
    latex::PdfResult m_pdfResult;
    std::string m_pdfPath;
    std::string m_pdfError;            // last failure, shown in the panel
};

} // namespace linalg
