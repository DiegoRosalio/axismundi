#pragma once

#include "ui/AppMode.hpp"
#include <raylib.h>

namespace linalg {

// First-person camera: mouse look + WASD/Space/Ctrl movement.
// Movement and look are only applied while the cursor is hidden, so the
// keyboard stays free for shortcuts and text boxes in UI mode.
class CameraController {
public:
    CameraController();

    void update(float deltaTime);

    // Move back to the default view for the given space
    void reset(AppMode mode);

    Camera3D& camera() { return m_camera; }
    const Camera3D& camera() const { return m_camera; }

    Vector3 position() const { return m_camera.position; }
    Vector3 target() const { return m_camera.target; }
    void setView(Vector3 position, Vector3 target);

private:
    Camera3D m_camera;
    float m_moveSpeed;
    float m_mouseSensitivity;
};

} // namespace linalg
