#include "app/App.hpp"
#include "ui/AppIcon.hpp"
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

SpaceKind App::kindOf(AppMode mode) { return Session::kindOf(mode); }

App::App(const AppOptions& options)
    : m_options(options), m_viewPanel(m_session.view()),
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
    // Just the app's name: GLFW also uses it as the X11 window class, which GNOME matches with
    // StartupWMClass in data/com.bisonanon.axismundi.desktop to show the right icon in the dock
    InitWindow(width, height, "axismundi");
    ui::installClickLatch();  // touchpad taps shorter than a frame still count as clicks
    ui::setWindowIcons();
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




void App::clearSpace(AppMode mode) {
    m_scenePanel.cancelEditing();
    m_session.clearSpace(mode);
}

void App::resetScene() {
    m_scenePanel.cancelEditing();
    m_session.resetScene();
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

void App::showPanel(const std::string& id, bool visible) {
    if (ui::FloatingPanel* p = panel(id)) p->setVisible(visible);
}

bool App::panelVisible(const std::string& id) const {
    ui::FloatingPanel* p = const_cast<App*>(this)->panel(id);
    return p && p->visible();
}

void App::restartBreakdown() { m_breakdownPanel.restart(); }

bool App::breakdownFinished() const {
    return m_breakdownPanel.progress() >= static_cast<float>(m_breakdownPanel.breakdown().steps.size()) - 1e-3f;
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
    m_session.applyViewStyle();
    bool smooth = m_uiSettings.surfaces == UiSettings::Surfaces::StyleDefault ? view::activeStyle().smoothDefault
                                                                                : m_uiSettings.surfaces == UiSettings::Surfaces::Smooth;
    m_sceneView.setSmoothSurfaces(smooth);
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

} // namespace linalg
