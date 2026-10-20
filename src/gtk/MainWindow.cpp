#include "gtk/MainWindow.hpp"
#include "gtk/SceneArea.hpp"
#include "gtk/SceneRenderer.hpp"
#include "ui/core/Theme.hpp"
#include <adwaita.h>

namespace linalg::gtk {

MainWindow::MainWindow(const Glib::RefPtr<Gtk::Application>& app, int width, int height) {
    m_window = adw_application_window_new(app->gobj());
    gtk_window_set_title(GTK_WINDOW(m_window), "axismundi");
    gtk_window_set_default_size(GTK_WINDOW(m_window), width, height);

    // The header bar: the space switch as its title
    GtkWidget* header = adw_header_bar_new();
    m_spaces = ADW_TOGGLE_GROUP(adw_toggle_group_new());
    const char* spaces[][2] = {{"space3d", "3D space"}, {"complex", "Complex plane"}};
    for (auto& s : spaces) {
        AdwToggle* toggle = adw_toggle_new();
        adw_toggle_set_name(toggle, s[0]);
        adw_toggle_set_label(toggle, s[1]);
        adw_toggle_group_add(m_spaces, toggle);
    }
    adw_toggle_group_set_active_name(m_spaces, "space3d");
    g_signal_connect(m_spaces, "notify::active-name", G_CALLBACK(onSpaceChanged), this);
    adw_header_bar_set_title_widget(ADW_HEADER_BAR(header), GTK_WIDGET(m_spaces));

    m_area = Gtk::make_managed<SceneArea>(m_scene);

    GtkWidget* toolbar = adw_toolbar_view_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), header);
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), GTK_WIDGET(m_area->gobj()));
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(m_window), toolbar);

    g_signal_connect(adw_style_manager_get_default(), "notify::dark", G_CALLBACK(onDarkChanged), this);
    applyDark();
}

void MainWindow::present() { gtk_window_present(GTK_WINDOW(m_window)); }

void MainWindow::setSpace(const std::string& name) {
    adw_toggle_group_set_active_name(m_spaces, name.c_str());  // onSpaceChanged does the rest
}

void MainWindow::onSpaceChanged(AdwToggleGroup* group, void*, MainWindow* self) {
    const char* name = adw_toggle_group_get_active_name(group);
    bool complex = name && std::string(name) == "complex";
    self->m_area->renderer().setMode(complex ? AppMode::Complex : AppMode::Space3D);
    self->m_area->changed();
}

void MainWindow::setTheme(const std::string& theme) {
    AdwColorScheme scheme = theme == "dark"    ? ADW_COLOR_SCHEME_FORCE_DARK
                            : theme == "light" ? ADW_COLOR_SCHEME_FORCE_LIGHT
                                               : ADW_COLOR_SCHEME_DEFAULT;
    adw_style_manager_set_color_scheme(adw_style_manager_get_default(), scheme);
}

void MainWindow::onDarkChanged(void*, void*, MainWindow* self) { self->applyDark(); }

// The 3D view follows the window's dark/light look (the system setting, or --theme)
void MainWindow::applyDark() {
    bool dark = adw_style_manager_get_dark(adw_style_manager_get_default());
    ui::setTheme(dark ? ui::ThemeKind::Dark : ui::ThemeKind::Light);
    m_area->changed();
}

} // namespace linalg::gtk
