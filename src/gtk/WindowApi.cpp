#include "gtk/WindowApi.hpp"
#include "gtk/InfoPane.hpp"
#include "gtk/MainWindow.hpp"
#include "gtk/SceneArea.hpp"

namespace linalg::gtk {

Session& WindowApi::session() { return m_window.session(); }

void WindowApi::setMode(AppMode mode) { m_window.setSpace(mode == AppMode::Complex ? "complex" : "space3d"); }

void WindowApi::showPanel(const std::string& id, bool visible) {
    if (id == "gallery") {
        if (visible) m_window.showGallery();
        else if (m_window.galleryOpen()) m_window.closeDialogs();
        return;
    }
    bool pane = id == "analysis" || id == "steps" || id == "breakdown";
    if (!pane) return;  // the clock is always shown; other panels don't exist here (yet)
    if (visible) m_window.showPane(id);
    else if (panelVisible(id)) m_window.showPane("");
}

bool WindowApi::panelVisible(const std::string& id) const {
    if (id == "animation") return true;
    if (id == "gallery") return m_window.galleryOpen();
    InfoPane& pane = m_window.infoPane();
    return pane.isOpen() && pane.page() == id;
}

bool WindowApi::cameraMovedThisFrame() const { return m_window.sceneArea().cameraMovedRecently(); }

void WindowApi::restartBreakdown() { m_window.infoPane().breakdown().restart(); }

bool WindowApi::breakdownFinished() const { return m_window.infoPane().breakdown().finished(); }

void WindowApi::toast(const std::string& message, bool) { m_window.toast(message); }

} // namespace linalg::gtk
