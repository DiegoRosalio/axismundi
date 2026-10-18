#include "math/OrbitCamera.hpp"
#include <algorithm>
#include <cmath>

namespace linalg {

void OrbitCamera::fromView(const Vector3D& position, const Vector3D& target) {
    m_pivot = target;
    Vector3D d = position - target;
    float len = d.magnitude();
    if (!(len > 1e-4f)) {
        d = {0, 0, 1};
        len = 1.0f;
    }
    setDistance(len);
    m_pitch = std::clamp(std::asin(std::clamp(d.y / len, -1.0f, 1.0f)), -kMaxPitch, kMaxPitch);
    m_yaw = std::atan2(d.x, d.z);
}

Vector3D OrbitCamera::position() const {
    float c = std::cos(m_pitch);
    return m_pivot + Vector3D(c * std::sin(m_yaw), std::sin(m_pitch), c * std::cos(m_yaw)) * m_distance;
}

Vector3D OrbitCamera::forward() const { return (m_pivot - position()).normalized(); }

Vector3D OrbitCamera::right() const {
    // Horizontal, so it never degenerates (the pitch never reaches ±90°)
    return Vector3D(std::cos(m_yaw), 0.0f, -std::sin(m_yaw));
}

Vector3D OrbitCamera::up() const { return right().cross(forward()); }

void OrbitCamera::orbit(float dyaw, float dpitch) {
    m_yaw = std::remainder(m_yaw + dyaw, 6.2831853f);
    m_pitch = std::clamp(m_pitch + dpitch, -kMaxPitch, kMaxPitch);
}

void OrbitCamera::pan(float dxPixels, float dyPixels, float viewportHeight, float fovyDegrees) {
    if (!(viewportHeight > 0.0f)) return;
    float worldPerPixel = 2.0f * m_distance * std::tan(fovyDegrees * 3.14159265f / 360.0f) / viewportHeight;
    // Dragging right moves the scene right, so the pivot moves left (screen y grows downward)
    m_pivot = m_pivot - right() * (dxPixels * worldPerPixel) + up() * (dyPixels * worldPerPixel);
}

void OrbitCamera::zoom(float steps) { setDistance(m_distance * std::pow(0.85f, steps)); }

void OrbitCamera::setDistance(float d) { m_distance = std::clamp(d, kMinDistance, kMaxDistance); }

} // namespace linalg
