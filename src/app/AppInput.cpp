// App: Keyboard and mouse: camera/UI mode, shortcuts, picking objects in the view (the rest of App is in App.cpp)
#include "app/App.hpp"
#include "view/ViewStyle.hpp"
#include "view/ObjectViews.hpp"
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

namespace {

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
bool App::isEditing() const {
    return m_scenePanel.isEditing() || m_referencePanel.isEditing() || m_fileDialog.isOpen() ||
           m_formulaEditor.isOpen();
}

void App::handleInput(float deltaTime) {
    bool editing = isEditing();

    // Escape steps back one level: cancel an edit, close a popup or the help,
    // or leave camera mode (frees the mouse). It never quits the app.
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (m_formulaEditor.isOpen()) {
            m_formulaEditor.close();
            editing = false;
        } else if (m_fileDialog.isOpen()) {
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

    syncCameraMode();
    // TAB switches between camera mode (cursor captured) and UI mode, with the fly camera.
    // While a field is being edited, TAB moves to the next field instead.
    if (!editing && IsKeyPressed(KEY_TAB)) {
        if (m_camera.mode() == CameraMode::Fly) setUiMode(IsCursorHidden());
        else toast("Drag the view to turn it, right-drag to pan, wheel to zoom. Fly camera: Settings → Camera");
    }

    bool ui = uiMode();
    // Widgets only respond in UI mode, and not behind the help overlay
    if (ui && !m_help.visible()) GuiUnlock();
    else GuiLock();

    m_scenePanel.update(m_scene);

    if (!editing) handleShortcuts(ui);
    if (ui && !m_help.visible()) handleViewMouse();
    else m_dragButton = -1;

    applyModeChange();  // tabs and keys 1/2 change m_mode directly

    m_camera.update(deltaTime);
}

void App::applyModeChange() {
    if (m_mode == m_lastMode) return;
    // A field in the hidden space would otherwise stay "active"
    m_scenePanel.finishEditing(m_scene);
    closePopovers();
    // Remember the camera of the space we leave, restore the one of the space we enter
    int from = static_cast<int>(m_lastMode), to = static_cast<int>(m_mode);
    m_camera.setSpace(m_mode);
    m_savedPosition[from] = m_camera.position();
    m_savedTarget[from] = m_camera.target();
    m_hasSavedView[from] = true;
    if (m_hasSavedView[to]) m_camera.setView(m_savedPosition[to], m_savedTarget[to]);
    else m_camera.reset(m_mode);
    m_lastMode = m_mode;
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
        if (IsKeyPressed(KEY_B)) setSideBarCollapsed(!SideBar::collapsed());
        if (IsKeyPressed(KEY_E)) {
            m_exportWindow.toggle();
            if (m_exportWindow.visible()) m_panels.bringToFront(&m_exportWindow);
        }
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
    if (IsKeyPressed(KEY_O)) toggleSolo();
    if (IsKeyPressed(KEY_M)) m_analysisWindow.toggle();
    if (IsKeyPressed(KEY_K)) m_stepsWindow.toggle();
    if (IsKeyPressed(KEY_B)) m_breakdownWindow.toggle();
    if (IsKeyPressed(KEY_C)) resetCamera();
    if (IsKeyPressed(KEY_PERIOD) || IsKeyPressed(KEY_KP_DECIMAL)) focusSelection();
    if (m_mode == AppMode::Space3D) {
        // Blender's numpad views
        if (IsKeyPressed(KEY_KP_1)) m_camera.frontView();
        if (IsKeyPressed(KEY_KP_3)) m_camera.rightView();
        if (IsKeyPressed(KEY_KP_7)) m_camera.topView();
    }
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

void App::toggleSolo() {
    ViewSettings& view = m_viewPanel.settings();
    if (!view.soloSelection && m_scene.selection(spaceKind()).empty()) {
        toast("Select objects first: O then shows only them", true);
        return;
    }
    view.soloSelection = !view.soloSelection;
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

void App::syncCameraMode() {
    CameraMode want = m_uiSettings.flyCamera ? CameraMode::Fly : CameraMode::Orbit;
    if (m_camera.mode() == want) return;
    m_camera.setMode(want);
    if (want == CameraMode::Orbit && !uiMode()) setUiMode(true);  // orbit never captures the mouse
}

bool App::viewUnderMouse(Vector2 mouse) const {
    // Only the 3D view itself, not the bars, panels or popovers
    if (mouse.x < SideBar::width() || mouse.y < TopBar::height()) return false;
    if (m_panels.anyContains(mouse)) return false;
    return !(m_addMenu.isOpen() || m_colorPopover.isOpen() || m_settingsPopover.isOpen() || m_fileDialog.isOpen() ||
             m_formulaEditor.isOpen());
}

void App::handleViewMouse() {
    m_cameraMoved = false;
    Vector2 mouse = GetMousePosition();
    bool overView = viewUnderMouse(mouse);
    bool orbit = m_camera.mode() == CameraMode::Orbit;
    OrbitInput in;
    in.viewportHeight = static_cast<float>(GetScreenHeight()) - TopBar::height();

    // A press on the view starts a drag; released before moving a few pixels, it was a click
    if (m_dragButton < 0 && overView) {
        for (int b : {MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE}) {
            if (ui::mousePressed(b)) {
                m_dragButton = b;
                m_dragFresh = true;
                m_dragDistance = 0.0f;
                break;
            }
        }
    }
    if (m_dragButton >= 0) {
        Vector2 d = GetMouseDelta();
        // A touchpad tap presses and releases within one frame (the click latch keeps both)
        bool released = ui::mouseReleased(m_dragButton) || !IsMouseButtonDown(m_dragButton);
        if (!m_dragFresh) m_dragDistance += std::fabs(d.x) + std::fabs(d.y);
        bool dragging = m_dragDistance > ui::px(4.0f);
        if (orbit && dragging && !m_dragFresh) {
            bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            if (m_dragButton == MOUSE_BUTTON_RIGHT || shift) in.panDrag = d;
            else in.orbitDrag = d;
        }
        m_dragFresh = false;
        if (released) {
            if (m_dragButton == MOUSE_BUTTON_LEFT && !dragging && !isEditing()) pickAt(mouse);
            m_dragButton = -1;
        }
    }
    if (orbit && overView) in.wheel = GetMouseWheelMove();
    if (in.orbitDrag.x != 0 || in.orbitDrag.y != 0 || in.panDrag.x != 0 || in.panDrag.y != 0 || in.wheel != 0) {
        m_camera.applyOrbit(in);
        m_cameraMoved = true;
    }
}

void App::orbitBy(float dxPixels, float dyPixels) {
    OrbitInput in;
    in.orbitDrag = {dxPixels, dyPixels};
    in.viewportHeight = static_cast<float>(GetScreenHeight()) - TopBar::height();
    m_camera.applyOrbit(in);
}

void App::focusSelection() {
    SpaceKind kind = spaceKind();
    scene::ObjectId id = m_scene.selection(kind).primary();
    const SceneObject* o = id == scene::kNoObject ? nullptr : m_scene.space(kind).find(id);
    Vector3 p{0, 0, 0};
    if (o && view::SceneView::anchorPoint(*o, drawOptions(), p)) {
        m_camera.focus(p);
    } else {
        m_camera.focus(p);
        toast(o ? "This object has no single point to center on: centered on the origin"
                : "Nothing selected: centered on the origin");
    }
}

void App::pickAt(Vector2 mouse) {
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

    // Each type says which segments a click should be near (view/objects/*.cpp)
    std::vector<view::Segment> segments;
    for (const auto& obj : m_scene.space(kind).objects()) {
        const SceneObject& o = *obj;
        if (!view::SceneView::shown(options, o) || !o.value().ok()) continue;
        const view::ObjectView* ov = view::objectView(o.typeId());
        if (!ov || !ov->pick) continue;
        segments.clear();
        ov->pick(o, options, segments);
        for (const auto& [a, b] : segments) consider(a, b, o.id());
    }

    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    scene::Selection& sel = m_scene.selection(kind);
    if (hit != scene::kNoObject) {
        if (ctrl) sel.toggle(hit);
        else sel.set(hit);
        // The inspector is hidden while the side bar is collapsed: say how to get it back (once)
        if (SideBar::collapsed() && !m_sideBarHintShown) {
            m_sideBarHintShown = true;
            toast("Selected. Ctrl+B shows the inspector");
        }
    } else if (!ctrl) {
        sel.clear();
    }
}

} // namespace linalg
