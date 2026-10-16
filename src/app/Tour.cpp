#include "app/Tour.hpp"
#include "app/App.hpp"
#include <raylib.h>
#include <rlgl.h>
#include <cstdio>

namespace linalg {

Tour::Tour(std::string outputDir) : m_dir(std::move(outputDir)) {
    buildSteps();
}

void Tour::buildSteps() {
    auto step = [this](const char* name, std::function<void(App&)> action, int frames = 12) {
        m_steps.push_back({name, std::move(action), frames});
    };

    step("01_matrices_default", [](App& app) {
        app.setUiMode(true);
        app.setMode(AppMode::LinearAlgebra);
    });
    step("02_matrices_rotate", [](App& app) { app.applyPreset(MatrixPreset::Rotate45Z); });
    step("03_matrices_4x4", [](App& app) {
        app.setMatrix4(Matrix4D::translation(1, 0.5f, 0) * Matrix4D::rotationY(0.6f) *
                       Matrix4D::scale(1.5f, 1, 1));
    });
    step("04_matrices_animating", [](App& app) {
        app.setMatrix3(Matrix3D({{{{1, 1, 0}}, {{0, 1, 0}}, {{0, 0, 2}}}}));
        app.startAnimation(AnimationPreset::Morph);
    }, 60);
    step("05_complex", [](App& app) { app.setMode(AppMode::Complex); });
    step("06_plot_surface", [](App& app) {
        app.setMode(AppMode::Plotting);
        PlotSpec s;
        s.type = PlotType::Surface;
        s.f = "sin(sqrt(x^2 + y^2))";
        s.xMin = s.yMin = -6;
        s.xMax = s.yMax = 6;
        s.resolution = 50;
        app.setPlotSpec(s);
    });
    step("07_plot_implicit", [](App& app) {
        PlotSpec s;
        s.type = PlotType::Implicit;
        s.f = "(x^2 + y^2)^2 - 8(x^2 - y^2)";
        app.setPlotSpec(s);
        app.resetCamera();
    });
    step("08_plot_parametric", [](App& app) {
        PlotSpec s;
        s.type = PlotType::Parametric;
        s.px = "sin(t) + 2sin(2t)";
        s.py = "cos(t) - 2cos(2t)";
        s.pz = "-sin(3t)";
        s.tMin = 0;
        s.tMax = 6.2832f;
        app.setPlotSpec(s);
    });
    step("09_help", [](App& app) {
        app.setMode(AppMode::LinearAlgebra);
        app.showHelp(true);
    });
    step("10_toast", [](App& app) {
        app.showHelp(false);
        app.toast("Scene saved to scene.txt");
    }, 6);
    step("11_settings", [](App& app) { app.showSettings(true); });
    step("11b_color_picker", [](App& app) {
        app.showSettings(false);
        app.openColorPicker(kVecA);
    });
    step("12_panels_moved", [](App& app) {
        app.closePopovers();
        SceneData layout;
        putFloat(layout, "panel.analysis.x", 0.0f);
        putFloat(layout, "panel.analysis.y", 0.15f);
        putBool(layout, "panel.animation.collapsed", true);
        putFloat(layout, "panel.animation.x", 1.0f);
        if (auto* p = app.panel("analysis")) p->loadState(layout);
        if (auto* p = app.panel("animation")) p->loadState(layout);
    });
    step("13_light_complex", [](App& app) {
        app.setTheme(false);
        app.setMode(AppMode::Complex);
    });
    step("14_light_plot", [](App& app) {
        app.setMode(AppMode::Plotting);
        PlotSpec s;
        s.type = PlotType::Function2D;
        s.f = "x^3/10 - x";
        app.setPlotSpec(s);
    });
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
