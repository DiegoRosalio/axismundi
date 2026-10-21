#pragma once

// The View button in the header bar: a popover with what the 3D view shows (grid, axes, labels, numbers, the
// grid's spacing and planes or polar grid), the view style (Colorful, Paper, High contrast, Chalkboard), the axis
// length, and the camera (reset, front, right, top, focus on the selection). It edits the Session's ViewSettings,
// which are saved in scene files.

#include <gtkmm/menubutton.h>
#include <functional>

namespace linalg {
class Session;
}

namespace linalg::gtk {

class ViewMenu : public Gtk::MenuButton {
public:
    explicit ViewMenu(Session& session);

    // A setting changed: the view must draw again
    void onChanged(std::function<void()> callback) { m_changed = std::move(callback); }
    // A camera button: 0 reset, 1 front, 2 right, 3 top, 4 focus on the selection
    void onCamera(std::function<void(int)> callback) { m_camera = std::move(callback); }

private:
    void rebuild();  // when the popover opens (the controls depend on the space)
    void changed();

    Session& m_session;
    std::function<void()> m_changed;
    std::function<void(int)> m_camera;
};

} // namespace linalg::gtk
