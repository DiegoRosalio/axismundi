#include "app/App.hpp"
#include "app/Tour.hpp"
#include "rendering/Labels.hpp"
#include "ui/SceneIO.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include "ui/core/Ui.hpp"

#include <raylib.h>
#include <raygui.h>
#include <algorithm>

namespace linalg {

namespace {

const char* kSceneFile = "scene.txt";

bool isIdentity(const Matrix4D& m) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (m.m[i][j] != (i == j ? 1.0f : 0.0f)) return false;
    return true;
}

void drawAxisLabels(const Camera3D& camera, float length) {
    const ui::Theme& t = ui::theme();
    drawWorldLabel(camera, {length, 0, 0}, "x", t.axisX);
    drawWorldLabel(camera, {0, length, 0}, "y", t.axisY);
    drawWorldLabel(camera, {0, 0, length}, "z", t.axisZ);
}

} // namespace

App::App(const AppOptions& options) : m_options(options), m_grid(10, 1.0f) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(options.width, options.height, "axismundi - Linear Algebra 3D Visualizer");
    SetWindowMinSize(800, 600);
    SetTargetFPS(60);
    ui::init();
    if (options.theme == "light") ui::setTheme(ui::ThemeKind::Light);
    else if (options.theme == "dark") ui::setTheme(ui::ThemeKind::Dark);

    if (!options.tourDir.empty()) m_tour = std::make_unique<Tour>(options.tourDir);

    // Start in camera mode (cursor captured), except during the tour
    if (!m_tour) DisableCursor();
}

App::~App() {
    ui::shutdown();
    CloseWindow();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void App::setMode(AppMode mode) {
    m_mode = mode;
}

void App::setUiMode(bool ui) {
    if (ui) EnableCursor();
    else DisableCursor();
}

void App::showHelp(bool show) {
    if (show != m_help.visible()) m_help.toggle();
}

void App::toggleTheme() {
    ui::toggleTheme();
    m_plotPanel.markChanged();  // plot colors are baked into its geometry
}

void App::toast(const std::string& message, bool error) {
    m_toast.text = message;
    m_toast.error = error;
    m_toast.timeLeft = 3.0f;
}

void App::saveScene() {
    SceneData data;
    putInt(data, "mode", static_cast<int>(m_mode));
    Vector3 p = m_camera.position(), t = m_camera.target();
    putFloat(data, "camera.px", p.x);
    putFloat(data, "camera.py", p.y);
    putFloat(data, "camera.pz", p.z);
    putFloat(data, "camera.tx", t.x);
    putFloat(data, "camera.ty", t.y);
    putFloat(data, "camera.tz", t.z);
    m_inputPanel.saveState(data);
    m_complexPanel.saveState(data);
    m_plotPanel.saveState(data);
    AnimationPanel::saveState(m_animator, data);
    m_viewPanel.saveState(data);

    std::string error;
    if (saveSceneFile(kSceneFile, data, error)) toast(std::string("Scene saved to ") + kSceneFile);
    else toast("Save failed: " + error, true);
}

void App::loadScene() {
    SceneData data;
    std::string error;
    if (!loadSceneFile(kSceneFile, data, error)) {
        toast("Load failed: " + error, true);
        return;
    }
    int m = static_cast<int>(m_mode);
    if (getInt(data, "mode", m) && m >= 0 && m < kAppModeCount) {
        m_mode = static_cast<AppMode>(m);
        m_lastMode = m_mode;  // keep the saved camera instead of the mode default
    }
    Vector3 p = m_camera.position(), t = m_camera.target();
    getFloat(data, "camera.px", p.x);
    getFloat(data, "camera.py", p.y);
    getFloat(data, "camera.pz", p.z);
    getFloat(data, "camera.tx", t.x);
    getFloat(data, "camera.ty", t.y);
    getFloat(data, "camera.tz", t.z);
    m_camera.setView(p, t);
    m_inputPanel.loadState(data);
    m_complexPanel.loadState(data);
    m_plotPanel.loadState(data);
    AnimationPanel::loadState(m_animator, data);
    m_viewPanel.loadState(data);
    toast(std::string("Scene loaded from ") + kSceneFile);
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------

void App::run() {
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        if (m_tour && !m_tour->advance(*this)) break;

        ui::beginFrame();
        handleInput(deltaTime);
        update(deltaTime);

        BeginDrawing();
            ClearBackground(ui::theme().background);
            drawScene();
            drawSceneLabels();
            drawUi(deltaTime);
            if (m_tour) m_tour->capture();
        EndDrawing();

        // Fields are activated while drawing, so decide here whether Escape
        // should quit (checked by WindowShouldClose() on the next frame)
        SetExitKey(isEditing() ? KEY_NULL : KEY_ESCAPE);
    }
}

bool App::isEditing() const {
    return m_inputPanel.isEditing() || m_complexPanel.isEditing() || m_plotPanel.isEditing();
}

void App::handleInput(float deltaTime) {
    bool editing = isEditing();

    // Escape cancels an edit in progress; otherwise it quits (raylib default)
    if (editing && IsKeyPressed(KEY_ESCAPE)) {
        m_inputPanel.cancelEditing();
        m_complexPanel.cancelEditing();
        m_plotPanel.cancelEditing();
        editing = false;
    }

    // TAB switches between camera mode (cursor captured) and UI mode.
    // While a field is being edited, TAB moves to the next field instead.
    if (!editing && IsKeyPressed(KEY_TAB)) setUiMode(IsCursorHidden());

    bool uiMode = !IsCursorHidden();
    // Widgets only respond in UI mode, and not behind the help overlay
    if (uiMode && !m_help.visible()) GuiUnlock();
    else GuiLock();

    m_inputPanel.update();
    m_complexPanel.update(deltaTime);
    m_plotPanel.update();

    if (!editing) handleShortcuts(uiMode);

    if (m_mode != m_lastMode) {
        // A field in the panel being hidden would otherwise stay "active"
        m_inputPanel.finishEditing();
        m_complexPanel.finishEditing();
        m_plotPanel.finishEditing();
        m_camera.reset(m_mode);
        m_lastMode = m_mode;
    }

    m_camera.update(deltaTime);
}

void App::handleShortcuts(bool uiMode) {
    ViewSettings& view = m_viewPanel.settings();
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

    if (ctrl && (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD))) ui::zoomIn();
    if (ctrl && (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT))) ui::zoomOut();
    if (ctrl && IsKeyPressed(KEY_ZERO)) ui::setZoom(1.0f);
    if (IsKeyPressed(KEY_T)) toggleTheme();
    if (IsKeyPressed(KEY_H)) m_help.toggle();
    if (IsKeyPressed(KEY_G)) view.showGrid = !view.showGrid;
    if (IsKeyPressed(KEY_L)) view.showLabels = !view.showLabels;
    if (IsKeyPressed(KEY_M)) m_infoPanel.toggleVisible();
    if (IsKeyPressed(KEY_C)) m_camera.reset(m_mode);
    if (IsKeyPressed(KEY_ONE)) m_mode = AppMode::LinearAlgebra;
    if (IsKeyPressed(KEY_TWO)) m_mode = AppMode::Complex;
    if (IsKeyPressed(KEY_THREE)) m_mode = AppMode::Plotting;
    if (IsKeyPressed(KEY_F5)) saveScene();
    if (IsKeyPressed(KEY_F9)) loadScene();

    if (m_mode != AppMode::LinearAlgebra) return;

    if (ctrl) {
        // Ctrl is "move down" in camera mode, so undo/redo is UI-mode only
        if (uiMode && IsKeyPressed(KEY_Z)) m_inputPanel.undo();
        if (uiMode && IsKeyPressed(KEY_Y)) m_inputPanel.redo();
        return;
    }
    if (IsKeyPressed(KEY_I)) m_inputPanel.applyPreset(MatrixPreset::Identity);
    if (IsKeyPressed(KEY_R)) m_inputPanel.applyPreset(MatrixPreset::Rotate45Z);
    if (IsKeyPressed(KEY_X)) m_inputPanel.applyPreset(MatrixPreset::ShearX);
    if (IsKeyPressed(KEY_F)) m_inputPanel.applyPreset(MatrixPreset::ReflectX);
    if (IsKeyPressed(KEY_P)) {
        if (m_animator.active()) m_animator.togglePlay();
        else m_animator.start(AnimationPreset::Morph);
    }
    // S moves the camera backwards in camera mode
    if (uiMode && IsKeyPressed(KEY_S)) m_inputPanel.applyPreset(MatrixPreset::Scale2);
}

void App::update(float deltaTime) {
    ViewSettings& view = m_viewPanel.settings();

    if (m_inputPanel.matrixChanged() || m_inputPanel.vectorsChanged()) {
        m_matrix = m_inputPanel.getActiveMatrix4D();
        m_vecA = m_inputPanel.getVectorA();
        m_vecB = m_inputPanel.getVectorB();
        m_animator.setMorphTarget(m_matrix);
        m_infoPanel.update(m_matrix, m_inputPanel.isUsing4x4(), m_vecA, m_vecB);
    }
    m_inputPanel.resetChangedFlags();

    // While animating, the scene shows the animated matrix instead of the input
    m_animator.update(deltaTime);
    m_shown = m_animator.active() ? m_animator.current() : m_matrix;
    m_transformViz.setTransform4D(m_shown);

    const ui::Theme& t = ui::theme();
    m_vectorRenderer.clearVectors();
    m_vectorRenderer.addVector(VectorObject(Vector3D::zero(), m_vecA, t.vecA, "A"));
    m_vectorRenderer.addVector(VectorObject(Vector3D::zero(), m_vecB, t.vecB, "B"));
    m_vectorRenderer.addVector(VectorObject(Vector3D::zero(), m_vecA + m_vecB, t.vecSum, "A+B"));
    m_vectorRenderer.addVector(VectorObject(Vector3D::zero(), m_vecA.cross(m_vecB), t.vecCross, "A×B"));
    if (view.showTransformedVectors && !isIdentity(m_shown)) {
        Vector3D origin = m_shown.transform(Vector3D::zero());
        m_vectorRenderer.addVector(VectorObject(origin, m_shown.transform(m_vecA) - origin, t.vecMA, "MA"));
        m_vectorRenderer.addVector(VectorObject(origin, m_shown.transform(m_vecB) - origin, t.vecMB, "MB"));
    }

    if (m_plotPanel.changed()) {
        m_plotRenderer.rebuild(m_plotPanel.spec());
        m_plotPanel.clearChanged();
    }

    m_grid.setShowGrid(view.showGrid);
    m_grid.setShowAxes(view.showAxes);
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void App::drawScene() {
    const ViewSettings& view = m_viewPanel.settings();
    BeginMode3D(m_camera.camera());
    switch (m_mode) {
        case AppMode::LinearAlgebra:
            m_grid.draw();
            m_grid.drawAxes(view.axisLength);
            m_transformViz.draw();
            m_vectorRenderer.draw();
            if (view.showEigenvectors) {
                TransformVisualizer::drawEigenvectors(m_infoPanel.eigen(), view.axisLength);
            }
            break;
        case AppMode::Complex:
            m_grid.drawPlaneXY();
            m_argand.draw(m_complexPanel.scene(), view.axisLength, view.showAxes);
            break;
        case AppMode::Plotting:
            if (m_plotRenderer.is3D()) m_grid.draw();
            else m_grid.drawPlaneXY();
            if (view.showAxes) m_plotRenderer.drawAxes(view.axisLength);
            m_plotRenderer.draw();
            break;
    }
    EndMode3D();
}

void App::drawSceneLabels() {
    const ViewSettings& view = m_viewPanel.settings();
    if (!view.showLabels) return;
    const Camera3D& cam = m_camera.camera();

    switch (m_mode) {
        case AppMode::LinearAlgebra:
            if (view.showAxes) drawAxisLabels(cam, view.axisLength);
            m_vectorRenderer.drawLabels(cam);
            if (view.showEigenvectors) TransformVisualizer::drawEigenLabels(cam, m_infoPanel.eigen());
            break;
        case AppMode::Complex:
            m_argand.drawLabels(cam, m_complexPanel.scene(), view.axisLength, view.showAxes);
            break;
        case AppMode::Plotting:
            m_plotRenderer.drawLabels(cam, view.axisLength, view.showAxes);
            break;
    }
}

void App::drawUi(float deltaTime) {
    drawHud();

    // Side bar
    Rectangle area = m_sideBar.begin();
    float y = area.y;
    switch (m_mode) {
        case AppMode::LinearAlgebra:
            y = m_inputPanel.draw(area.x, y, area.width);
            break;
        case AppMode::Complex:
            y = m_complexPanel.draw(area.x, y, area.width);
            break;
        case AppMode::Plotting:
            y = m_plotPanel.draw(area.x, y, area.width, m_plotRenderer);
            break;
    }
    y = m_viewPanel.draw(area.x, y, area.width, m_mode);
    m_sideBar.end(y);

    if (m_viewPanel.takeResetCameraRequest()) m_camera.reset(m_mode);
    if (m_viewPanel.takeSaveRequest()) saveScene();
    if (m_viewPanel.takeLoadRequest()) loadScene();

    if (m_mode == AppMode::LinearAlgebra) {
        m_infoPanel.draw();
        float left = SideBar::width();
        float right = GetScreenWidth() - m_infoPanel.width();
        m_animationPanel.draw(m_animator, m_matrix, left, right);
    }

    // The top bar is drawn last among the panels so dropdown-like content
    // from the side bar never covers it
    TopBarActions actions = m_topBar.draw(m_mode, m_help.visible());
    if (actions.toggleTheme) toggleTheme();
    if (actions.toggleHelp) m_help.toggle();

    drawToast(deltaTime);
    m_help.draw();
}

void App::drawHud() {
    const ui::Theme& t = ui::theme();
    bool uiMode = !IsCursorHidden();
    float x = SideBar::width() + ui::px(16.0f);
    float bottom = static_cast<float>(GetScreenHeight()) - ui::px(14.0f);
    if (m_mode == AppMode::LinearAlgebra) {
        // Keep clear of the animation bar
        float left = SideBar::width();
        float right = GetScreenWidth() - m_infoPanel.width();
        Rectangle bar = AnimationPanel::bounds(left, right);
        if (bar.x < x + ui::px(300.0f)) bottom = bar.y - ui::px(10.0f);
    }

    const char* modeText = uiMode ? "UI mode  ·  Tab: camera mode  ·  H: help"
                                  : "Camera mode  ·  WASD move, mouse look  ·  Tab: UI mode";
    Vector3 p = m_camera.position();
    const char* camText = TextFormat("Camera (%.2f, %.2f, %.2f)  ·  %d FPS", p.x, p.y, p.z, GetFPS());

    float lineH = ui::px(20.0f);
    float w = std::max(ui::textWidth(modeText, ui::FontRole::Small), ui::textWidth(camText, ui::FontRole::Small));
    Rectangle box = {x, bottom - 2 * lineH - ui::px(10.0f), w + ui::px(20.0f), 2 * lineH + ui::px(10.0f)};
    ui::fillRounded(box, ui::px(8.0f), t.labelBg);
    float tx = box.x + ui::px(10.0f);
    // A small dot shows the input mode at a glance
    DrawCircleV({tx + ui::px(3), box.y + ui::px(5) + lineH / 2}, ui::px(3.5f), uiMode ? t.accent : t.success);
    ui::textIn({tx + ui::px(12), box.y + ui::px(5), w, lineH}, modeText, ui::FontRole::Small, t.text);
    ui::textIn({tx, box.y + ui::px(5) + lineH, w, lineH}, camText, ui::FontRole::Small, t.textMuted);
}

void App::drawToast(float deltaTime) {
    if (m_toast.timeLeft <= 0.0f) return;
    m_toast.timeLeft -= deltaTime;
    const ui::Theme& t = ui::theme();

    float alpha = m_toast.timeLeft < 0.4f ? m_toast.timeLeft / 0.4f : 1.0f;
    float w = ui::textWidth(m_toast.text.c_str(), ui::FontRole::Body) + ui::px(56.0f);
    float h = ui::px(42.0f);
    float left = SideBar::width();
    float right = static_cast<float>(GetScreenWidth()) - (m_mode == AppMode::LinearAlgebra ? m_infoPanel.width() : 0);
    float bottom = static_cast<float>(GetScreenHeight()) - ui::px(24.0f);
    if (m_mode == AppMode::LinearAlgebra) bottom = AnimationPanel::bounds(left, right).y - ui::px(14.0f);
    Rectangle box = {left + (right - left - w) / 2, bottom - h, w, h};

    Color accent = m_toast.error ? t.danger : t.success;
    ui::shadow(box, ui::px(10), ui::px(10));
    ui::fillRounded(box, ui::px(10), Fade(t.panel, alpha));
    ui::strokeRounded(box, ui::px(10), ui::pxi(1), Fade(t.border, alpha));
    float iconSize = ui::px(18.0f);
    Rectangle icon = {box.x + ui::px(14), box.y + (h - iconSize) / 2, iconSize, iconSize};
    ui::drawIcon(m_toast.error ? ui::Icon::Close : ui::Icon::Check, icon, Fade(accent, alpha));
    ui::textIn({icon.x + iconSize + ui::px(10), box.y, w, h}, m_toast.text.c_str(), ui::FontRole::Body,
               Fade(t.text, alpha));
}

} // namespace linalg
