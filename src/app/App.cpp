#include "app/App.hpp"
#include "app/Tour.hpp"
#include "edu/Examples.hpp"
#include "math/Decompositions.hpp"
#include "rendering/WorldMapping.hpp"
#include "ui/SceneIO.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include "ui/core/Ui.hpp"

#include <raylib.h>
#include <raygui.h>
#include <raymath.h>
#include <rlgl.h>
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <cmath>
#include <ctime>

namespace linalg {

using scene::SceneObject;
using scene::SpaceKind;

namespace {

const char* kSceneFile = "scene.txt";

// Distance from point p to segment ab (screen space)
float distanceToSegment(Vector2 p, Vector2 a, Vector2 b) {
    Vector2 ab = Vector2Subtract(b, a);
    float len2 = Vector2LengthSqr(ab);
    float t = len2 > 1e-6f ? std::clamp(Vector2DotProduct(Vector2Subtract(p, a), ab) / len2, 0.0f, 1.0f) : 0.0f;
    return Vector2Distance(p, Vector2Add(a, Vector2Scale(ab, t)));
}

bool inFront(const Camera3D& cam, Vector3 p) {
    return Vector3DotProduct(Vector3Subtract(cam.target, cam.position), Vector3Subtract(p, cam.position)) > 0.0f;
}

} // namespace

SpaceKind App::kindOf(AppMode mode) {
    return mode == AppMode::Space3D ? SpaceKind::Euclidean3D : SpaceKind::Complex;
}

App::App(const AppOptions& options)
    : m_options(options), m_grid(10, 1.0f),
      m_analysisWindow("analysis", "Analysis", 356.0f, {1.0f, 0.0f}),
      m_animationWindow("animation", "Time", 640.0f, {0.5f, 1.0f}),
      m_stepsWindow("steps", "Calculation steps", 430.0f, {0.0f, 0.0f}, false),
      m_breakdownWindow("breakdown", "Transformation breakdown", 380.0f, {0.0f, 0.4f}, false),
      m_galleryWindow("gallery", "Example gallery", 700.0f, {0.5f, 0.2f}, false),
      m_tutorialWindow("tutorial", "Tutorial", 400.0f, {0.0f, 0.0f}, false),
      m_referenceWindow("reference", "Formula reference", 620.0f, {0.45f, 0.1f}, false) {
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
    SetExitKey(KEY_NULL);  // Esc never quits; close the window to exit

    m_panels.add(&m_animationWindow);
    m_panels.add(&m_analysisWindow);
    m_panels.add(&m_stepsWindow);
    m_panels.add(&m_breakdownWindow);
    m_panels.add(&m_galleryWindow);
    m_panels.add(&m_tutorialWindow);
    m_panels.add(&m_referenceWindow);

    applyPreferences(prefs);
    if (options.theme == "light") ui::setTheme(ui::ThemeKind::Light);
    else if (options.theme == "dark") ui::setTheme(ui::ThemeKind::Dark);
    ui::init();

    if (!options.tourDir.empty()) m_tour = std::make_unique<Tour>(options.tourDir);

    // Start with an empty scene and the cursor free, ready to add objects. The
    // tour keeps the demo scene so its screenshots stay reproducible.
    if (!m_tour) {
        m_scene.clearAll();
        m_scene.clearHistory();
    }
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

SceneObject* App::object(const std::string& name, AppMode mode) {
    return space(mode).find(name);
}

SceneObject* App::addObject(AppMode mode, const std::string& typeId, const std::string& name,
                            const std::map<std::string, std::string>& fields,
                            const std::map<std::string, std::string>& props) {
    AddChoice c;
    c.typeId = typeId;
    c.name = name;
    c.fields = fields;
    c.props = props;
    return applyAddChoice(m_scene, kindOf(mode), c);
}

void App::select(const std::string& name, AppMode mode, bool add) {
    SceneObject* o = object(name, mode);
    if (!o) return;
    if (add) m_scene.selection(kindOf(mode)).toggle(o->id());
    else m_scene.selection(kindOf(mode)).set(o->id());
}

void App::clearSpace(AppMode mode) {
    m_scenePanel.cancelEditing();
    m_scene.space(kindOf(mode)).clear();
    m_scene.selection(kindOf(mode)).clear();
    m_scene.commit();
}

void App::resetScene() {
    m_scenePanel.cancelEditing();
    m_scene.loadDefaultScene();
}

void App::setMatrix3(const Matrix3D& m) {
    auto* obj = dynamic_cast<scene::MatrixObject*>(object("M"));
    if (!obj) obj = static_cast<scene::MatrixObject*>(addObject(AppMode::Space3D, "matrix", "M"));
    obj->setByFormula(false);
    obj->setSize(3);
    obj->visible = true;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            char buf[32];
            formatNumber(buf, sizeof(buf), m.m[i][j]);
            obj->setField(scene::MatrixObject::cellKey(i, j), buf);
        }
    m_scene.commit();
}

void App::setMatrix4(const Matrix4D& m) {
    auto* obj = dynamic_cast<scene::MatrixObject*>(object("M"));
    if (!obj) obj = static_cast<scene::MatrixObject*>(addObject(AppMode::Space3D, "matrix", "M"));
    obj->setByFormula(false);
    obj->setSize(4);
    obj->visible = true;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            char buf[32];
            formatNumber(buf, sizeof(buf), m.m[i][j]);
            obj->setField(scene::MatrixObject::cellKey(i, j), buf);
        }
    m_scene.commit();
}

void App::setVectors(const Vector3D& a, const Vector3D& b) {
    const char* names[2] = {"a", "b"};
    const Vector3D values[2] = {a, b};
    for (int k = 0; k < 2; k++) {
        auto* v = dynamic_cast<scene::VectorObject*>(object(names[k]));
        if (!v) v = static_cast<scene::VectorObject*>(addObject(AppMode::Space3D, "vector", names[k]));
        v->setByFormula(false);
        char buf[32];
        formatNumber(buf, sizeof(buf), values[k].x);
        v->setField("x", buf);
        formatNumber(buf, sizeof(buf), values[k].y);
        v->setField("y", buf);
        formatNumber(buf, sizeof(buf), values[k].z);
        v->setField("z", buf);
    }
    m_scene.commit();
}

void App::applyMatrixPreset(const Matrix3D& m) {
    auto* obj = dynamic_cast<scene::MatrixObject*>(const_cast<SceneObject*>(targetMatrix()));
    if (!obj) {
        setMatrix3(m);
        return;
    }
    m_scene.evaluate();  // presets compose with the matrix's current value
    obj->applyPreset(m);
    m_scene.evaluate();
    m_scene.commit();
}

void App::resetCamera() {
    m_camera.reset(m_mode);
}

bool App::breakdownActive() const {
    return m_mode == AppMode::Space3D && m_breakdownWindow.visible() && targetMatrix() != nullptr;
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

void App::openAddMenu() {
    Rectangle anchor;
    if (!ui::findAnchor("objects.add", anchor)) anchor = {SideBar::width() - ui::px(90), TopBar::height(), ui::px(74), ui::px(32)};
    m_addMenu.open(anchor);
}

void App::openColorPicker() {
    SceneObject* o = m_scene.space(spaceKind()).find(m_scene.selection(spaceKind()).primary());
    if (!o) return;
    Rectangle anchor;
    if (!ui::findAnchor("inspector.name", anchor)) anchor = {SideBar::width() - ui::px(40), TopBar::height(), ui::px(24), ui::px(24)};
    m_colorPopover.open(static_cast<int>(o->id()), anchor, "Color of " + o->name(), view::SceneView::colorOf(*o));
}

void App::closePopovers() {
    m_settingsPopover.close();
    m_colorPopover.close();
    m_addMenu.close();
}

void App::setTheme(bool dark) {
    ui::setTheme(dark ? ui::ThemeKind::Dark : ui::ThemeKind::Light);
}

void App::toggleTheme() {
    ui::toggleTheme();
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

// --- Read-only state -----------------------------------------------------------

const SceneObject* App::targetMatrix() const {
    const scene::Space& sp = m_scene.space(SpaceKind::Euclidean3D);
    const auto& sel = const_cast<scene::Scene&>(m_scene).selection(SpaceKind::Euclidean3D);
    for (auto it = sel.ids.rbegin(); it != sel.ids.rend(); ++it) {
        const SceneObject* o = sp.find(*it);
        if (o && o->typeId() == "matrix" && o->value().type == expr::Type::Matrix) return o;
    }
    for (const auto& o : sp.objects())
        if (o->typeId() == "matrix" && o->visible && o->value().type == expr::Type::Matrix) return o.get();
    return nullptr;
}

Matrix4D App::matrix() const {
    const SceneObject* m = targetMatrix();
    return m ? m->value().matrix4() : Matrix4D::identity();
}

bool App::using4x4() const {
    const SceneObject* m = targetMatrix();
    return m && m->value().size == 4;
}

Vector3D App::vectorValue(const std::string& name) const {
    const SceneObject* o = m_scene.space(SpaceKind::Euclidean3D).find(name);
    if (o && o->value().type == expr::Type::Vector) return o->value().vector;
    return Vector3D::zero();
}

Complex App::complexValue(const std::string& name) const {
    const SceneObject* o = m_scene.space(SpaceKind::Complex).find(name);
    if (o && o->value().isNumber()) return Complex(static_cast<float>(o->value().re), static_cast<float>(o->value().im));
    return Complex(0, 0);
}

double App::parameterValue(const std::string& name, AppMode mode) const {
    const SceneObject* o = m_scene.space(kindOf(mode)).find(name);
    if (o && o->value().isRealNumber()) return o->value().re;
    return 0.0;
}

EigenResult App::eigen() const {
    const SceneObject* m = targetMatrix();
    return m ? eigen3(m->value().matrix3()) : EigenResult{};
}

// --- Scenes ----------------------------------------------------------------------

void App::saveScene() {
    if (m_currentFile.empty()) {
        saveSceneAs();
        return;
    }
    writeScene(m_currentFile);
}

void App::saveSceneAs() {
    closePopovers();
    std::string name = m_currentFile.empty() ? std::string(kSceneFile)
                                             : std::filesystem::path(m_currentFile).filename().string();
    m_fileDialog.open(FileDialog::Mode::Save, currentFolder(), name, m_recentFiles);
}

void App::loadScene() {
    closePopovers();
    m_fileDialog.open(FileDialog::Mode::Open, currentFolder(), "", m_recentFiles);
}

std::string App::currentFolder() const {
    if (!m_currentFile.empty()) return std::filesystem::path(m_currentFile).parent_path().string();
    return m_lastFolder;
}

void App::rememberFile(const std::string& path) {
    m_currentFile = path;
    m_lastFolder = std::filesystem::path(path).parent_path().string();
    m_recentFiles.erase(std::remove(m_recentFiles.begin(), m_recentFiles.end(), path), m_recentFiles.end());
    m_recentFiles.insert(m_recentFiles.begin(), path);
    if (m_recentFiles.size() > 6) m_recentFiles.resize(6);
}

void App::newScene() {
    m_scenePanel.cancelEditing();
    m_scene.timeline() = scene::Timeline{};
    m_scene.clearAll();
    m_scene.commit();  // undoable
    m_currentFile.clear();
    resetCamera();
    toast("New scene (Ctrl+Z brings the previous one back)");
}

void App::writeScene(const std::string& path) {
    SceneData data;
    m_scene.save(data);
    putInt(data, "mode", static_cast<int>(m_mode));
    Vector3 p = m_camera.position(), t = m_camera.target();
    putFloat(data, "camera.px", p.x);
    putFloat(data, "camera.py", p.y);
    putFloat(data, "camera.pz", p.z);
    putFloat(data, "camera.tx", t.x);
    putFloat(data, "camera.ty", t.y);
    putFloat(data, "camera.tz", t.z);
    m_viewPanel.saveState(data);

    std::string error;
    if (saveSceneFile(path, data, error)) {
        rememberFile(path);
        toast("Saved " + std::filesystem::path(path).filename().string());
    } else {
        toast("Save failed: " + error, true);
    }
}

void App::convertOldKeyframes(const SceneData& data) {
    // Files from before the Time panel stored matrix keyframes for the old Animation
    // panel; they become K = keys(s, ...) driven by an animated parameter s
    int count = 0;
    if (!getInt(data, "anim.keyframes", count) || count < 2) return;
    std::vector<std::string> mats;
    for (int k = 0; k < count; k++) {
        auto it = data.find("anim.key" + std::to_string(k));
        if (it == data.end()) continue;
        std::istringstream in(it->second);
        float m[4][4];
        bool ok = true;
        for (int i = 0; i < 4 && ok; i++)
            for (int j = 0; j < 4 && ok; j++) ok = static_cast<bool>(in >> m[i][j]);
        if (!ok) continue;
        std::ostringstream text;
        text << "[";
        for (int i = 0; i < 4; i++) {
            text << (i ? ", [" : "[");
            for (int j = 0; j < 4; j++) text << (j ? ", " : "") << m[i][j];
            text << "]";
        }
        text << "]";
        mats.push_back(text.str());
    }
    if (mats.size() < 2) return;
    scene::Space& sp = m_scene.space(SpaceKind::Euclidean3D);
    SceneObject* s = m_scene.add(SpaceKind::Euclidean3D, "parameter", "s");
    auto* p = static_cast<scene::ParameterObject*>(s);
    p->setMode(scene::ParameterObject::Mode::Animate);
    p->setProp("min", 0.0);
    p->setProp("max", 1.0);
    Playback pb = p->playback();
    pb.repeat = Repeat::Loop;
    pb.duration = 4.0f;
    p->setPlayback(pb);
    std::string formula = "keys(" + s->name();
    for (const auto& m : mats) formula += ", " + m;
    formula += ")";
    SceneObject* k = m_scene.add(SpaceKind::Euclidean3D, "matrix", "K");
    static_cast<scene::MatrixObject*>(k)->setSize(4);
    static_cast<scene::MatrixObject*>(k)->setByFormula(true);
    k->setField("value", formula);
    (void)sp;
    m_scene.evaluate();
    m_scene.commit();
    toast("Old keyframes became the matrix " + k->name() + " = keys(" + s->name() + ", ...)");
}

void App::readScene(const std::string& path) {
    SceneData data;
    std::string error;
    if (!loadSceneFile(path, data, error) || !m_scene.load(data, error)) {
        toast("Could not open " + std::filesystem::path(path).filename().string() + ": " + error, true);
        return;
    }
    m_scenePanel.cancelEditing();
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
    convertOldKeyframes(data);
    m_viewPanel.loadState(data);
    rememberFile(path);
    toast("Opened " + std::filesystem::path(path).filename().string() + " (Ctrl+Z goes back)");
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
    getBool(prefs, "ui.smooth_surfaces", m_uiSettings.smoothSurfaces);
    m_tutorialPanel.loadState(prefs);
    for (auto* p : m_panels.panels()) p->loadState(prefs);
    getString(prefs, "files.folder", m_lastFolder);
    m_recentFiles.clear();
    for (int i = 0; i < 6; i++) {
        std::string f;
        if (getString(prefs, "files.recent." + std::to_string(i), f) && !f.empty()) m_recentFiles.push_back(f);
    }
}

void App::savePreferences() {
    SceneData prefs;
    putFloat(prefs, "ui.zoom", ui::zoom());
    prefs["ui.theme"] = ui::themeKind() == ui::ThemeKind::Light ? "light" : "dark";
    putBool(prefs, "ui.tooltips", m_uiSettings.tooltips);
    putBool(prefs, "ui.hud", m_uiSettings.showHud);
    putBool(prefs, "ui.smooth_surfaces", m_uiSettings.smoothSurfaces);
    if (!IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) && !IsWindowFullscreen()) {
        putInt(prefs, "window.width", GetScreenWidth());
        putInt(prefs, "window.height", GetScreenHeight());
    }
    m_tutorialPanel.saveState(prefs);
    for (auto* p : m_panels.panels()) p->saveState(prefs);
    if (!m_lastFolder.empty()) prefs["files.folder"] = m_lastFolder;
    for (size_t i = 0; i < m_recentFiles.size(); i++) prefs["files.recent." + std::to_string(i)] = m_recentFiles[i];
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
    return m_scenePanel.isEditing() || m_referencePanel.isEditing() || m_fileDialog.isOpen();
}

void App::handleInput(float deltaTime) {
    bool editing = isEditing();

    // Escape steps back one level: cancel an edit, close a popup or the help,
    // or leave camera mode (frees the mouse). It never quits the app.
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (m_fileDialog.isOpen()) {
            m_fileDialog.close();
            editing = false;
        } else if (editing) {
            m_scenePanel.cancelEditing();
            m_referencePanel.cancelEditing();
            editing = false;
        } else if (m_addMenu.isOpen()) {
            m_addMenu.close();
        } else if (m_colorPopover.isOpen()) {
            m_colorPopover.close();
        } else if (m_settingsPopover.isOpen()) {
            m_settingsPopover.close();
        } else if (m_help.visible()) {
            m_help.toggle();
        } else if (IsCursorHidden()) {
            setUiMode(true);
        }
    }

    // TAB switches between camera mode (cursor captured) and UI mode.
    // While a field is being edited, TAB moves to the next field instead.
    if (!editing && IsKeyPressed(KEY_TAB)) setUiMode(IsCursorHidden());

    bool ui = uiMode();
    // Widgets only respond in UI mode, and not behind the help overlay
    if (ui && !m_help.visible()) GuiUnlock();
    else GuiLock();

    m_scenePanel.update(m_scene);

    if (!editing) handleShortcuts(ui);
    if (ui && !m_help.visible()) pickInViewport();

    if (m_mode != m_lastMode) {
        // A field in the hidden space would otherwise stay "active"
        m_scenePanel.finishEditing(m_scene);
        closePopovers();
        // Remember the camera of the space we leave
        int from = static_cast<int>(m_lastMode), to = static_cast<int>(m_mode);
        m_savedPosition[from] = m_camera.position();
        m_savedTarget[from] = m_camera.target();
        m_hasSavedView[from] = true;
        if (m_hasSavedView[to]) m_camera.setView(m_savedPosition[to], m_savedTarget[to]);
        else m_camera.reset(m_mode);
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
    if (IsKeyPressed(KEY_F5)) {
        if (shift) saveSceneAs();
        else saveScene();
    }
    if (IsKeyPressed(KEY_F9)) loadScene();
    if (IsKeyPressed(KEY_F2)) {
        m_referenceWindow.toggle();
        if (m_referenceWindow.visible()) m_panels.bringToFront(&m_referenceWindow);
    }
    if (IsKeyPressed(KEY_F1)) {
        m_tutorialWindow.toggle();
        if (m_tutorialWindow.visible()) m_panels.bringToFront(&m_tutorialWindow);
    }

    if (ctrl) {
        // Ctrl is "move down" in camera mode, so these are UI-mode only
        if (!uiMode) return;
        if (IsKeyPressed(KEY_Z)) {
            m_scenePanel.cancelEditing();
            m_scene.undo();
        }
        if (IsKeyPressed(KEY_Y)) {
            m_scenePanel.cancelEditing();
            m_scene.redo();
        }
        if (IsKeyPressed(KEY_D)) duplicateSelection();
        if (IsKeyPressed(KEY_S)) {
            if (shift) saveSceneAs();
            else saveScene();
        }
        if (IsKeyPressed(KEY_O)) loadScene();
        return;
    }

    if (IsKeyPressed(KEY_T)) toggleTheme();
    if (IsKeyPressed(KEY_H)) m_help.toggle();
    if (IsKeyPressed(KEY_G)) view.showGrid = !view.showGrid;
    if (IsKeyPressed(KEY_L)) view.showLabels = !view.showLabels;
    if (IsKeyPressed(KEY_M)) m_analysisWindow.toggle();
    if (IsKeyPressed(KEY_K)) m_stepsWindow.toggle();
    if (IsKeyPressed(KEY_B)) m_breakdownWindow.toggle();
    if (IsKeyPressed(KEY_C)) resetCamera();
    if (IsKeyPressed(KEY_ONE)) m_mode = AppMode::Space3D;
    if (IsKeyPressed(KEY_TWO)) m_mode = AppMode::Complex;
    if (IsKeyPressed(KEY_E)) {
        m_galleryWindow.toggle();
        if (m_galleryWindow.visible()) m_panels.bringToFront(&m_galleryWindow);
    }
    if (uiMode && (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_INSERT))) openAddMenu();
    if (uiMode && IsKeyPressed(KEY_DELETE)) deleteSelection();
    if (IsKeyPressed(KEY_P)) TimePanel::togglePlay(m_scene);

    if (m_mode != AppMode::Space3D) return;
    if (IsKeyPressed(KEY_I)) applyMatrixPreset(Matrix3D::identity());
    if (IsKeyPressed(KEY_R)) applyMatrixPreset(Matrix3D::rotation(3.14159265f / 4));
    if (IsKeyPressed(KEY_X)) applyMatrixPreset(Matrix3D::shear(0.5f, 0.0f));
    if (IsKeyPressed(KEY_F)) applyMatrixPreset(Matrix3D::scale(-1.0f, 1.0f));
    // S moves the camera backwards in camera mode
    if (uiMode && IsKeyPressed(KEY_S)) applyMatrixPreset(Matrix3D::scale(2.0f, 2.0f));
}

void App::deleteSelection() {
    SpaceKind kind = spaceKind();
    auto ids = m_scene.selection(kind).ids;
    if (ids.empty()) return;
    m_scenePanel.cancelEditing();
    for (auto id : ids) m_scene.space(kind).remove(id);
    m_scene.selection(kind).clear();
    m_scene.commit();
}

void App::duplicateSelection() {
    SpaceKind kind = spaceKind();
    scene::ObjectId id = m_scene.selection(kind).primary();
    if (id == scene::kNoObject) return;
    m_scenePanel.cancelEditing();
    if (SceneObject* copy = m_scene.space(kind).duplicate(id)) {
        m_scene.selection(kind).set(copy->id());
        m_scene.commit();
    }
}

void App::pickInViewport() {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || isEditing()) return;
    Vector2 mouse = GetMousePosition();
    // Only clicks on the 3D view itself, not on panels or popovers
    if (mouse.x < SideBar::width() || mouse.y < TopBar::height()) return;
    if (m_panels.anyContains(mouse)) return;
    if (m_addMenu.isOpen() || m_colorPopover.isOpen() || m_settingsPopover.isOpen() || m_fileDialog.isOpen()) return;

    const Camera3D& cam = m_camera.camera();
    SpaceKind kind = spaceKind();
    view::DrawOptions options = drawOptions();
    float best = ui::px(12.0f);
    scene::ObjectId hit = scene::kNoObject;
    auto consider = [&](Vector3 a, Vector3 b, scene::ObjectId id) {
        if (!inFront(cam, a) || !inFront(cam, b)) return;
        float d = distanceToSegment(mouse, GetWorldToScreen(a, cam), GetWorldToScreen(b, cam));
        if (d < best) {
            best = d;
            hit = id;
        }
    };

    for (const auto& obj : m_scene.space(kind).objects()) {
        const SceneObject& o = *obj;
        if (!o.visible || !o.value().ok()) continue;
        if (kind == SpaceKind::Euclidean3D) {
            if (o.typeId() == "vector") {
                Vector3D tail = static_cast<const scene::VectorObject&>(o).tail();
                consider(mathToWorld(tail), mathToWorld(tail + o.value().vector), o.id());
            } else if (o.typeId() == "matrix") {
                Matrix4D m = view::SceneView::displayedMatrix(o, options);
                Vector3D origin = m.transform(Vector3D::zero());
                for (Vector3D e : {Vector3D(1, 0, 0), Vector3D(0, 1, 0), Vector3D(0, 0, 1)})
                    consider(mathToWorld(origin), mathToWorld(origin + m.transformDirection(e)), o.id());
            }
        } else if (o.value().isNumber() && o.typeId() != "parameter") {
            consider({0, 0, 0}, {static_cast<float>(o.value().re), static_cast<float>(o.value().im), 0}, o.id());
        }
    }

    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    scene::Selection& sel = m_scene.selection(kind);
    if (hit != scene::kNoObject) {
        if (ctrl) sel.toggle(hit);
        else sel.set(hit);
    } else if (!ctrl) {
        sel.clear();
    }
}

void App::update(float deltaTime) {
    // The scene clock moves on; then everything is re-evaluated
    m_scene.advance(deltaTime);
    m_scene.evaluate();

    // Analysis follows the selection of the current space
    m_infoPanel.update(m_scene.space(spaceKind()), m_scene.selection(spaceKind()));

    // The breakdown acts on the target matrix
    m_breakdownPanel.setMatrix(matrix());
    bool breakdown = breakdownActive();
    if (breakdown && !m_breakdownWasVisible) m_breakdownPanel.restart();
    m_breakdownWasVisible = breakdown;
    if (breakdown) m_breakdownPanel.update(deltaTime);

    // Lesson setup and task checks
    if (m_tutorialWindow.visible()) m_tutorialPanel.update(*this);

    ViewSettings& view = m_viewPanel.settings();
    m_grid.setShowGrid(view.showGrid);
    m_grid.setShowAxes(view.showAxes);
    m_grid.setSize(static_cast<int>(view.gridSize));
    m_sceneView.setSmoothSurfaces(m_uiSettings.smoothSurfaces);
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

view::DrawOptions App::drawOptions() const {
    view::DrawOptions o;
    o.view = &const_cast<ViewPanel&>(m_viewPanel).settings();
    o.selection = &const_cast<scene::Scene&>(m_scene).selection(spaceKind());
    const SceneObject* target = targetMatrix();
    if (target && m_mode == AppMode::Space3D) {
        if (breakdownActive()) {
            o.overrideMatrix = target->id();
            o.overrideValue = m_breakdownPanel.current();
            int steps = static_cast<int>(m_breakdownPanel.breakdown().steps.size());
            int done = std::min(m_breakdownPanel.completedSteps(), steps);
            for (int k = 1; k <= done && k < steps; k++) o.ghosts.push_back(m_breakdownPanel.breakdown().after(k));
        }
    }
    return o;
}

void App::drawScene() {
    BeginMode3D(m_camera.camera());
    if (m_mode == AppMode::Space3D) m_grid.draw();
    else m_grid.drawPlaneXY();
    m_sceneView.draw3D(m_scene.space(spaceKind()), drawOptions());
    EndMode3D();
}

void App::drawSceneLabels() {
    m_sceneView.drawLabels(m_scene.space(spaceKind()), drawOptions(), m_camera.camera());
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
    bool popoverOwnsMouse = m_fileDialog.isOpen() || m_settingsPopover.contains(mouse) ||
                            m_colorPopover.contains(mouse) || m_addMenu.contains(mouse);
    bool wasLocked = GuiIsLocked();
    if (popoverOwnsMouse) GuiLock();

    if (m_uiSettings.showHud) drawHud();

    // Side bar: objects and inspector of the current space, then the view settings
    Rectangle area = m_sideBar.begin();
    float y = m_scenePanel.draw(area.x, area.y, area.width, m_scene, spaceKind(), m_sceneView);
    m_viewPanel.setCurrentFile(m_currentFile);
    y = m_viewPanel.draw(area.x, y, area.width, m_mode);
    m_sideBar.end(y);

    ScenePanelRequests requests = m_scenePanel.takeRequests();
    if (requests.openAdd) {
        closePopovers();
        m_addMenu.open(requests.addAnchor);
    }
    if (requests.openColor) {
        closePopovers();
        openColorPicker();
    }
    if (requests.loadDemo) {
        resetScene();
        toast("Demo scene loaded (Ctrl+Z goes back)");
    }
    if (requests.openReference) {
        m_referenceWindow.setVisible(true);
        m_panels.bringToFront(&m_referenceWindow);
    }
    if (requests.openTime) {
        m_animationWindow.setVisible(true);
        m_panels.bringToFront(&m_animationWindow);
    }
    if (!requests.message.empty()) toast(requests.message, requests.messageIsError);
    if (m_viewPanel.takeResetCameraRequest()) resetCamera();
    if (m_viewPanel.takeSaveRequest()) saveScene();
    if (m_viewPanel.takeSaveAsRequest()) saveSceneAs();
    if (m_viewPanel.takeLoadRequest()) loadScene();
    if (m_viewPanel.takeNewRequest()) newScene();

    drawFloatingPanels();

    TopBarState state;
    state.helpVisible = m_help.visible();
    state.settingsOpen = m_settingsPopover.isOpen();
    state.fullscreen = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
    state.stepsVisible = m_stepsWindow.visible();
    state.breakdownVisible = m_breakdownWindow.visible();
    state.galleryVisible = m_galleryWindow.visible();
    state.tutorialVisible = m_tutorialWindow.visible();
    state.space3D = (m_mode == AppMode::Space3D);
    state.analysisVisible = m_analysisWindow.visible();
    state.timeVisible = m_animationWindow.visible();
    state.playing = m_scene.timeline().playing;
    state.referenceVisible = m_referenceWindow.visible();
    TopBarActions actions = m_topBar.draw(m_mode, state);
    if (actions.toggleTheme) toggleTheme();
    if (actions.toggleHelp) m_help.toggle();
    if (actions.toggleFullscreen) toggleFullscreen();
    if (actions.screenshot) requestScreenshot(false);
    auto toggleWindow = [this](ui::FloatingPanel& w) {
        w.toggle();
        if (w.visible()) m_panels.bringToFront(&w);
    };
    if (actions.toggleTutorial) toggleWindow(m_tutorialWindow);
    if (actions.toggleGallery) toggleWindow(m_galleryWindow);
    if (actions.toggleBreakdown) toggleWindow(m_breakdownWindow);
    if (actions.toggleSteps) toggleWindow(m_stepsWindow);
    if (actions.toggleAnalysis) toggleWindow(m_analysisWindow);
    if (actions.toggleTime) toggleWindow(m_animationWindow);
    if (actions.toggleReference) toggleWindow(m_referenceWindow);
    if (actions.togglePlay) TimePanel::togglePlay(m_scene);
    if (actions.toggleSettings) {
        bool open = !m_settingsPopover.isOpen();
        closePopovers();
        showSettings(open);
    }

    if (popoverOwnsMouse && !wasLocked) GuiUnlock();

    SettingsActions settingsActions = m_settingsPopover.draw(m_uiSettings);
    if (settingsActions.resetLayout) {
        m_panels.resetLayout();
        toast("Panel layout reset");
    }

    ColorPopover::Result color = m_colorPopover.draw();
    if (color.changed || color.reset) {
        SceneObject* o = m_scene.space(spaceKind()).find(static_cast<scene::ObjectId>(m_colorPopover.slot()));
        if (o) {
            o->customColor = color.changed && !color.reset;
            if (o->customColor) o->color = {color.color.r, color.color.g, color.color.b, 255};
            o->touch();
            m_scene.commit();
        }
    }

    AddChoice choice;
    if (m_addMenu.draw(m_scene, spaceKind(), choice)) {
        if (SceneObject* o = applyAddChoice(m_scene, spaceKind(), choice)) toast("Added " + o->name());
    }

    drawTutorialHighlight();
    if (m_fileDialog.draw()) {
        if (m_fileDialog.mode() == FileDialog::Mode::Save) writeScene(m_fileDialog.path());
        else readScene(m_fileDialog.path());
    }
    drawToast(deltaTime);
    m_help.draw();
    ui::drawTooltip(m_uiSettings.tooltips && !m_help.visible());
}

StepsInput App::stepsInput() const {
    StepsInput in;
    in.mode = m_mode;
    in.matrix = matrix();
    in.is4x4 = using4x4();
    // Vectors: the selected ones, else a and b
    std::vector<Vector3D> vectors;
    const scene::Space& sp = m_scene.space(SpaceKind::Euclidean3D);
    for (auto id : const_cast<scene::Scene&>(m_scene).selection(SpaceKind::Euclidean3D).ids) {
        const SceneObject* o = sp.find(id);
        if (o && o->value().type == expr::Type::Vector) vectors.push_back(o->value().vector);
    }
    in.a = vectors.size() > 0 ? vectors[0] : vectorA();
    in.b = vectors.size() > 1 ? vectors[1] : vectorB();
    // Complex numbers: the selected ones, else z1 and z2
    std::vector<Complex> numbers;
    const scene::Space& cp = m_scene.space(SpaceKind::Complex);
    for (auto id : const_cast<scene::Scene&>(m_scene).selection(SpaceKind::Complex).ids) {
        const SceneObject* o = cp.find(id);
        if (o && o->value().isNumber() && o->typeId() != "parameter")
            numbers.push_back(Complex(static_cast<float>(o->value().re), static_cast<float>(o->value().im)));
    }
    in.z1 = numbers.size() > 0 ? numbers[0] : complexValue("z1");
    in.z2 = numbers.size() > 1 ? numbers[1] : complexValue("z2");
    in.theta = static_cast<float>(parameterValue("theta", AppMode::Complex));
    return in;
}

void App::drawFloatingPanels() {
    bool space3d = (m_mode == AppMode::Space3D);
    for (ui::FloatingPanel* p : m_panels.panels()) {
        Rectangle content;
        if (p == &m_analysisWindow) {
            if (p->begin(m_analysisHeight, content)) {
                m_analysisHeight = m_infoPanel.drawContent(content);
                p->end();
            }
        } else if (p == &m_referenceWindow) {
            if (p->begin(m_referenceHeight, content)) {
                m_referenceHeight = m_referencePanel.drawContent(content, &m_scene.space(spaceKind()),
                                                                 m_scene.timeline().time);
                p->end();
            }
        } else if (p == &m_animationWindow) {
            if (p->begin(m_animationHeight, content)) {
                m_animationHeight = m_timePanel.drawContent(content, m_scene, spaceKind());
                p->end();
            }
        } else if (p == &m_breakdownWindow) {
            if (!space3d) continue;
            if (p->begin(m_breakdownHeight, content)) {
                m_breakdownHeight = m_breakdownPanel.drawContent(content);
                p->end();
            }
        } else if (p == &m_tutorialWindow) {
            if (p->begin(m_tutorialHeight, content)) {
                m_tutorialHeight = m_tutorialPanel.drawContent(content, *this);
                p->end();
            }
        } else if (p == &m_galleryWindow) {
            if (p->begin(m_galleryHeight, content)) {
                int chosen = -1;
                m_galleryHeight = m_galleryPanel.drawContent(content, chosen);
                p->end();
                if (chosen >= 0) {
                    const Example& e = examples()[chosen];
                    m_scenePanel.cancelEditing();
                    e.apply(*this);
                    m_galleryWindow.setVisible(false);
                    toast(std::string("Example: ") + e.title);
                }
            }
        } else if (p == &m_stepsWindow) {
            if (p->begin(m_stepsHeight, content)) {
                m_stepsHeight = m_stepsPanel.drawContent(content, stepsInput());
                p->end();
            }
        }
    }
}

void App::drawTutorialHighlight() {
    if (!m_tutorialWindow.visible() || m_tutorialWindow.collapsed()) return;
    std::string id = m_tutorialPanel.currentAnchor();
    Rectangle r;
    if (id.empty() || !ui::findAnchor(id, r)) return;
    // Pulsing outline around the control the current step is about
    float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 4.0f);
    float grow = ui::px(4.0f + 2.0f * pulse);
    Rectangle ring = {r.x - grow, r.y - grow, r.width + 2 * grow, r.height + 2 * grow};
    Color c = ui::theme().accent;
    Color soft = c;
    soft.a = static_cast<unsigned char>(40 + 50 * pulse);
    ui::strokeRounded(ring, ui::px(ui::kRadius + 4), ui::px(5.0f), soft);
    ui::strokeRounded(ring, ui::px(ui::kRadius + 4), ui::pxi(2.0f), c);
}

void App::drawHud() {
    const ui::Theme& t = ui::theme();
    bool ui = uiMode();
    float x = viewportLeft() + ui::px(16.0f);
    float bottom = static_cast<float>(GetScreenHeight()) - ui::px(14.0f);

    const char* modeText = ui ? "UI mode  ·  click objects to select  ·  Tab: camera  ·  H: help"
                              : "Camera mode  ·  WASD move, mouse look  ·  Esc or Tab: free the mouse";
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
