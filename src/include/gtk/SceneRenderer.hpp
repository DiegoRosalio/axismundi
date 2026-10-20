#pragma once

// The 3D view of the GTK app, without any GTK: raylib's GL layer (rlgl) set up inside a GL context that
// someone else made (a Gtk::GLArea), then the same drawing as the raylib app: grid, SceneView, labels.
// Only raylib headers here: the GTK widget around it is gtk/SceneArea.

#include "rendering/CameraController.hpp"
#include "rendering/Grid.hpp"
#include "rendering/GridShader.hpp"
#include "scene/Scene.hpp"
#include "ui/AppMode.hpp"
#include "ui/ViewPanel.hpp"
#include "view/SceneView.hpp"
#include <memory>
#include <string>

namespace linalg::gtk {

class SceneRenderer {
public:
    explicit SceneRenderer(scene::Scene& scene);
    ~SceneRenderer();

    // With the GL context current: load the GL functions and start rlgl. False (and why) if that fails.
    bool start(int pixelWidth, int pixelHeight, std::string& error);
    // With the GL context current: free GPU memory (meshes, shaders, font atlases)
    void stop();
    bool started() const { return m_started; }

    // Evaluate the scene (advancing the clock by dt while it plays) and draw a frame into the bound framebuffer
    void render(int pixelWidth, int pixelHeight, double dt);

    // Camera input in the widget's coordinates (logical pixels)
    void orbit(float dx, float dy, float viewHeight);
    void pan(float dx, float dy, float viewHeight);
    void zoom(float wheelSteps, float viewHeight);

    void setMode(AppMode mode);
    AppMode mode() const { return m_mode; }
    // Something moves on its own (the clock plays), so frames must keep coming
    bool animating() const;

    scene::Scene& scene() { return m_scene; }
    ViewSettings& view() { return m_view; }
    CameraController& camera() { return m_camera; }

private:
    scene::SpaceKind spaceKind() const;
    double pixelsPerUnit(int pixelHeight) const;
    double tickStep(int pixelHeight) const;
    view::DrawOptions drawOptions(int pixelHeight);
    void drawGrid(int pixelHeight);
    void drawLabels(int pixelHeight);

    scene::Scene& m_scene;
    AppMode m_mode = AppMode::Space3D;
    ViewSettings m_view;
    CameraController m_camera;
    std::unique_ptr<view::SceneView> m_sceneView;  // holds GPU caches: made after start, freed in stop
    std::unique_ptr<GridShader> m_gridShader;
    Grid m_grid;
    bool m_started = false;
};

} // namespace linalg::gtk
