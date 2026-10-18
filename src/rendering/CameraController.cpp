#include "rendering/CameraController.hpp"
#include <algorithm>
#include <cmath>
#include <raymath.h>

namespace linalg {

namespace {
Vector3D toD(Vector3 v) { return {v.x, v.y, v.z}; }
Vector3 toV(const Vector3D& v) { return {v.x, v.y, v.z}; }
}  // namespace

CameraController::CameraController()
    : m_camera{}, m_moveSpeed(5.0f), m_mouseSensitivity(0.003f) {
    m_camera.up = Vector3{0.0f, 1.0f, 0.0f};
    m_camera.fovy = 60.0f;
    m_camera.projection = CAMERA_PERSPECTIVE;
    reset(AppMode::Space3D);
}

void CameraController::reset(AppMode space) {
    m_space = space;
    switch (space) {
        case AppMode::Space3D:
            // Looking at the origin from the front-right, a little above (z is up)
            setView({6.5f, 5.0f, 7.5f}, {0.0f, 0.5f, 0.0f});
            break;
        case AppMode::Complex:
            // Face the complex plane (world XY) straight on
            setView({0.5f, 0.5f, 7.0f}, {0.5f, 0.5f, 0.0f});
            break;
    }
}

void CameraController::setView(Vector3 position, Vector3 target) {
    if (Vector3Distance(position, target) < 1e-4f) target = Vector3Add(position, {0.0f, 0.0f, -1.0f});
    m_orbit.fromView(toD(position), toD(target));
    if (m_mode == CameraMode::Orbit) {
        syncFromOrbit();
    } else {
        // Fly keeps the target one unit ahead, as update() expects
        m_camera.position = position;
        m_camera.target = Vector3Add(position, Vector3Normalize(Vector3Subtract(target, position)));
    }
}

Vector3 CameraController::pivotAlong(Vector3 position, Vector3 target, AppMode space) {
    Vector3 dir = Vector3Subtract(target, position);
    float len = Vector3Length(dir);
    if (len < 1e-4f) return target;
    dir = Vector3Scale(dir, 1.0f / len);
    // The floor: world y = 0 in the 3D space (math z = 0), world z = 0 for the complex plane
    float height = space == AppMode::Complex ? position.z : position.y;
    float down = space == AppMode::Complex ? -dir.z : -dir.y;
    if (down > 1e-3f) {
        float s = height / down;
        if (s > 0.5f && s < 50.0f) return Vector3Add(position, Vector3Scale(dir, s));
    }
    return Vector3Add(position, Vector3Scale(dir, 8.0f));
}

Vector3 CameraController::viewCenter() const {
    if (m_mode == CameraMode::Orbit) return m_camera.target;
    return pivotAlong(m_camera.position, m_camera.target, m_space);
}

float CameraController::viewDistance() const {
    return std::max(0.3f, Vector3Distance(m_camera.position, viewCenter()));
}

void CameraController::setMode(CameraMode mode) {
    if (mode == m_mode) return;
    Vector3 p = m_camera.position, t = m_camera.target;
    m_mode = mode;
    // From fly the target is one unit ahead: orbit around the floor point we look at instead
    setView(p, mode == CameraMode::Orbit ? pivotAlong(p, t, m_space) : t);
}

void CameraController::syncOrbitFromFly() {
    // Fly moves the raylib camera directly; pick up where it is before an orbit-style change
    if (m_mode == CameraMode::Fly)
        m_orbit.fromView(toD(m_camera.position), toD(pivotAlong(m_camera.position, m_camera.target, m_space)));
}

void CameraController::applyOrbitView() {
    if (m_mode == CameraMode::Orbit) syncFromOrbit();
    else setView(toV(m_orbit.position()), toV(m_orbit.pivot()));
}

void CameraController::syncFromOrbit() {
    m_camera.position = toV(m_orbit.position());
    m_camera.target = toV(m_orbit.pivot());
}

void CameraController::applyOrbit(const OrbitInput& in) {
    if (m_mode != CameraMode::Orbit) return;
    bool changed = false;
    if ((in.orbitDrag.x != 0.0f || in.orbitDrag.y != 0.0f) && m_space == AppMode::Space3D) {
        // About 180° across a 900 px drag, like Blender's default
        const float perPixel = 0.0055f;
        m_orbit.orbit(-in.orbitDrag.x * perPixel, in.orbitDrag.y * perPixel);
        changed = true;
    }
    // The complex plane stays face-on: every drag pans
    Vector2 pan = in.panDrag;
    if (m_space == AppMode::Complex) pan = Vector2Add(pan, in.orbitDrag);
    if (pan.x != 0.0f || pan.y != 0.0f) {
        m_orbit.pan(pan.x, pan.y, in.viewportHeight, m_camera.fovy);
        changed = true;
    }
    if (in.wheel != 0.0f) {
        m_orbit.zoom(in.wheel);
        changed = true;
    }
    if (changed) syncFromOrbit();
}

void CameraController::focus(Vector3 worldPoint) {
    m_orbit.setPivot(toD(worldPoint));
    if (m_mode == CameraMode::Orbit) {
        syncFromOrbit();
    } else {
        // Fly: turn to look at the point from where we are
        setView(m_camera.position, worldPoint);
    }
}

void CameraController::frontView() {
    syncOrbitFromFly();
    m_orbit.frontView();
    applyOrbitView();
}

void CameraController::rightView() {
    syncOrbitFromFly();
    m_orbit.rightView();
    applyOrbitView();
}

void CameraController::topView() {
    syncOrbitFromFly();
    m_orbit.topView();
    applyOrbitView();
}

void CameraController::update(float deltaTime) {
    if (m_mode != CameraMode::Fly || !IsCursorHidden()) return;

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
