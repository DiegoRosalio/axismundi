#pragma once

#include "math/Matrix3D.hpp"
#include "math/Vector3D.hpp"

namespace linalg {

// Unit quaternion for 3D rotations (w + xi + yj + zk).
struct Quaternion {
    float w = 1.0f, x = 0.0f, y = 0.0f, z = 0.0f;

    static Quaternion identity() { return {}; }
    static Quaternion fromAxisAngle(const Vector3D& axis, float angle);
    // Rotation matrix (orthonormal, det +1) to quaternion
    static Quaternion fromMatrix(const Matrix3D& m);

    Matrix3D toMatrix() const;
    float angle() const;     // rotation angle in [0, pi]
    Vector3D axis() const;   // unit axis (x-axis when the angle is ~0)
    Quaternion normalized() const;

    // Shortest-arc spherical interpolation, t in [0, 1]
    static Quaternion slerp(const Quaternion& a, const Quaternion& b, float t);
};

} // namespace linalg
