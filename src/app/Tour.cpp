#include "app/Tour.hpp"
#include "app/App.hpp"
#include "edu/Examples.hpp"
#include "edu/Tutorial.hpp"
#include "ui/AddMenu.hpp"
#include <raylib.h>
#include <rlgl.h>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include "ui/core/Ui.hpp"

namespace linalg {

Tour::Tour(std::string outputDir) : m_dir(std::move(outputDir)) {
    buildSteps();
}

namespace {

// Put the mouse on a named control from the last frame (see ui::anchor)
void hover(const char* anchor) {
    Rectangle r;
    if (ui::findAnchor(anchor, r)) SetMousePosition(static_cast<int>(r.x + r.width / 2), static_cast<int>(r.y + r.height / 2));
}

void park() { SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2); }

int exampleIndex(const char* title) {
    const auto& list = examples();
    for (size_t i = 0; i < list.size(); i++)
        if (std::string(list[i].title) == title) return static_cast<int>(i);
    return 0;
}

void runExample(App& app, const char* title) {
    const Example& e = examples()[exampleIndex(title)];
    e.apply(app);
}

} // namespace

void Tour::buildSteps() {
    auto step = [this](const char* name, std::function<void(App&)> action, int frames = 12) {
        m_steps.push_back({name, std::move(action), frames});
    };

    // --- 3D space and objects ---------------------------------------------------
    step("00_empty_start", [](App& app) {
        app.setUiMode(true);
        park();
        app.setMode(AppMode::Space3D);
        app.newScene();
    });
    step("00b_save_dialog", [](App& app) { app.saveSceneAs(); }, 15);
    std::string dir = m_dir;
    step("00c_save_and_open", [dir](App& app) {
        // Round trip through a real file: save the demo scene, clear, open it again
        app.fileDialog().close();
        app.resetScene();
        app.writeScene(dir + "/tour_scene.txt");
        app.newScene();
        app.readScene(dir + "/tour_scene.txt");
    }, 15);
    step("01_space_default", [](App& app) { app.resetScene(); });
    step("02_vector_inspector", [](App& app) { app.select("a"); });
    step("02b_long_formula", [](App& app) {
        app.addObject(AppMode::Space3D, "vector", "w",
                      {{"value", "normalize(cross(a, b) + 2*a - b/3) * (1 + 0.5*sin(t)) + rotz(pi/6)*[1, 0, 0]"}},
                      {{"mode", "formula"}});
        app.select("w");
    });
    step("02c_formula_editor", [](App& app) { app.openFormulaEditor("w", "value"); }, 15);
    step("02d_formula_editor_error", [](App& app) {
        app.formulaEditor().close();
        app.openFormulaEditor("w", "value");
        // A typo shows the error live (not applied)
        const_cast<std::string&>(app.formulaEditor().text()) += " + q";
    }, 15);
    step("02e_sidebar_collapsed", [](App& app) {
        app.formulaEditor().close();
        app.setSideBarCollapsed(true);
    }, 15);
    step("03_add_menu_templates", [](App& app) {
        app.setSideBarCollapsed(false);
        app.select("a");
        app.select("b", AppMode::Space3D, true);
        app.openAddMenu();
    });
    step("04_many_objects", [](App& app) {
        app.closePopovers();
        app.addObject(AppMode::Space3D, "vector", "p", {{"value", "dot(a, b + [1, 1, 0])/2 * normalize(a + b)"}},
                      {{"mode", "formula"}});
        app.addObject(AppMode::Space3D, "vector", "w", {{"x", "-1"}, {"y", "1.5"}, {"z", "2"}});
        app.addObject(AppMode::Space3D, "vector", "n", {{"value", "cross(w, a)"}, {"tail", "w"}}, {{"mode", "formula"}});
    });
    step("05_expression_error", [](App& app) {
        app.addObject(AppMode::Space3D, "vector", "q", {{"value", "a + missing"}}, {{"mode", "formula"}});
    });
    step("06_multi_select_analysis", [](App& app) {
        if (auto* q = app.object("q")) app.space(AppMode::Space3D).remove(q->id());
        app.select("a");
        app.select("w", AppMode::Space3D, true);
    });
    step("06b_solo_selection", [](App& app) { app.view().soloSelection = true; });
    step("07_parameter_rotation", [](App& app) {
        app.view().soloSelection = false;
        runExample(app, "Animated rotation");
    }, 50);
    step("07b_parameter_animate", [](App& app) {
        auto* p = dynamic_cast<scene::ParameterObject*>(app.object("theta"));
        if (p) {
            Playback pb = p->playback();
            pb.repeat = Repeat::PingPong;
            pb.easing = Easing::Bounce;
            p->setPlayback(pb);
        }
        app.scenePanel().setEasingOpen(true);
        app.select("theta");
    }, 40);
    step("07c_parameter_formula", [](App& app) {
        app.scenePanel().setEasingOpen(false);
        auto* p = dynamic_cast<scene::ParameterObject*>(app.object("theta"));
        if (p) {
            p->setMode(scene::ParameterObject::Mode::Formula);
            p->setField("formula", "pi/4*sin(2t)");
        }
        app.restartTime(true);
        app.select("theta");
    }, 50);
    step("08_matrix_formula_4x4", [](App& app) {
        runExample(app, "Move and turn (4×4)");
        app.select("M");
    });
    step("09_matrix_cells", [](App& app) {
        app.resetScene();
        app.select("M");
        app.applyMatrixPreset(Matrix3D({{{{1, 1, 0}}, {{0, 1, 0}}, {{0, 0, 2}}}}));
    }, 30);
    step("09a_presets_add_up", [](App& app) {
        app.resetScene();
        app.select("M");
        app.applyMatrixPreset(Matrix3D::rotation(3.14159265f / 4));
        app.applyMatrixPreset(Matrix3D::scale(2, 1));
    }, 20);
    step("09b_static_morph", [](App& app) {
        // Add menu template: a static slider s and lerp(eye(3), M, s)
        app.clearSpace(AppMode::Space3D);
        app.addObject(AppMode::Space3D, "matrix", "M", {{"value", "[[1, 1, 0], [0, 1, 0], [0, 0, 2]]"}},
                      {{"mode", "formula"}});
        app.select("M");
        for (const auto& [label, choice] : AddMenu::templates(app.scene(), scene::SpaceKind::Euclidean3D))
            if (label.rfind("Morph from identity", 0) == 0) applyAddChoice(app.scene(), scene::SpaceKind::Euclidean3D, choice);
        app.select("s");
    }, 30);
    step("09c_pendulum_time_panel", [](App& app) {
        runExample(app, "Pendulum (formula of time)");
        if (auto* p = app.panel("animation")) p->setVisible(true);
    }, 50);
    step("10_projection_example", [](App& app) {
        runExample(app, "Projection of a vector");
    });

    // --- Complex plane ------------------------------------------------------------
    step("11_complex_default", [](App& app) {
        app.setMode(AppMode::Complex);
        app.select("z1", AppMode::Complex);
    });
    step("12_complex_roots", [](App& app) { runExample(app, "Powers of a root of unity"); });
    step("13_complex_product", [](App& app) {
        runExample(app, "Rotate and scale");
        app.select("w", AppMode::Complex);
    });

    // --- Plots ----------------------------------------------------------------------
    step("14_plot_ripple_slider", [](App& app) { runExample(app, "Ripple with a slider"); });
    step("14b_plot_wide_dense", [](App& app) {
        app.clearSpace(AppMode::Space3D);
        app.addObject(AppMode::Space3D, "surface", "S",
                      {{"f", "2sin(sqrt(x^2 + y^2))/(1 + 0.05(x^2 + y^2))"}, {"xmin", "-20"}, {"xmax", "20"},
                       {"ymin", "-20"}, {"ymax", "20"}},
                      {{"density", "12"}});
        app.setCameraView({22, 18, 26}, {0, 0, 0});
    }, 40);
    step("14b2_plot_faceted", [](App& app) {
        runExample(app, "Ripple with a slider");
        app.setSurfaces(UiSettings::Surfaces::Faceted);
    }, 20);
    step("14c_large_grid", [](App& app) {
        app.setSurfaces(UiSettings::Surfaces::StyleDefault);
        app.view().gridSize = 60.0f;
        app.select("S");
        app.setCameraView({45, 30, 55}, {0, 0, 0});
    }, 30);
    // View styles (View section): demo scene, a surface, the complex plane, paper
    step("14d_style_contrast", [](App& app) {
        app.view().gridSize = 10.0f;
        app.resetScene();
        app.setCameraView({6.5f, 5.0f, 7.5f}, {0, 0, 0});
        app.select("a");
        app.view().style = view::StyleId::HighContrast;
    }, 20);
    step("14e_style_chalkboard", [](App& app) {
        runExample(app, "Saddle");
        app.view().style = view::StyleId::Chalkboard;
    }, 25);
    step("14f_style_chalkboard_complex", [](App& app) { runExample(app, "Rotate and scale"); }, 25);
    step("14g_style_paper", [](App& app) {
        app.setMode(AppMode::Space3D);
        app.resetScene();
        app.view().style = view::StyleId::Paper;
    }, 20);
    step("15_plot_implicit", [](App& app) {
        app.view().style = view::StyleId::Colorful;
        app.view().gridSize = 10.0f;
        runExample(app, "Lemniscate");
    });
    step("16_plot_curve", [](App& app) {
        runExample(app, "Trefoil knot");
        app.select("f");
    });

    // --- Tools ----------------------------------------------------------------------
    step("16b_travelling_wave", [](App& app) { runExample(app, "Travelling wave"); }, 30);
    step("16c_travelling_wave_later", [](App&) {}, 30);
    step("16d_spinning_vector", [](App& app) { runExample(app, "Spinning vector"); }, 40);
    // Vector fields: plane with flow lines, particles at two times, a volume, styles
    step("16e_field_rotation", [](App& app) {
        runExample(app, "Rotation field");
        app.select("F");
    }, 20);
    step("16f_field_particles", [](App& app) {
        runExample(app, "Flow with particles");
        app.select("F");
    }, 30);
    step("16g_field_particles_later", [](App&) {}, 60);
    step("16h_field_volume", [](App& app) {
        runExample(app, "3D swirl");
        app.select("F");
    }, 25);
    step("16i_field_source_sink_chalk", [](App& app) {
        runExample(app, "Source and sink");
        app.view().style = view::StyleId::Chalkboard;
    }, 25);
    step("16j_field_paper", [](App& app) { app.view().style = view::StyleId::Paper; }, 20);
    // Quaternions: axis and angle with the rotated basis, then acting on a vector
    step("16k_quaternion", [](App& app) {
        app.view().style = view::StyleId::Colorful;
        app.setMode(AppMode::Space3D);
        app.clearSpace(AppMode::Space3D);
        app.resetCamera();
        app.addObject(AppMode::Space3D, "quaternion", "q", {{"value", "qaxis([1, 1, 1], 2pi/3)"}});
        app.select("q");
        app.setCameraView({4.2f, 3.4f, 4.8f}, {0, 0.6f, 0});
    }, 20);
    step("16l_quaternion_acts", [](App& app) {
        if (auto* q = app.object("q")) {
            q->setField("value", "qaxis([0, 0, 1], pi/2)");
            q->setProp("act", true);
            q->setProp("basis", false);
            q->setField("v", "[1.5, 0, 1]");
        }
        app.scene().commit();
    }, 20);
    step("16m_quaternion_paper", [](App& app) { app.view().style = view::StyleId::Paper; }, 20);
    step("16n_quaternion_pair", [](App& app) {
        app.view().style = view::StyleId::Colorful;
        app.clearSpace(AppMode::Space3D);
        app.addObject(AppMode::Space3D, "quaternion", "q1", {{"value", "qaxis([0, 0, 1], pi/2)"}}, {{"basis", "0"}});
        app.addObject(AppMode::Space3D, "quaternion", "q2", {{"value", "qaxis([1, 0, 0], pi/2)"}}, {{"basis", "0"}});
        app.select("q1");
        app.select("q2", AppMode::Space3D, true);
    }, 20);
    step("16o_quaternion_templates", [](App& app) { app.openAddMenu(); }, 15);
    step("16p_quaternion_order", [](App& app) {
        app.closePopovers();
        runExample(app, "Order matters (quaternions)");
    }, 20);
    step("16q_quaternion_slerp", [](App& app) { runExample(app, "Slerp between two rotations"); }, 70);
    // Fourier: a square wave from sines, Gibbs, epicycles drawing a heart and a square
    step("16fa_fourier_square", [](App& app) { runExample(app, "Square wave from sines"); }, 20);
    step("16fb_fourier_gibbs", [](App& app) { runExample(app, "Gibbs overshoot"); }, 20);
    step("16fc_epicycles_heart", [](App& app) { runExample(app, "Epicycles draw a heart"); }, 200);
    step("16fd_epicycles_square", [](App& app) { runExample(app, "Epicycles draw a square"); }, 150);
    step("16fe_epicycles_paper", [](App& app) { app.view().style = view::StyleId::Paper; }, 10);
    step("16ff_fourier_reset", [](App& app) {
        app.view().style = view::StyleId::Colorful;
        app.setMode(AppMode::Space3D);
    }, 2);
    // Curves: the Frenet frame on a helix, the comb of an ellipse, an osculating circle
    step("16x_frame_helix", [](App& app) { runExample(app, "Frenet frame on a helix"); }, 40);
    step("16y_frame_ellipse", [](App& app) { runExample(app, "Curvature of an ellipse"); }, 20);
    step("16z_frame_parabola", [](App& app) { runExample(app, "Osculating circle on a parabola"); }, 50);
    // Integrals: volume under a surface, refining boxes, work along paths
    step("16r_integral_volume", [](App& app) { runExample(app, "Volume under a paraboloid"); }, 20);
    step("16s_integral_refine", [](App& app) { runExample(app, "Riemann boxes refine"); }, 120);
    step("16t_integral_work", [](App& app) { runExample(app, "Work around a circle"); }, 90);
    step("16u_integral_paths", [](App& app) { runExample(app, "Path independence"); }, 20);
    step("16v_integral_paper", [](App& app) {
        runExample(app, "Volume under a paraboloid");
        app.view().style = view::StyleId::Paper;
    }, 20);
    step("16w_integral_reset", [](App& app) { app.view().style = view::StyleId::Colorful; }, 2);
    step("17_help", [](App& app) {
        app.closePopovers();
        app.view().style = view::StyleId::Colorful;
        app.resetScene();
        app.setMode(AppMode::Space3D);
        app.showHelp(true);
    });
    step("18_tooltip_preset", [](App& app) {
        app.showHelp(false);
        app.select("M");
    }, 4);
    step("18b_tooltip_preset", [](App&) { hover("preset.rotate"); }, 45);
    step("19_steps_det", [](App& app) {
        park();
        app.setMatrix3(Matrix3D({{{{2, 1, 0}}, {{1, 3, 1}}, {{0, 1, 4}}}}));
        app.select("M");
        if (auto* p = app.panel("steps")) p->setVisible(true);
        if (auto* p = app.panel("analysis")) p->setVisible(false);
    });
    step("20_breakdown", [](App& app) {
        if (auto* p = app.panel("steps")) p->setVisible(false);
        app.setMatrix3(Matrix3D({{{{1, 0.8f, 0}}, {{0, 1, 0}}, {{0, 0, 1.4f}}}}));
        if (auto* p = app.panel("breakdown")) p->setVisible(true);
    }, 150);
    step("21_gallery", [](App& app) {
        if (auto* p = app.panel("breakdown")) p->setVisible(false);
        if (auto* p = app.panel("analysis")) p->setVisible(true);
        if (auto* p = app.panel("gallery")) p->setVisible(true);
    });
    // LaTeX export: the panel, the Save .tex dialog, and standalone figures for several scenes
    auto saveLatex = [dir](App& app, const char* name) {
        std::filesystem::create_directories(dir + "/latex");
        bool standalone = app.exportPanel().options().standalone;
        app.exportPanel().options().standalone = true;
        std::ofstream(dir + "/latex/" + name + ".tex") << app.exportLatexCode();
        app.exportPanel().options().standalone = standalone;
    };
    step("21a_export_panel", [saveLatex](App& app) {
        if (auto* p = app.panel("gallery")) p->setVisible(false);
        app.resetScene();
        if (auto* p = app.panel("export")) p->setVisible(true);
        saveLatex(app, "default3d");
    }, 25);
    step("21a2_export_save_dialog", [](App& app) {
        app.fileDialog().open(FileDialog::Mode::Save, ".", "figure", {}, FileDialog::Kind::Latex);
    }, 10);
    step("21a3_export_examples", [saveLatex](App& app) {
        app.fileDialog().close();
        const char* names[][2] = {{"Saddle", "saddle"}, {"Lemniscate", "lemniscate"}, {"Trefoil knot", "trefoil"},
                                  {"Damped wave", "damped_wave"}, {"Projection of a vector", "projection"},
                                  {"Shear", "shear"}, {"Powers of a root of unity", "roots_of_unity"},
                                  {"Rotate and scale", "rotate_scale"}, {"Source and sink", "source_sink"},
                                  {"3D swirl", "swirl"}, {"Quarter turn (quaternion)", "quaternion"},
                                  {"Volume under a paraboloid", "volume"}, {"Path independence", "work"},
                                  {"Frenet frame on a helix", "frenet"}};
        for (const auto& n : names) {
            runExample(app, n[0]);
            app.scene().evaluate();
            saveLatex(app, n[1]);
        }
        app.setMode(AppMode::Space3D);
        runExample(app, "Saddle");
    }, 25);
    step("21a4_preview_3d", [](App& app) {
        app.resetScene();
        app.exportPanel().setPreview(true);
        if (auto* p = app.panel("export")) p->setVisible(true);
    }, 25);
    step("21a5_preview_surface", [](App& app) { runExample(app, "Saddle"); }, 25);
    step("21a6_preview_complex", [](App& app) { runExample(app, "Rotate and scale"); }, 25);
    step("21b_reference", [](App& app) {
        app.exportPanel().setPreview(false);
        app.setMode(AppMode::Space3D);
        if (auto* p = app.panel("export")) p->setVisible(false);
        if (auto* p = app.panel("gallery")) p->setVisible(false);
        app.resetScene();
        if (auto* p = app.panel("reference")) p->setVisible(true);
        app.referencePanel().setTryText("rotz(pi/4)*[1, 0, 0]");
    }, 20);
    step("21c_reference_search", [](App& app) {
        app.referencePanel().setSearch("matrix");
        app.referencePanel().setTryText("M^-1 * 2");
    }, 20);
    step("22_tutorial_list", [](App& app) {
        app.referencePanel().setSearch("");
        app.referencePanel().setTryText("");
        if (auto* p = app.panel("reference")) p->setVisible(false);
        if (auto* p = app.panel("gallery")) p->setVisible(false);
        if (auto* p = app.panel("tutorial")) p->setVisible(true);
    });
    step("23_tutorial_objects", [](App& app) {
        app.tutorialPanel().startLesson(1);
        app.tutorialPanel().goToStep(1);
    }, 20);
    step("24_tutorial_parameters", [](App& app) { app.tutorialPanel().startLesson(6); }, 20);
    step("24b_tutorial_quaternions", [](App& app) {
        app.tutorialPanel().startLesson(static_cast<int>(lessons().size()) - 1);
    }, 20);
    step("25_settings", [](App& app) {
        if (auto* p = app.panel("tutorial")) p->setVisible(false);
        app.resetScene();
        app.showSettings(true);
    });
    step("26_color_picker", [](App& app) {
        app.showSettings(false);
        app.select("a");
    }, 4);
    step("26b_color_picker", [](App& app) { app.openColorPicker(); });
    step("27_light_space", [](App& app) {
        app.closePopovers();
        app.setTheme(false);
        runExample(app, "Animated rotation");
    }, 30);
    step("28_light_complex", [](App& app) {
        app.setMode(AppMode::Complex);
        app.resetScene();
        app.select("u", AppMode::Complex);
    });
    step("29_light_plot", [](App& app) { runExample(app, "Saddle"); });
    step("30_toast", [](App& app) { app.toast("Scene saved to scene.txt"); }, 6);
}

bool Tour::advance(App& app) {
    if (m_index >= m_steps.size()) return false;
    if (m_framesLeft < 0) {
        m_steps[m_index].action(app);
        m_framesLeft = m_steps[m_index].frames;
    }
    if (m_framesLeft == 0) m_captureNow = true;
    m_framesLeft--;
    return true;
}

void Tour::capture() {
    if (!m_captureNow) return;
    m_captureNow = false;

    // Draw calls are batched; flush them so the framebuffer holds the whole frame
    rlDrawRenderBatchActive();
    Image image = LoadImageFromScreen();
    std::string path = m_dir + "/" + m_steps[m_index].name + ".png";
    ExportImage(image, path.c_str());
    UnloadImage(image);
    printf("tour: %s (screen %dx%d, render %dx%d, image %dx%d)\n", path.c_str(), GetScreenWidth(), GetScreenHeight(), GetRenderWidth(), GetRenderHeight(), image.width, image.height);

    m_index++;
    m_framesLeft = -1;
}

} // namespace linalg
