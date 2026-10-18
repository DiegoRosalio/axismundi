#pragma once

#include "math/Vector3D.hpp"

namespace linalg {

// An orbit camera (no raylib): the eye sits on a sphere around a pivot, like Blender's view.
// Works in the renderer's world axes (y up): yaw turns around the up axis, pitch is the
// elevation above the horizon. Yaw 0, pitch 0 looks along −z (the eye is on +z).
class OrbitCamera {
public:
    static constexpr float kMaxPitch = 1.5533f;  // 89°: straight up/down would lose the left-right axis
    static constexpr float kMinDistance = 0.3f;
    static constexpr float kMaxDistance = 300.0f;

    void fromView(const Vector3D& position, const Vector3D& target);
    Vector3D position() const;
    const Vector3D& pivot() const { return m_pivot; }
    float distance() const { return m_distance; }
    float yaw() const { return m_yaw; }
    float pitch() const { return m_pitch; }

    // Turn around the pivot (radians); the pitch stays within ±kMaxPitch
    void orbit(float dyaw, float dpitch);
    // Drag the view by a mouse move in pixels: the point at the pivot's depth follows the cursor
    void pan(float dxPixels, float dyPixels, float viewportHeight, float fovyDegrees);
    // Wheel steps: positive moves closer (distance × 0.85 per step)
    void zoom(float steps);
    void setPivot(const Vector3D& pivot) { m_pivot = pivot; }
    void setDistance(float d);
    // Blender's numpad views, keeping the pivot and distance
    void frontView() { m_yaw = 0.0f, m_pitch = 0.0f; }
    void rightView() { m_yaw = 1.5707963f, m_pitch = 0.0f; }
    void topView() { m_yaw = 0.0f, m_pitch = kMaxPitch; }  // x to the right, y up the screen

    // Unit vectors of the eye: looking direction, screen right, screen up
    Vector3D forward() const;
    Vector3D right() const;
    Vector3D up() const;

private:
    Vector3D m_pivot{0, 0, 0};
    float m_distance = 10.0f;
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
};

} // namespace linalg
