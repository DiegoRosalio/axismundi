#pragma once

// Session: what both front ends (the raylib app and the GTK app) share. The scene, the space being shown, the
// view settings, the camera (one saved view per space), the 3D view with its caches and the grid, the drawing
// steps that don't depend on the window, and the scripting functions the tutorial, the gallery and the tours
// use (add, select, set matrices, read values). No windows, panels or input: the front ends own those.

#include "gfx/Overlay.hpp"
#include "math/Complex.hpp"
#include "math/Decompositions.hpp"
#include "math/Matrix3D.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/Grid.hpp"
#include "rendering/GridShader.hpp"
#include "scene/Scene.hpp"
#include "ui/AppMode.hpp"
#include "ui/SceneIO.hpp"
#include "view/ViewSettings.hpp"
#include "view/SceneView.hpp"
#include <map>
#include <memory>
#include <string>

namespace linalg {

class Session {
public:
    Session();

    // --- Spaces ------------------------------------------------------------------
    scene::Scene& scene() { return m_scene; }
    const scene::Scene& scene() const { return m_scene; }
    AppMode mode() const { return m_mode; }
    scene::SpaceKind spaceKind() const { return kindOf(m_mode); }
    static scene::SpaceKind kindOf(AppMode mode);
    scene::Space& space(AppMode mode) { return m_scene.space(kindOf(mode)); }
    // Show another space: the camera of the space left is remembered, the one of the new space restored
    void setMode(AppMode mode);
    // Take a space from a scene file without swapping cameras (the file's camera follows)
    void adoptMode(AppMode mode);

    // --- View ----------------------------------------------------------------------
    ViewSettings& view() { return m_view; }
    const ViewSettings& view() const { return m_view; }
    CameraController& camera() { return m_camera; }
    const CameraController& camera() const { return m_camera; }
    view::SceneView& sceneView() { return m_sceneView; }
    const view::SceneView& sceneView() const { return m_sceneView; }
    // The view style of this frame (colors follow the theme): call once per frame before drawing
    void applyViewStyle();
    // Free GPU memory (plot and field meshes, the grid shader) while the GL context still exists
    void releaseGpu();

    // --- Drawing (heights in pixels of the 3D view) --------------------------------------
    // Pixels one unit covers at the view center; the grid spacing and the axis numbers follow it
    double pixelsPerUnit(int viewHeight) const;
    double tickStep(int viewHeight) const;
    // The options every front end uses; they add their own (print scope, breakdown, fading)
    view::DrawOptions drawOptions(int viewHeight) const;
    // Inside the 3D camera: the grid (shader, or lines when the shader can't run), then the objects
    void drawGrid(int viewHeight);
    void draw3D(const view::DrawOptions& options);
    // After the 3D camera: axis numbers and labels, as a list for the front end to paint
    void labels(const view::DrawOptions& options, int viewHeight, const gfx::TextMeasure& measure,
                gfx::Overlay& out);

    // --- Scripting (tutorial, gallery, tours) ----------------------------------------
    scene::SceneObject* object(const std::string& name, AppMode mode = AppMode::Space3D);
    // Add an object with preset fields and properties; it becomes selected. One undo step.
    scene::SceneObject* addObject(AppMode mode, const std::string& typeId, const std::string& name,
                                  const std::map<std::string, std::string>& fields = {},
                                  const std::map<std::string, std::string>& props = {});
    void select(const std::string& name, AppMode mode = AppMode::Space3D, bool add = false);
    void clearSpace(AppMode mode);
    // On the current space's selection, one undo step each; false when nothing was selected
    bool deleteSelection();
    bool duplicateSelection();  // the copy of the primary object becomes the selection
    void resetScene();  // both spaces back to the default objects
    // The objects M, a and b (made when missing)
    void setMatrix3(const Matrix3D& m);
    void setMatrix4(const Matrix4D& m);
    void setVectors(const Vector3D& a, const Vector3D& b);
    void applyMatrixPreset(const Matrix3D& m);  // to the target matrix (or M), composed with its value
    void setCameraView(Vector3 position, Vector3 target) { m_camera.setView(position, target); }
    void resetCamera() { m_camera.reset(m_mode); }
    void orbitBy(float dxPixels, float dyPixels, float viewHeight);
    // Center the camera on the selection's handle; false (centered on the origin) when there is none
    bool focusSelection(const view::DrawOptions& options);
    void restartTime(bool play) {
        m_scene.timeline().time = 0.0;
        m_scene.timeline().playing = play;
    }

    // --- Picking and files --------------------------------------------------------------
    // The object whose pick segments (ObjectView::pick) pass closest to a screen point, within `radius` pixels;
    // kNoObject when none. Screen size in pixels, as the camera projects onto it.
    scene::ObjectId pickAt(Vector2 point, int screenWidth, int screenHeight, float radius,
                           const view::DrawOptions& options) const;
    // A scene file's contents: both spaces, the space shown, the camera and the view settings
    void saveTo(SceneData& data) const;
    // Replaces the scene (an undo step); false and `error` when the data can't be read. `note` tells about
    // conversions (old keyframes), empty otherwise.
    bool loadFrom(const SceneData& data, std::string& error, std::string& note);

    // --- Read-only state ------------------------------------------------------------------
    // The matrix the breakdown and steps use: the selected matrix, else the first visible one
    const scene::SceneObject* targetMatrix() const;
    Matrix4D matrix() const;
    bool using4x4() const;
    Vector3D vectorValue(const std::string& name) const;
    Complex complexValue(const std::string& name) const;
    double parameterValue(const std::string& name, AppMode mode) const;
    EigenResult eigen() const;

private:
    std::string convertOldKeyframes(const SceneData& data);

    scene::Scene m_scene;
    AppMode m_mode = AppMode::Space3D;
    ViewSettings m_view;
    CameraController m_camera;
    view::SceneView m_sceneView;
    Grid m_grid{10, 1.0f};    // the line grid: used when the shader grid can't run
    GridShader m_gridShader;  // the endless, adaptive grid
    // Camera per space, so switching spaces keeps each viewpoint
    Vector3 m_savedPosition[kAppModeCount] = {};
    Vector3 m_savedTarget[kAppModeCount] = {};
    bool m_hasSavedView[kAppModeCount] = {};
};

} // namespace linalg
