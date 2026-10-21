#pragma once

// The GTK app's AppApi: what gallery examples (and later the tutorial) may do to the window. Scene work goes to
// the Session; panels map to the window's parts ("analysis", "steps", "breakdown" are pages of the pane on the
// right; the clock, "animation", is always shown under the view).

#include "app/AppApi.hpp"

namespace linalg::gtk {

class MainWindow;

class WindowApi : public AppApi {
public:
    explicit WindowApi(MainWindow& window) : m_window(window) {}

    Session& session() override;
    void setMode(AppMode mode) override;
    void showPanel(const std::string& id, bool visible) override;
    bool panelVisible(const std::string& id) const override;
    bool uiMode() const override { return true; }  // no fly camera in the GTK app yet
    bool cameraMovedThisFrame() const override;
    void restartBreakdown() override;
    bool breakdownFinished() const override;
    void toast(const std::string& message, bool error = false) override;

private:
    MainWindow& m_window;
};

} // namespace linalg::gtk
