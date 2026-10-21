#pragma once

#include "math/OrbitCamera.hpp"
#include "ui/AppMode.hpp"
#include <raylib.h>

namespace linalg {

// How the camera moves (a preference, Settings → Camera)
enum class CameraMode {
    Orbit,  // Blender-like: drag to turn around a pivot, right-drag to pan, wheel to zoom
    Fly,    // Minecraft-like: Tab captures the mouse, mouse look + WASD/Space/Ctrl
};

// Mouse input for the orbit camera, already filtered by App (only drags that started on the
// empty 3D view, the wheel only over it)
struct OrbitInput {
    Vector2 orbitDrag{0, 0};  // pixels this frame
    Vector2 panDrag{0, 0};
    float wheel = 0.0f;
    float viewportHeight = 0.0f;
};

// Keyboard and mouse for the fly camera, read by the front end while the mouse is captured
struct FlyInput {
    bool captured = false;     // the mouse is captured (fly mode is moving)
    Vector2 mouseDelta{0, 0};  // pixels since the last frame
    bool forward = false, back = false, left = false, right = false, up = false, down = false;
    bool fast = false;         // Shift: twice the speed
};

// The 3D view's camera. Orbit mode keeps the eye on a sphere around a pivot (math/OrbitCamera);
// fly mode walks a first-person camera, applied only while the cursor is hidden, so the keyboard
// stays free for shortcuts and text boxes in UI mode.
class CameraController {
public:
    CameraController();

    // Fly movement (only in fly mode while the mouse is captured)
    void update(float deltaTime, const FlyInput& input);
    // Orbit mode: turn, pan and zoom from the filtered mouse input
    void applyOrbit(const OrbitInput& input);

    CameraMode mode() const { return m_mode; }
    // Keeps the viewpoint; entering orbit puts the pivot where the view meets the floor
    void setMode(CameraMode mode);

    // Move back to the default view for the given space (also sets which plane is the floor)
    void reset(AppMode space);
    // The space shown: the complex plane only pans and zooms, the 3D space also turns
    void setSpace(AppMode space) { m_space = space; }

    Camera3D& camera() { return m_camera; }
    const Camera3D& camera() const { return m_camera; }

    Vector3 position() const { return m_camera.position; }
    // In orbit mode the pivot; in fly mode a point one unit ahead
    Vector3 target() const { return m_camera.target; }
    // target is kept as the orbit pivot (its distance matters in orbit mode)
    void setView(Vector3 position, Vector3 target);
    // A target only one unit ahead (fly-mode files and older scene files): look for the floor
    // along the same direction instead, so orbiting turns around something visible
    static Vector3 pivotAlong(Vector3 position, Vector3 target, AppMode space);

    // The point the view is about (the orbit pivot; in fly mode where the view meets the floor) and how far
    // the camera is from it: the grid and the axis numbers pick their spacing from this distance
    Vector3 viewCenter() const;
    float viewDistance() const;

    // Orbit helpers (they also work from fly mode, by switching the view only)
    void focus(Vector3 worldPoint);
    void frontView();
    void rightView();
    void topView();

private:
    void syncFromOrbit();
    void syncOrbitFromFly();
    void applyOrbitView();

    Camera3D m_camera;
    OrbitCamera m_orbit;
    CameraMode m_mode = CameraMode::Orbit;
    AppMode m_space = AppMode::Space3D;
    float m_moveSpeed;
    float m_mouseSensitivity;
};

} // namespace linalg
