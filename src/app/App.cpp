#include "app/App.hpp"
#include "app/Tour.hpp"
#include "edu/Examples.hpp"
#include "rendering/Labels.hpp"
#include "ui/SceneIO.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include "ui/core/Ui.hpp"

#include <raylib.h>
#include <raygui.h>
#include <rlgl.h>
#include <algorithm>
#include <ctime>

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

App::App(const AppOptions& options)
    : m_options(options), m_grid(10, 1.0f),
      m_analysisWindow("analysis", "Analysis", 356.0f, {1.0f, 0.0f}),
      m_animationWindow("animation", "Animation", 640.0f, {0.5f, 1.0f}),
      m_stepsWindow("steps", "Calculation steps", 430.0f, {0.0f, 0.0f}, false),
      m_breakdownWindow("breakdown", "Transformation breakdown", 380.0f, {0.0f, 0.4f}, false),
      m_galleryWindow("gallery", "Example gallery", 700.0f, {0.5f, 0.2f}, false) {
    // Preferences (not during the tour, which must be reproducible)
    SceneData prefs;
    if (options.tourDir.empty()) loadPreferences(prefs);

    int width = options.width, height = options.height;
    if (!options.sizeGiven) {
        getInt(prefs, "window.width", width);
        getInt(prefs, "window.height", height);
        width = std::max(width, 800);
        height = std::max(height, 600);
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(width, height, "axismundi - Linear Algebra 3D Visualizer");
    SetWindowMinSize(800, 600);
    SetTargetFPS(60);

    m_panels.add(&m_animationWindow);
    m_panels.add(&m_analysisWindow);
    m_panels.add(&m_stepsWindow);
    m_panels.add(&m_breakdownWindow);
    m_panels.add(&m_galleryWindow);

    applyPreferences(prefs);
    if (options.theme == "light") ui::setTheme(ui::ThemeKind::Light);
    else if (options.theme == "dark") ui::setTheme(ui::ThemeKind::Dark);
    ui::init();

    if (!options.tourDir.empty()) m_tour = std::make_unique<Tour>(options.tourDir);

    // Start in camera mode (cursor captured), except during the tour
    if (!m_tour) DisableCursor();
}

App::~App() {
    if (!m_tour) savePreferences();
    ui::shutdown();
    CloseWindow();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void App::setMode(AppMode mode) {
    m_mode = mode;
}

void App::resetCamera() {
    m_camera.reset(m_mode, m_mode == AppMode::Plotting && !m_plotRenderer.is3D());
}

bool App::breakdownActive() const {
    return m_mode == AppMode::LinearAlgebra && m_breakdownWindow.visible();
}

bool App::uiMode() const {
    return !IsCursorHidden();
}

void App::setUiMode(bool ui) {
    if (ui) EnableCursor();
    else DisableCursor();
}

void App::showHelp(bool show) {
    if (show != m_help.visible()) m_help.toggle();
}

void App::showSettings(bool show) {
    if (show) m_settingsPopover.open(m_topBar.settingsButton());
    else m_settingsPopover.close();
}

void App::openColorPicker(int slot) {
    Rectangle anchor = {SideBar::width() + ui::px(16), TopBar::height(), ui::px(24), ui::px(4)};
    m_colorPopover.open(slot, anchor, std::string("Color of ") + vectorSlotName(slot),
                        m_viewPanel.settings().colorFor(slot));
}

void App::closePopovers() {
    m_settingsPopover.close();
    m_colorPopover.close();
}

void App::setTheme(bool dark) {
    ui::setTheme(dark ? ui::ThemeKind::Dark : ui::ThemeKind::Light);
    m_plotPanel.markChanged();
}

void App::toggleTheme() {
    ui::toggleTheme();
    m_plotPanel.markChanged();  // plot colors are baked into its geometry
}

void App::toggleFullscreen() {
    ToggleBorderlessWindowed();
}

void App::toast(const std::string& message, bool error) {
    m_toast.text = message;
    m_toast.error = error;
    m_toast.timeLeft = 3.0f;
}

ui::FloatingPanel* App::panel(const std::string& id) {
    for (auto* p : m_panels.panels())
        if (p->id() == id) return p;
    return nullptr;
}

void App::requestScreenshot(bool sceneOnly) {
    m_screenshotRequest = sceneOnly ? 2 : 1;
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
// Preferences
// ---------------------------------------------------------------------------

void App::loadPreferences(SceneData& prefs) {
    Preferences::load(prefs);
}

void App::applyPreferences(const SceneData& prefs) {
    std::string theme;
    if (getString(prefs, "ui.theme", theme)) {
        ui::setTheme(theme == "light" ? ui::ThemeKind::Light : ui::ThemeKind::Dark);
    }
    float zoom = 1.0f;
    if (getFloat(prefs, "ui.zoom", zoom)) ui::setZoom(zoom);
    getBool(prefs, "ui.tooltips", m_uiSettings.tooltips);
    getBool(prefs, "ui.hud", m_uiSettings.showHud);
    m_viewPanel.loadColors(prefs);
    for (auto* p : m_panels.panels()) p->loadState(prefs);
}

void App::savePreferences() {
    SceneData prefs;
    putFloat(prefs, "ui.zoom", ui::zoom());
    prefs["ui.theme"] = ui::themeKind() == ui::ThemeKind::Light ? "light" : "dark";
    putBool(prefs, "ui.tooltips", m_uiSettings.tooltips);
    putBool(prefs, "ui.hud", m_uiSettings.showHud);
    if (!IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) && !IsWindowFullscreen()) {
        putInt(prefs, "window.width", GetScreenWidth());
        putInt(prefs, "window.height", GetScreenHeight());
    }
    m_viewPanel.saveColors(prefs);
    for (auto* p : m_panels.panels()) p->saveState(prefs);
    std::string error;
    Preferences::save(prefs, error);  // best effort: nothing useful to show while closing
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------

void App::run() {
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        if (m_tour && !m_tour->advance(*this)) break;

        ui::beginFrame();
        m_panels.beginFrame();
        handleInput(deltaTime);
        update(deltaTime);

        BeginDrawing();
            ClearBackground(ui::theme().background);
            drawScene();
            drawSceneLabels();
            if (m_screenshotRequest != 2) drawUi(deltaTime);
            if (m_tour) m_tour->capture();
            if (m_screenshotRequest) takeScreenshot();
        EndDrawing();

        // Fields are activated while drawing, so decide here whether Escape
        // should quit (checked by WindowShouldClose() on the next frame)
        SetExitKey(isEditing() || m_settingsPopover.isOpen() || m_colorPopover.isOpen() ? KEY_NULL : KEY_ESCAPE);
    }
}

void App::takeScreenshot() {
    m_screenshotRequest = 0;
    char name[64];
    std::time_t now = std::time(nullptr);
    std::strftime(name, sizeof(name), "axismundi_%Y%m%d_%H%M%S.png", std::localtime(&now));

    rlDrawRenderBatchActive();  // flush pending draw calls into the framebuffer
    Image image = LoadImageFromScreen();
    bool ok = ExportImage(image, name);
    UnloadImage(image);
    if (ok) toast(std::string("Screenshot saved as ") + name);
    else toast("Could not save the screenshot", true);
}

bool App::isEditing() const {
    return m_inputPanel.isEditing() || m_complexPanel.isEditing() || m_plotPanel.isEditing();
}

void App::handleInput(float deltaTime) {
    bool editing = isEditing();

    // Escape cancels an edit or closes a popup; otherwise it quits (raylib default)
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (editing) {
            m_inputPanel.cancelEditing();
            m_complexPanel.cancelEditing();
            m_plotPanel.cancelEditing();
            editing = false;
        } else if (m_colorPopover.isOpen()) {
            m_colorPopover.close();
        } else if (m_settingsPopover.isOpen()) {
            m_settingsPopover.close();
        }
    }

    // TAB switches between camera mode (cursor captured) and UI mode.
    // While a field is being edited, TAB moves to the next field instead.
    if (!editing && IsKeyPressed(KEY_TAB)) setUiMode(IsCursorHidden());

    bool ui = uiMode();
    // Widgets only respond in UI mode, and not behind the help overlay
    if (ui && !m_help.visible()) GuiUnlock();
    else GuiLock();

    m_inputPanel.update();
    m_complexPanel.update(deltaTime);
    m_plotPanel.update();

    if (!editing) handleShortcuts(ui);

    if (m_mode != m_lastMode) {
        // A field in the panel being hidden would otherwise stay "active"
        m_inputPanel.finishEditing();
        m_complexPanel.finishEditing();
        m_plotPanel.finishEditing();
        resetCamera();
        m_lastMode = m_mode;
    }

    m_camera.update(deltaTime);
}

void App::handleShortcuts(bool uiMode) {
    ViewSettings& view = m_viewPanel.settings();
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    if (ctrl && (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD))) ui::zoomIn();
    if (ctrl && (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT))) ui::zoomOut();
    if (ctrl && IsKeyPressed(KEY_ZERO)) ui::setZoom(1.0f);
    if (IsKeyPressed(KEY_F11)) toggleFullscreen();
    if (IsKeyPressed(KEY_F12)) requestScreenshot(shift);
    if (IsKeyPressed(KEY_T)) toggleTheme();
    if (IsKeyPressed(KEY_H)) m_help.toggle();
    if (IsKeyPressed(KEY_G)) view.showGrid = !view.showGrid;
    if (IsKeyPressed(KEY_L)) view.showLabels = !view.showLabels;
    if (IsKeyPressed(KEY_M)) m_analysisWindow.toggle();
    if (IsKeyPressed(KEY_K)) m_stepsWindow.toggle();
    if (IsKeyPressed(KEY_B)) m_breakdownWindow.toggle();
    if (IsKeyPressed(KEY_E)) {
        m_galleryWindow.toggle();
        if (m_galleryWindow.visible()) m_panels.bringToFront(&m_galleryWindow);
    }
    if (IsKeyPressed(KEY_C)) resetCamera();
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

    // The breakdown and the animator both replace the matrix shown in the
    // scene; opening the breakdown stops any animation
    m_breakdownPanel.setMatrix(m_matrix);
    bool breakdown = breakdownActive();
    if (breakdown && !m_breakdownWasVisible) {
        m_animator.stop();
        m_breakdownPanel.restart();
    }
    m_breakdownWasVisible = breakdown;
    if (breakdown) m_breakdownPanel.update(deltaTime);

    m_animator.update(deltaTime);
    if (breakdown) m_shown = m_breakdownPanel.current();
    else m_shown = m_animator.active() ? m_animator.current() : m_matrix;
    m_transformViz.setTransform4D(m_shown);

    m_vectorRenderer.clearVectors();
    m_vectorRenderer.addVector(VectorObject(Vector3D::zero(), m_vecA, view.colorFor(kVecA), "A"));
    m_vectorRenderer.addVector(VectorObject(Vector3D::zero(), m_vecB, view.colorFor(kVecB), "B"));
    m_vectorRenderer.addVector(VectorObject(Vector3D::zero(), m_vecA + m_vecB, view.colorFor(kVecSum), "A+B"));
    m_vectorRenderer.addVector(
        VectorObject(Vector3D::zero(), m_vecA.cross(m_vecB), view.colorFor(kVecCross), "A×B"));
    if (view.showTransformedVectors && !isIdentity(m_shown)) {
        Vector3D origin = m_shown.transform(Vector3D::zero());
        m_vectorRenderer.addVector(
            VectorObject(origin, m_shown.transform(m_vecA) - origin, view.colorFor(kVecMA), "MA"));
        m_vectorRenderer.addVector(
            VectorObject(origin, m_shown.transform(m_vecB) - origin, view.colorFor(kVecMB), "MB"));
    }
    m_inputPanel.setVectorColors(view.colorFor(kVecA), view.colorFor(kVecB));

    if (m_plotPanel.changed()) {
        m_plotRenderer.rebuild(m_plotPanel.spec());
        m_plotPanel.clearChanged();
        // Switching between 2D and 3D plots needs a different viewpoint
        if (m_plotRenderer.is3D() != m_plotWas3D) {
            m_plotWas3D = m_plotRenderer.is3D();
            if (m_mode == AppMode::Plotting) resetCamera();
        }
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
            if (breakdownActive()) {
                // Faded bases for the steps already done
                int done = std::min(m_breakdownPanel.completedSteps(),
                                    static_cast<int>(m_breakdownPanel.breakdown().steps.size()));
                for (int k = 1; k <= done; k++) {
                    if (k == static_cast<int>(m_breakdownPanel.breakdown().steps.size())) break;
                    m_transformViz.drawGhost(m_breakdownPanel.breakdown().after(k), 0.22f);
                }
            }
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

float App::viewportLeft() const {
    return SideBar::width();
}

float App::viewportRight() const {
    return static_cast<float>(GetScreenWidth());
}

void App::drawUi(float deltaTime) {
    // An open popover owns the mouse over its area
    Vector2 mouse = GetMousePosition();
    bool popoverOwnsMouse = m_settingsPopover.contains(mouse) || m_colorPopover.contains(mouse);
    bool wasLocked = GuiIsLocked();
    if (popoverOwnsMouse) GuiLock();

    if (m_uiSettings.showHud) drawHud();

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

    if (m_viewPanel.takeResetCameraRequest()) resetCamera();
    if (m_viewPanel.takeSaveRequest()) saveScene();
    if (m_viewPanel.takeLoadRequest()) loadScene();
    Rectangle swatch;
    int slot = m_viewPanel.takeColorRequest(swatch);
    if (slot >= 0) {
        m_settingsPopover.close();
        m_colorPopover.open(slot, swatch, std::string("Color of ") + vectorSlotName(slot),
                            m_viewPanel.settings().colorFor(slot));
    }

    drawFloatingPanels();

    TopBarState state;
    state.helpVisible = m_help.visible();
    state.settingsOpen = m_settingsPopover.isOpen();
    state.fullscreen = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
    state.stepsVisible = m_stepsWindow.visible();
    state.breakdownVisible = m_breakdownWindow.visible();
    state.galleryVisible = m_galleryWindow.visible();
    state.matricesMode = (m_mode == AppMode::LinearAlgebra);
    TopBarActions actions = m_topBar.draw(m_mode, state);
    if (actions.toggleTheme) toggleTheme();
    if (actions.toggleHelp) m_help.toggle();
    if (actions.toggleFullscreen) toggleFullscreen();
    if (actions.screenshot) requestScreenshot(false);
    if (actions.toggleGallery) {
        m_galleryWindow.toggle();
        if (m_galleryWindow.visible()) m_panels.bringToFront(&m_galleryWindow);
    }
    if (actions.toggleBreakdown) {
        m_breakdownWindow.toggle();
        if (m_breakdownWindow.visible()) m_panels.bringToFront(&m_breakdownWindow);
    }
    if (actions.toggleSteps) {
        m_stepsWindow.toggle();
        if (m_stepsWindow.visible()) m_panels.bringToFront(&m_stepsWindow);
    }
    if (actions.toggleSettings) {
        m_colorPopover.close();
        showSettings(!m_settingsPopover.isOpen());
    }

    if (popoverOwnsMouse && !wasLocked) GuiUnlock();

    SettingsActions settingsActions = m_settingsPopover.draw(m_uiSettings);
    if (settingsActions.themeChanged) m_plotPanel.markChanged();
    if (settingsActions.resetLayout) {
        m_panels.resetLayout();
        toast("Panel layout reset");
    }

    ColorPopover::Result color = m_colorPopover.draw();
    if (color.changed || color.reset) {
        ViewSettings& view = m_viewPanel.settings();
        int s = m_colorPopover.slot();
        view.customColor[s] = color.changed && !color.reset;
        if (view.customColor[s]) view.vectorColor[s] = color.color;
    }

    drawToast(deltaTime);
    m_help.draw();
    ui::drawTooltip(m_uiSettings.tooltips && !m_help.visible());
}

void App::drawFloatingPanels() {
    bool matrices = (m_mode == AppMode::LinearAlgebra);
    for (ui::FloatingPanel* p : m_panels.panels()) {
        Rectangle content;
        if (p == &m_analysisWindow) {
            if (!matrices) continue;
            if (p->begin(m_analysisHeight, content)) {
                m_analysisHeight = m_infoPanel.drawContent(content);
                p->end();
            }
        } else if (p == &m_animationWindow) {
            if (!matrices) continue;
            if (p->begin(m_animationHeight, content)) {
                m_animationHeight = m_animationPanel.drawContent(content, m_animator, m_matrix);
                p->end();
            }
        } else if (p == &m_breakdownWindow) {
            if (!matrices) continue;
            if (p->begin(m_breakdownHeight, content)) {
                m_breakdownHeight = m_breakdownPanel.drawContent(content);
                p->end();
            }
        } else if (p == &m_galleryWindow) {
            if (p->begin(m_galleryHeight, content)) {
                int chosen = -1;
                m_galleryHeight = m_galleryPanel.drawContent(content, chosen);
                p->end();
                if (chosen >= 0) {
                    const Example& e = examples()[chosen];
                    e.apply(*this);
                    m_galleryWindow.setVisible(false);
                    toast(std::string("Example: ") + e.title);
                }
            }
        } else if (p == &m_stepsWindow) {
            if (p->begin(m_stepsHeight, content)) {
                StepsInput input{m_mode, m_matrix, m_inputPanel.isUsing4x4(), m_vecA, m_vecB, m_complexPanel.scene()};
                m_stepsHeight = m_stepsPanel.drawContent(content, input);
                p->end();
            }
        }
    }
}

void App::drawHud() {
    const ui::Theme& t = ui::theme();
    bool ui = uiMode();
    float x = viewportLeft() + ui::px(16.0f);
    float bottom = static_cast<float>(GetScreenHeight()) - ui::px(14.0f);

    const char* modeText = ui ? "UI mode  ·  Tab: camera mode  ·  H: help"
                              : "Camera mode  ·  WASD move, mouse look  ·  Tab: UI mode";
    Vector3 p = m_camera.position();
    const char* camText = TextFormat("Camera (%.2f, %.2f, %.2f)  ·  %d FPS", p.x, p.y, p.z, GetFPS());

    float lineH = ui::px(20.0f);
    float w = std::max(ui::textWidth(modeText, ui::FontRole::Small), ui::textWidth(camText, ui::FontRole::Small));
    Rectangle box = {x, bottom - 2 * lineH - ui::px(10.0f), w + ui::px(20.0f), 2 * lineH + ui::px(10.0f)};
    ui::fillRounded(box, ui::px(8.0f), t.labelBg);
    float tx = box.x + ui::px(10.0f);
    // A small dot shows the input mode at a glance
    DrawCircleV({tx + ui::px(3), box.y + ui::px(5) + lineH / 2}, ui::px(3.5f), ui ? t.accent : t.success);
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
    float left = viewportLeft(), right = viewportRight();
    Rectangle box = {left + (right - left - w) / 2, TopBar::height() + ui::px(16.0f), w, h};

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
