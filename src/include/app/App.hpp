#pragma once

#include "animation/Animator.hpp"
#include "math/Matrix3D.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"
#include "rendering/ArgandRenderer.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/Grid.hpp"
#include "rendering/PlotRenderer.hpp"
#include "rendering/TransformVisualizer.hpp"
#include "rendering/VectorRenderer.hpp"
#include "ui/AnimationPanel.hpp"
#include "ui/AppMode.hpp"
#include "ui/BreakdownPanel.hpp"
#include "ui/ColorPopover.hpp"
#include "ui/ComplexPanel.hpp"
#include "ui/HelpOverlay.hpp"
#include "ui/InfoPanel.hpp"
#include "ui/InputPanel.hpp"
#include "ui/PlotPanel.hpp"
#include "ui/Preferences.hpp"
#include "ui/SettingsPopover.hpp"
#include "ui/SideBar.hpp"
#include "ui/StepsPanel.hpp"
#include "ui/TopBar.hpp"
#include "ui/ViewPanel.hpp"
#include "ui/core/FloatingPanel.hpp"

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

    // --- Scene control ------------------------------------------------------
    AppMode mode() const { return m_mode; }
    void setMode(AppMode mode);
    void setMatrix3(const Matrix3D& m) { m_inputPanel.setMatrix3(m); }
    void setMatrix4(const Matrix4D& m) { m_inputPanel.setMatrix4(m); }
    void setVectors(const Vector3D& a, const Vector3D& b) { m_inputPanel.setVectors(a, b); }
    void applyPreset(MatrixPreset preset) { m_inputPanel.applyPreset(preset); }
    void setComplexScene(const ComplexScene& scene) { m_complexPanel.setScene(scene); }
    void setPlotSpec(const PlotSpec& spec) { m_plotPanel.setSpec(spec); }
    void setCameraView(Vector3 position, Vector3 target) { m_camera.setView(position, target); }
    void resetCamera();
    void startAnimation(AnimationPreset preset) { m_animator.start(preset); }
    void stopAnimation() { m_animator.stop(); }
    void setUiMode(bool ui);  // true: cursor visible, panels active
    void showHelp(bool show);
    void showSettings(bool show);
    void openColorPicker(int slot);  // VectorSlot, anchored at the top of the viewport
    void closePopovers();
    void setTheme(bool dark);
    void toast(const std::string& message, bool error = false);

    // Floating panels by id ("analysis", "animation", ...)
    ui::FloatingPanel* panel(const std::string& id);

    // --- Read-only state (tutorial checks) ----------------------------------
    const Matrix4D& matrix() const { return m_matrix; }
    const Vector3D& vectorA() const { return m_vecA; }
    const Vector3D& vectorB() const { return m_vecB; }
    bool using4x4() const { return m_inputPanel.isUsing4x4(); }
    const ComplexScene& complexScene() const { return m_complexPanel.scene(); }
    PlotSpec plotSpec() const { return m_plotPanel.spec(); }
    const PlotRenderer& plot() const { return m_plotRenderer; }
    const Animator& animator() const { return m_animator; }
    const EigenResult& eigen() const { return m_infoPanel.eigen(); }
    ViewSettings& view() { return m_viewPanel.settings(); }
    bool uiMode() const;
    bool breakdownActive() const;
    BreakdownPanel& breakdownPanel() { return m_breakdownPanel; }

    void saveScene();
    void loadScene();
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
    void toggleTheme();
    void toggleFullscreen();
    void takeScreenshot();

    void loadPreferences(SceneData& prefs);
    void applyPreferences(const SceneData& prefs);
    void savePreferences();

    // Space between the side bar and any panel docked on the right (pixels)
    float viewportLeft() const;
    float viewportRight() const;

    AppOptions m_options;

    CameraController m_camera;
    Grid m_grid;
    VectorRenderer m_vectorRenderer;
    TransformVisualizer m_transformViz;
    InputPanel m_inputPanel;
    ComplexPanel m_complexPanel;
    ArgandRenderer m_argand;
    PlotPanel m_plotPanel;
    PlotRenderer m_plotRenderer;
    Animator m_animator;
    AnimationPanel m_animationPanel;
    InfoPanel m_infoPanel;
    ViewPanel m_viewPanel;
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
    float m_breakdownHeight = 0.0f;
    bool m_breakdownWasVisible = false;
    float m_stepsHeight = 0.0f;
    ui::PanelManager m_panels;
    float m_analysisHeight = 0.0f;
    float m_animationHeight = 0.0f;

    AppMode m_mode = AppMode::LinearAlgebra;
    AppMode m_lastMode = AppMode::LinearAlgebra;

    // Committed inputs, refreshed when the input panel reports a change
    Matrix4D m_matrix = Matrix4D::identity();
    Vector3D m_vecA, m_vecB;
    Matrix4D m_shown = Matrix4D::identity();  // what the scene displays (may be animated)

    int m_screenshotRequest = 0;  // 0 none, 1 full window, 2 scene only
    bool m_plotWas3D = true;
};

} // namespace linalg
