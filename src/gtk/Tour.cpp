#include "gtk/Tour.hpp"
#include "gtk/Capture.hpp"
#include "gtk/Inspector.hpp"
#include "gtk/FormulaDialog.hpp"
#include "gtk/InfoPane.hpp"
#include "gtk/MainWindow.hpp"
#include "gtk/SceneArea.hpp"
#include "gtk/SceneRenderer.hpp"
#include "edu/Examples.hpp"
#include "scene/Templates.hpp"
#include "scene/Types.hpp"
#include <gtk/gtk.h>
#include <filesystem>
#include <iostream>

namespace linalg::gtk {

Tour::Tour(MainWindow& window, Glib::RefPtr<Gtk::Application> app, std::string directory)
    : m_window(window), m_app(std::move(app)), m_directory(std::move(directory)) {
    SceneRenderer& r = m_window.sceneArea().renderer();
    Session& s = m_window.session();
    float h = 600.0f;
    std::string theme = m_window.theme();  // the tour's own theme (--theme), for the steps after the light one
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
        // The inspector (run 5)
        {"11_gtk_vector_inspector", [this, &s, theme] {
             m_window.setTheme(theme);
             s.select("a");
             m_window.sceneChanged();
         }},
        {"12_gtk_matrix_inspector", [this, &s] {
             s.select("M");
             m_window.sceneChanged();
         }},
        {"13_gtk_matrix_formula", [this, &s] {
             if (auto* m = dynamic_cast<scene::MatrixObject*>(s.object("M"))) m->setByFormula(true);
             s.scene().commit();
             m_window.sceneChanged();
         }},
        {"14_gtk_parameter_slider", [this] { m_window.addObject(AddChoice{"parameter", "theta", {}, {}, ""}); }},
        {"15_gtk_parameter_animate", [this, &s] {
             if (auto* p = dynamic_cast<scene::ParameterObject*>(s.object("theta")))
                 p->setMode(scene::ParameterObject::Mode::Animate);
             s.scene().commit();
             m_window.sceneChanged();
         }},
        {"16_gtk_surface_inspector", [this] { m_window.addObject(AddChoice{"surface", "", {}, {}, ""}); }},
        {"17_gtk_field_error", [this, &s] {
             s.select("a");
             if (scene::SceneObject* a = s.object("a")) a->setField("y", "q + 1");
             s.scene().commit();
             m_window.sceneChanged();
         }},
        {"18_gtk_complex_inspector", [this, &s] {
             m_window.setSpace("complex");
             scene::Space& space = s.scene().space(scene::SpaceKind::Complex);
             for (const auto& o : space.objects())
                 if (o->typeId() == "complex") {
                     s.select(o->name(), AppMode::Complex);
                     break;
                 }
             m_window.sceneChanged();
         }},
        {"19_gtk_inspector_light", [this, &s] {
             m_window.setSpace("space3d");
             m_window.setTheme("light");
             s.select("M");
             m_window.sceneChanged();
         }},
    };
    // The other types' inspectors (run 6): each alone in its space
    const char* more[][3] = {
        {"20_gtk_quaternion", "quaternion", "space3d"},   {"21_gtk_field", "field", "space3d"},
        {"22_gtk_double_integral", "doubleintegral", "space3d"}, {"23_gtk_line_integral", "lineintegral", "space3d"},
        {"24_gtk_frame", "frame", "space3d"},             {"25_gtk_tangent", "tangent", "space3d"},
        {"26_gtk_tangent_plane", "tangentplane", "space3d"}, {"27_gtk_velocity", "velocity", "space3d"},
        {"28_gtk_taylor", "taylor", "space3d"},           {"29_gtk_newton", "newton", "space3d"},
        {"30_gtk_fourier_series", "fourierseries", "space3d"}, {"31_gtk_epicycles", "epicycles", "complex"},
    };
    for (auto& m : more) {
        std::string type = m[1], space = m[2];
        m_steps.push_back({m[0], [this, &s, type, space, theme] {
                               m_window.setTheme(theme);
                               m_window.setSpace(space);
                               s.clearSpace(space == "complex" ? AppMode::Complex : AppMode::Space3D);
                               m_window.addObject(AddChoice{type, "", {}, {}, ""});
                           }});
        // A switch that adds a row: the vector the rotation acts on
        if (type == "quaternion") {
            m_steps.push_back({"20b_gtk_quaternion_acts", [this, &s] {
                                   if (scene::SceneObject* q = s.object("q")) q->setProp("act", true);
                                   s.scene().commit();
                                   m_window.sceneChanged();
                               }});
            m_steps.push_back({"20c_gtk_quaternion_acts_end", [this] { m_window.inspector().scrollToEnd(); }});
        }
        // Long inspectors: their last rows too (Newton's steps, the Fourier spectrum)
        if (type == "newton" || type == "fourierseries")
            m_steps.push_back({std::string(m[0]) + "_end", [this] { m_window.inspector().scrollToEnd(); }});
    }
    // The pane on the right (run 7), with the demo scene again
    m_steps.push_back({"40_gtk_analysis", [this, &s] {
                           m_window.setSpace("space3d");
                           s.resetScene();
                           s.select("a");
                           s.select("b", AppMode::Space3D, true);
                           m_window.showPane("analysis");
                       }});
    m_steps.push_back({"41_gtk_analysis_matrix", [this, &s] {
                           s.select("M");
                           m_window.sceneChanged();
                       }});
    m_steps.push_back({"42_gtk_steps", [this] { m_window.showPane("steps"); }});
    m_steps.push_back({"43_gtk_breakdown", [this, &s] {
                           if (auto* m = dynamic_cast<scene::MatrixObject*>(s.object("M"))) {
                               m->applyPreset(Matrix3D::rotation(0.6f));
                               s.scene().evaluate();  // presets act on the current value
                               m->applyPreset(Matrix3D::scale(1.8f, 0.7f));
                           }
                           s.scene().commit();
                           m_window.showPane("breakdown");
                       }});
    m_steps.push_back({"44_gtk_breakdown_end", [this] { m_window.infoPane().breakdown().seek(100.0f); }});
    m_steps.push_back({"45_gtk_steps_complex", [this] {
                           m_window.setSpace("complex");
                           m_window.showPane("steps");
                       }});
    m_steps.push_back({"46_gtk_view_menu", [this] {
                           m_window.showPane("");
                           m_window.setSpace("space3d");
                           m_window.openViewMenu(true);
                       }});
    m_steps.push_back({"47_gtk_view_paper_walls", [this, &s] {
                           m_window.openViewMenu(false);
                           ViewSettings& v = s.view();
                           v.style = view::StyleId::Paper;
                           v.gridWallXZ = true;
                           v.gridAuto = false;
                       }});
    m_steps.push_back({"48_gtk_time_playing", [this, &s] {
                           ViewSettings& v = s.view();
                           v.style = view::StyleId::Colorful;
                           v.gridWallXZ = false;
                           v.gridAuto = true;
                           m_window.addObject(AddChoice{"parameter", "k", {}, {{"mode", "animate"}}, ""});
                           s.scene().timeline().time = 1.5;
                           s.scene().timeline().playing = true;
                       }});
    m_steps.push_back({"49_gtk_preferences", [this, &s] {
                           s.scene().timeline().playing = false;
                           m_window.showPreferences();
                       }});
    m_steps.push_back({"50_gtk_reference", [this] {
                           m_window.closeDialogs();
                           m_window.showReference();
                       }});
    m_steps.push_back({"51_gtk_gallery", [this] {
                           m_window.closeDialogs();
                           m_window.showGallery();
                       }});
    m_steps.push_back({"52_gtk_example", [this] {
                           m_window.closeDialogs();
                           const Example& e = examples()[1];
                           e.apply(m_window.api());
                           m_window.sceneChanged();
                       }});
    m_steps.push_back({"53_gtk_export", [this] { m_window.showExport(); }});
    m_steps.push_back({"54_gtk_shortcuts", [this] {
                           m_window.closeDialogs();
                           m_window.showShortcuts();
                       }});
    m_steps.push_back({"55_gtk_formula_editor", [this, &s] {
                           m_window.closeDialogs();
                           m_window.setSpace("space3d");
                           s.resetScene();
                           s.select("c");
                           m_window.sceneChanged();
                           const scene::SceneObject* c = s.object("c");
                           if (c)
                               showFormulaDialog(m_window.widget(), s, scene::SpaceKind::Euclidean3D, c->id(), "value",
                                                 "cross(a, b) + 2*normalize(a)", [](const std::string&) {});
                       }});
    m_steps.push_back({"56_gtk_custom_color", [this, &s] {
                           m_window.closeDialogs();
                           if (scene::SceneObject* c = s.object("c")) {
                               c->customColor = true;
                               c->color = {230, 97, 0, 255};
                               c->touch();
                           }
                           s.scene().commit();
                           m_window.sceneChanged();
                       }});
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
