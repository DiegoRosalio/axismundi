#pragma once

#include "animation/Playback.hpp"
#include "math/Complex.hpp"
#include "math/Matrix3D.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/Grid.hpp"
#include "scene/Scene.hpp"
#include "ui/AddMenu.hpp"
#include "ui/AppMode.hpp"
#include "ui/BreakdownPanel.hpp"
#include "ui/ColorPopover.hpp"
#include "ui/FileDialog.hpp"
#include "ui/GalleryPanel.hpp"
#include "ui/HelpOverlay.hpp"
#include "ui/InfoPanel.hpp"
#include "ui/Preferences.hpp"
#include "ui/ReferencePanel.hpp"
#include "ui/ExportPanel.hpp"
#include "ui/ScenePanel.hpp"
#include "ui/SettingsPopover.hpp"
#include "ui/SideBar.hpp"
#include "ui/StepsPanel.hpp"
#include "ui/TopBar.hpp"
#include "ui/TimePanel.hpp"
#include "ui/TutorialPanel.hpp"
#include "ui/ViewPanel.hpp"
#include "ui/core/FloatingPanel.hpp"
#include "view/SceneView.hpp"

#include <map>
#include <memory>
#include <string>

namespace linalg {

class Tour;

struct AppOptions {
    int width = 1600;
    int height = 900;
    bool sizeGiven = false;  // --size was passed (overrides the saved window size)
    std::string tourDir;     // non-empty: run the scripted screenshot tour, then exit
    std::string theme;       // "dark" or "light" (empty: saved preference)
};

// Owns every component and runs the main loop. The public methods are the
// API used by the gallery, tutorials and the screenshot tour.
class App {
public:
    explicit App(const AppOptions& options);
    ~App();

    void run();

    // --- Spaces and objects -----------------------------------------------------
    AppMode mode() const { return m_mode; }
    void setMode(AppMode mode);
    scene::Scene& scene() { return m_scene; }
    scene::SpaceKind spaceKind() const { return kindOf(m_mode); }
    scene::Space& space(AppMode mode) { return m_scene.space(kindOf(mode)); }
    static scene::SpaceKind kindOf(AppMode mode);

    // Find an object by name in a space (nullptr if missing)
    scene::SceneObject* object(const std::string& name, AppMode mode = AppMode::Space3D);
    // Add an object with preset fields and properties; it becomes selected. One undo step.
    scene::SceneObject* addObject(AppMode mode, const std::string& typeId, const std::string& name,
                                  const std::map<std::string, std::string>& fields = {},
                                  const std::map<std::string, std::string>& props = {});
    void select(const std::string& name, AppMode mode = AppMode::Space3D, bool add = false);
    void clearSpace(AppMode mode);
    void resetScene();  // both spaces back to the default objects

    // Shortcuts kept for examples and tutorials: they edit the objects M, a and b
    void setMatrix3(const Matrix3D& m);
    void setMatrix4(const Matrix4D& m);
    void setVectors(const Vector3D& a, const Vector3D& b);
    void applyMatrixPreset(const Matrix3D& m);  // to the selected matrix (or M)

    // --- View --------------------------------------------------------------------
    void setCameraView(Vector3 position, Vector3 target) { m_camera.setView(position, target); }
    void resetCamera();
    // Scene clock: back to t = 0, then play or stay paused
    void restartTime(bool play) {
        m_scene.timeline().time = 0.0;
        m_scene.timeline().playing = play;
    }
    void setUiMode(bool ui);  // true: cursor visible, panels active
    void showHelp(bool show);
    void showSettings(bool show);
    void openAddMenu();
    void openColorPicker();  // for the selected object
    void closePopovers();
    void setTheme(bool dark);
    void setSmoothSurfaces(bool smooth) { m_uiSettings.smoothSurfaces = smooth; }
    void toast(const std::string& message, bool error = false);

    // Floating panels by id ("analysis", "animation" (the Time panel), ...)
    ui::FloatingPanel* panel(const std::string& id);

    // --- Read-only state (tutorial checks) ----------------------------------
    // The matrix the breakdown and steps use: the selected matrix, else the first one
    const scene::SceneObject* targetMatrix() const;
    Matrix4D matrix() const;
    bool using4x4() const;
    Vector3D vectorValue(const std::string& name) const;
    Vector3D vectorA() const { return vectorValue("a"); }
    Vector3D vectorB() const { return vectorValue("b"); }
    Complex complexValue(const std::string& name) const;
    double parameterValue(const std::string& name, AppMode mode) const;
    EigenResult eigen() const;
    ViewSettings& view() { return m_viewPanel.settings(); }
    const view::SceneView& sceneView() const { return m_sceneView; }
    bool uiMode() const;
    bool breakdownActive() const;
    BreakdownPanel& breakdownPanel() { return m_breakdownPanel; }
    TutorialPanel& tutorialPanel() { return m_tutorialPanel; }
    ScenePanel& scenePanel() { return m_scenePanel; }
    ReferencePanel& referencePanel() { return m_referencePanel; }
    ExportPanel& exportPanel() { return m_exportPanel; }
    // The LaTeX figure of the current space with the export panel's settings
    std::string exportLatexCode() const;
    bool addMenuOpen() const { return m_addMenu.isOpen(); }

    void saveScene();     // to the current file (asks for a name the first time)
    void saveSceneAs();   // opens the Save dialog
    void loadScene();     // opens the Open dialog
    void newScene();      // empty both spaces (undoable)
    void writeScene(const std::string& path);
    void readScene(const std::string& path);
    const std::string& currentFile() const { return m_currentFile; }
    FileDialog& fileDialog() { return m_fileDialog; }
    // Screenshot at the end of this frame (sceneOnly hides the UI)
    void requestScreenshot(bool sceneOnly);

private:
    struct Toast {
        std::string text;
        bool error = false;
        float timeLeft = 0.0f;
    };

    bool isEditing() const;
    void handleInput(float deltaTime);
    void handleShortcuts(bool uiMode);
    void update(float deltaTime);
    void drawScene();
    void drawSceneLabels();
    void drawUi(float deltaTime);
    void drawFloatingPanels();
    void drawToast(float deltaTime);
    void drawHud();
    void drawTutorialHighlight();
    void toggleTheme();
    void toggleFullscreen();
    void takeScreenshot();
    void pickInViewport();
    void deleteSelection();
    void duplicateSelection();
    StepsInput stepsInput() const;
    view::DrawOptions drawOptions() const;

    void convertOldKeyframes(const SceneData& data);
    void applyModeChange();  // camera swap and cleanup when the space changes
    std::string currentFolder() const;
    void rememberFile(const std::string& path);
    void loadPreferences(SceneData& prefs);
    void applyPreferences(const SceneData& prefs);
    void savePreferences();

    float viewportLeft() const;
    float viewportRight() const;

    AppOptions m_options;

    scene::Scene m_scene;
    view::SceneView m_sceneView;
    CameraController m_camera;
    Grid m_grid;
    TimePanel m_timePanel;
    InfoPanel m_infoPanel;
    ViewPanel m_viewPanel;
    ScenePanel m_scenePanel;
    AddMenu m_addMenu;
    SideBar m_sideBar;
    TopBar m_topBar;
    HelpOverlay m_help;
    SettingsPopover m_settingsPopover;
    ColorPopover m_colorPopover;
    UiSettings m_uiSettings;
    Toast m_toast;
    std::unique_ptr<Tour> m_tour;

    // Floating panels
    ui::FloatingPanel m_analysisWindow;
    ui::FloatingPanel m_animationWindow;
    ui::FloatingPanel m_stepsWindow;
    StepsPanel m_stepsPanel;
    ui::FloatingPanel m_breakdownWindow;
    BreakdownPanel m_breakdownPanel;
    ui::FloatingPanel m_galleryWindow;
    GalleryPanel m_galleryPanel;
    ui::FloatingPanel m_tutorialWindow;
    TutorialPanel m_tutorialPanel;
    ui::FloatingPanel m_referenceWindow;
    ReferencePanel m_referencePanel;
    float m_referenceHeight = 0.0f;
    ui::FloatingPanel m_exportWindow;
    ExportPanel m_exportPanel;
    float m_exportHeight = 0.0f;
    void exportView(double& azimuth, double& elevation) const;  // from the camera
    void writeLatex(const std::string& path);
    FileDialog m_fileDialog;
    std::string m_currentFile;               // scene file of the last save / open
    std::string m_lastFolder;                // where the file dialog starts
    std::vector<std::string> m_recentFiles;  // most recent first
    ui::PanelManager m_panels;
    float m_analysisHeight = 0.0f;
    float m_animationHeight = 0.0f;
    float m_stepsHeight = 0.0f;
    float m_breakdownHeight = 0.0f;
    float m_galleryHeight = 0.0f;
    float m_tutorialHeight = 0.0f;
    bool m_breakdownWasVisible = false;

    AppMode m_mode = AppMode::Space3D;
    AppMode m_lastMode = AppMode::Space3D;
    // Camera per space, so switching tabs keeps each viewpoint
    Vector3 m_savedPosition[kAppModeCount] = {};
    Vector3 m_savedTarget[kAppModeCount] = {};
    bool m_hasSavedView[kAppModeCount] = {};

    int m_screenshotRequest = 0;  // 0 none, 1 full window, 2 scene only
};

} // namespace linalg
