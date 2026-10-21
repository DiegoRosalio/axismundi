#include "gtk/Tour.hpp"
#include "gtk/Capture.hpp"
#include "gtk/MainWindow.hpp"
#include "gtk/SceneArea.hpp"
#include "gtk/SceneRenderer.hpp"
#include "scene/Templates.hpp"
#include <gtk/gtk.h>
#include <filesystem>
#include <iostream>

namespace linalg::gtk {

Tour::Tour(MainWindow& window, Glib::RefPtr<Gtk::Application> app, std::string directory)
    : m_window(window), m_app(std::move(app)), m_directory(std::move(directory)) {
    SceneRenderer& r = m_window.sceneArea().renderer();
    Session& s = m_window.session();
    float h = 600.0f;
    m_steps = {
        {"01_gtk_space3d", [] {}},
        {"02_gtk_select", [this, &s] {
             s.select("a");
             m_window.sceneChanged();
         }},
        {"03_gtk_template", [this, &s] {
             s.select("b", AppMode::Space3D, true);
             for (const auto& [label, choice] : addTemplates(s.scene(), s.spaceKind()))
                 if (label.rfind("Cross product", 0) == 0) m_window.addObject(choice);
         }},
        {"04_gtk_undo", [this] { m_window.undo(); }},
        {"05_gtk_add_menu", [this] { m_window.openAddMenu(); }},
        {"06_gtk_orbit", [this, &r, h] {
             m_window.closeAddMenu();
             r.orbit(160.0f, 50.0f, h);
         }},
        {"07_gtk_pan_zoom", [&r, h] {
             r.pan(-80.0f, 30.0f, h);
             r.zoom(3.0f, h);
         }},
        {"08_gtk_sidebar_hidden", [this] { m_window.showSidebar(false); }},
        {"09_gtk_complex", [this] {
             m_window.showSidebar(true);
             m_window.setSpace("complex");
         }},
        {"10_gtk_light", [this] {
             m_window.setSpace("space3d");
             m_window.setTheme("light");
         }},
    };
}

void Tour::start() {
    std::filesystem::create_directories(m_directory);
    m_window.sceneArea().setFixedStep(1.0 / 60.0);
    gtk_widget_add_tick_callback(
        m_window.widget(),
        [](GtkWidget*, GdkFrameClock*, gpointer self) -> gboolean { return static_cast<Tour*>(self)->tick(); },
        this, nullptr);
}

bool Tour::tick() {
    SceneArea& area = m_window.sceneArea();
    if (m_next >= m_steps.size()) {
        m_app->quit();
        return G_SOURCE_REMOVE;
    }
    if (!m_waiting) {
        m_steps[m_next].action();
        area.changed();
        m_framesAtAction = area.framesDrawn();
        m_ticks = 0;
        m_waiting = true;
        return G_SOURCE_CONTINUE;
    }
    // A third of a second after the view drew the change (popovers and toasts animate in), or after ~5 s if it
    // can't draw, save the window
    m_ticks++;
    bool drawn = area.framesDrawn() >= m_framesAtAction + 2;
    if ((drawn && m_ticks >= 20) || m_ticks > 300) {
        std::string path = m_directory + "/" + m_steps[m_next].name + ".png";
        bool ok = saveWidgetPng(m_window.widget(), path);
        std::cerr << (ok ? "TOUR " : "TOUR FAILED ") << path
                  << (area.problem().empty() ? "" : "  (3D view: " + area.problem() + ")") << "\n";
        m_next++;
        m_waiting = false;
    }
    return G_SOURCE_CONTINUE;
}

} // namespace linalg::gtk
