#include "rendering/CameraController.hpp"
#include <raymath.h>

namespace linalg {

CameraController::CameraController()
    : m_camera{}, m_moveSpeed(5.0f), m_mouseSensitivity(0.003f) {
    m_camera.up = Vector3{0.0f, 1.0f, 0.0f};
    m_camera.fovy = 60.0f;
    m_camera.projection = CAMERA_PERSPECTIVE;
    reset(AppMode::LinearAlgebra);
}

void CameraController::reset(AppMode mode) {
    switch (mode) {
        case AppMode::LinearAlgebra:
            setView({5.0f, 3.0f, 5.0f}, {0.0f, 0.0f, 0.0f});
            break;
        case AppMode::Complex:
            // Face the Argand plane (world XY) straight on
            setView({0.5f, 0.5f, 7.0f}, {0.5f, 0.5f, 0.0f});
            break;
        case AppMode::Plotting:
            setView({7.0f, 6.0f, 9.0f}, {0.0f, 0.0f, 0.0f});
            break;
    }
}

void CameraController::setView(Vector3 position, Vector3 target) {
    m_camera.position = position;
    // Keep the target one unit ahead, as update() expects
    Vector3 dir = Vector3Subtract(target, position);
    if (Vector3Length(dir) < 1e-4f) dir = {0.0f, 0.0f, -1.0f};
    m_camera.target = Vector3Add(position, Vector3Normalize(dir));
}

void CameraController::update(float deltaTime) {
    if (!IsCursorHidden()) return;

    Vector3 forward = Vector3Normalize(Vector3Subtract(m_camera.target, m_camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, m_camera.up));

    // Mouse look
    Vector2 mouseDelta = GetMouseDelta();

    float yaw = -mouseDelta.x * m_mouseSensitivity;
    forward = Vector3RotateByAxisAngle(forward, m_camera.up, yaw);
    right = Vector3RotateByAxisAngle(right, m_camera.up, yaw);

    float pitch = -mouseDelta.y * m_mouseSensitivity;
    Vector3 pitched = Vector3RotateByAxisAngle(forward, right, pitch);

    // Limit pitch to avoid flipping (~5 to 175 degrees from up)
    float angle = Vector3Angle(pitched, m_camera.up);
    if (angle > 0.1f && angle < 3.04f) {
        forward = pitched;
    }

    // WASD movement, Shift to sprint, Space/Ctrl for up/down
    float speed = m_moveSpeed * deltaTime;
    if (IsKeyDown(KEY_LEFT_SHIFT)) speed *= 2.0f;

    Vector3& pos = m_camera.position;
    if (IsKeyDown(KEY_W)) pos = Vector3Add(pos, Vector3Scale(forward, speed));
    if (IsKeyDown(KEY_S)) pos = Vector3Subtract(pos, Vector3Scale(forward, speed));
    if (IsKeyDown(KEY_A)) pos = Vector3Subtract(pos, Vector3Scale(right, speed));
    if (IsKeyDown(KEY_D)) pos = Vector3Add(pos, Vector3Scale(right, speed));
    if (IsKeyDown(KEY_SPACE)) pos.y += speed;
    if (IsKeyDown(KEY_LEFT_CONTROL)) pos.y -= speed;

    m_camera.target = Vector3Add(pos, forward);
}

} // namespace linalg
