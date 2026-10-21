#pragma once

// The GTK app's main window (an AdwApplicationWindow): a header bar with the 3D space / complex plane switch,
// and the 3D view. libadwaita has no C++ wrapper, so its widgets are made through its C API and our own
// widgets (gtkmm) are put inside them.

#include "app/Session.hpp"
#include <gtkmm/application.h>
#include <string>

typedef struct _GtkWidget GtkWidget;
typedef struct _AdwToggleGroup AdwToggleGroup;

namespace linalg::gtk {

class SceneArea;

class MainWindow {
public:
    MainWindow(const Glib::RefPtr<Gtk::Application>& app, int width, int height);

    void present();
    GtkWidget* widget() const { return m_window; }
    SceneArea& sceneArea() { return *m_area; }
    Session& session() { return m_session; }

    // Space switch: "space3d" or "complex"
    void setSpace(const std::string& name);
    // Dark or light: follow the system (empty), or force "dark" / "light"
    void setTheme(const std::string& theme);

private:
    static void onSpaceChanged(AdwToggleGroup* group, void* pspec, MainWindow* self);
    static void onDarkChanged(void* manager, void* pspec, MainWindow* self);
    void applyDark();

    Session m_session;
    GtkWidget* m_window = nullptr;      // owned by GTK (the application's window list)
    AdwToggleGroup* m_spaces = nullptr;
    SceneArea* m_area = nullptr;        // a managed gtkmm widget inside the window
};

} // namespace linalg::gtk
