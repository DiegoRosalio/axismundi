#include "app/App.hpp"
#include "view/ViewStyle.hpp"
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
#include <fstream>
#include <sstream>
#include <cmath>
#include <csignal>
#include <ctime>

namespace linalg {

using scene::SceneObject;
using scene::SpaceKind;

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
      m_referenceWindow("reference", "Formula reference", 620.0f, {0.45f, 0.1f}, false),
      m_exportWindow("export", "Export to LaTeX", 470.0f, {1.0f, 0.3f}, false) {
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
    ui::installClickLatch();  // touchpad taps shorter than a frame still count as clicks
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
    m_panels.add(&m_exportWindow);

    applyPreferences(prefs);
    if (options.theme == "light") ui::setTheme(ui::ThemeKind::Light);
    else if (options.theme == "dark") ui::setTheme(ui::ThemeKind::Dark);
    ui::init();

    if (!options.tourDir.empty()) {
        m_tour = std::make_unique<Tour>(options.tourDir);
        ui::setFixedStep(1.0f / 60.0f);  // the same clock every run, so screenshots can be compared
    }

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
    // Swap cameras now: a camera reset right after (as gallery examples do) must apply to the
    // new space, not be saved as the old space's view on the next frame
    applyModeChange();
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

void App::setSideBarCollapsed(bool collapsed) {
    if (collapsed) m_scenePanel.finishEditing(m_scene);  // a field being edited would be hidden
    SideBar::setCollapsed(collapsed);
}

void App::openFormulaEditor(const std::string& objectName, const std::string& key) {
    SceneObject* o = m_scene.space(spaceKind()).find(objectName);
    if (!o || !o->field(key)) return;
    scene::ObjectId oid = o->id();
    SpaceKind kind = spaceKind();
    scene::Scene* sc = &m_scene;
    m_formulaEditor.open(kind, oid, key, o->field(key)->text, [sc, kind, oid, key](const std::string& text) {
        SceneObject* obj = sc->space(kind).find(oid);
        if (!obj) return false;
        obj->setField(key, text);
        sc->commit();
        return true;
    });
}

// ---------------------------------------------------------------------------
// Preferences
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------

namespace {
// Testing hook (tools/dev/input-test.sh): SIGUSR1 saves a screenshot of the next frame as
// signal_NNN.png in the current folder, whatever the keyboard focus is
volatile std::sig_atomic_t g_shotSignal = 0;
void onShotSignal(int) { g_shotSignal = 1; }
}

void App::run() {
#ifdef SIGUSR1
    std::signal(SIGUSR1, onShotSignal);
#endif
    int signalShots = 0;
    while (!WindowShouldClose()) {
        float deltaTime = ui::frameStep(GetFrameTime());
        bool signalShot = g_shotSignal != 0;
        g_shotSignal = 0;

        if (m_tour && !m_tour->advance(*this)) break;

        ui::beginFrame();
        m_panels.beginFrame();
        handleInput(deltaTime);
        update(deltaTime);

        BeginDrawing();
            ClearBackground(view::activeStyle().background);
            drawScene();
            drawSceneLabels();
            if (m_screenshotRequest != 2) drawUi(deltaTime);
            if (m_tour) m_tour->capture();
            if (m_screenshotRequest) takeScreenshot();
            if (signalShot) {
                char name[32];
                snprintf(name, sizeof(name), "signal_%03d.png", signalShots++);
                rlDrawRenderBatchActive();
                Image image = LoadImageFromScreen();
                ExportImage(image, name);
                UnloadImage(image);
            }
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
    // A PDF compile finished in the background
    latex::PdfResult pdf;
    std::string pdfPath;
    if (m_exportPanel.takePdfResult(pdf, pdfPath)) {
        if (pdf.ok) toast("Saved " + std::filesystem::path(pdfPath).filename().string());
        else toast("PDF failed: see the export panel", true);
    }

    // The export panel's B/W preview is a shortcut to the Paper style (and back to the previous one)
    bool preview = m_exportPanel.preview();
    if (preview != m_previewShown) {
        if (preview) {
            if (view.style != view::StyleId::Paper) m_styleBeforePaper = view.style;
            view.style = view::StyleId::Paper;
        } else if (view.style == view::StyleId::Paper) {
            view.style = m_styleBeforePaper;
        }
    } else if (view.style != view::StyleId::Paper) {
        m_styleBeforePaper = view.style;
    }
    m_previewShown = view.style == view::StyleId::Paper;
    m_exportPanel.setPreview(m_previewShown);

    // The view style for this frame (colors follow the theme, so recomputed every frame)
    view::setActiveStyle(view::styleFor(view.style, ui::theme()));
    m_grid.setGridColor(view::activeStyle().grid);
    bool smooth = m_uiSettings.surfaces == UiSettings::Surfaces::StyleDefault ? view::activeStyle().smoothDefault
                                                                                : m_uiSettings.surfaces == UiSettings::Surfaces::Smooth;
    m_sceneView.setSmoothSurfaces(smooth);
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

} // namespace linalg
