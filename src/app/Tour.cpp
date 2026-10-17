#include "app/Tour.hpp"
#include "app/App.hpp"
#include "edu/Examples.hpp"
#include <raylib.h>
#include <rlgl.h>
#include <cstdio>
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
    step("01_space_default", [](App& app) {
        app.setUiMode(true);
        park();
        app.setMode(AppMode::Space3D);
    });
    step("02_vector_inspector", [](App& app) { app.select("a"); });
    step("03_add_menu_templates", [](App& app) {
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
    step("07_parameter_rotation", [](App& app) { runExample(app, "Animated rotation"); }, 50);
    step("08_matrix_formula_4x4", [](App& app) {
        runExample(app, "Move and turn (4×4)");
        app.select("M");
    });
    step("09_matrix_cells", [](App& app) {
        app.resetScene();
        app.select("M");
        app.applyMatrixPreset(Matrix3D({{{{1, 1, 0}}, {{0, 1, 0}}, {{0, 0, 2}}}}));
        app.startAnimation(AnimationPreset::Morph);
    }, 50);
    step("10_projection_example", [](App& app) {
        app.stopAnimation();
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
    step("15_plot_implicit", [](App& app) { runExample(app, "Lemniscate"); });
    step("16_plot_curve", [](App& app) {
        runExample(app, "Trefoil knot");
        app.select("f");
    });

    // --- Tools ----------------------------------------------------------------------
    step("17_help", [](App& app) {
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
    step("22_tutorial_list", [](App& app) {
        if (auto* p = app.panel("gallery")) p->setVisible(false);
        if (auto* p = app.panel("tutorial")) p->setVisible(true);
    });
    step("23_tutorial_objects", [](App& app) {
        app.tutorialPanel().startLesson(1);
        app.tutorialPanel().goToStep(1);
    }, 20);
    step("24_tutorial_parameters", [](App& app) { app.tutorialPanel().startLesson(6); }, 20);
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
