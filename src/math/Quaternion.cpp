#include "math/Quaternion.hpp"
#include <algorithm>
#include <cmath>

namespace linalg {

Quaternion Quaternion::fromAxisAngle(const Vector3D& axis, float angle) {
    Vector3D a = axis.normalized();
    float s = std::sin(angle / 2.0f);
    return Quaternion{std::cos(angle / 2.0f), a.x * s, a.y * s, a.z * s}.normalized();
}

Quaternion Quaternion::fromMatrix(const Matrix3D& m) {
    // Shepperd's method: pick the largest diagonal term for stability
    const auto& r = m.m;
    float trace = r[0][0] + r[1][1] + r[2][2];
    Quaternion q;
    if (trace > 0.0f) {
        float s = std::sqrt(trace + 1.0f) * 2.0f;
        q.w = 0.25f * s;
        q.x = (r[2][1] - r[1][2]) / s;
        q.y = (r[0][2] - r[2][0]) / s;
        q.z = (r[1][0] - r[0][1]) / s;
    } else if (r[0][0] > r[1][1] && r[0][0] > r[2][2]) {
        float s = std::sqrt(1.0f + r[0][0] - r[1][1] - r[2][2]) * 2.0f;
        q.w = (r[2][1] - r[1][2]) / s;
        q.x = 0.25f * s;
        q.y = (r[0][1] + r[1][0]) / s;
        q.z = (r[0][2] + r[2][0]) / s;
    } else if (r[1][1] > r[2][2]) {
        float s = std::sqrt(1.0f + r[1][1] - r[0][0] - r[2][2]) * 2.0f;
        q.w = (r[0][2] - r[2][0]) / s;
        q.x = (r[0][1] + r[1][0]) / s;
        q.y = 0.25f * s;
        q.z = (r[1][2] + r[2][1]) / s;
    } else {
        float s = std::sqrt(1.0f + r[2][2] - r[0][0] - r[1][1]) * 2.0f;
        q.w = (r[1][0] - r[0][1]) / s;
        q.x = (r[0][2] + r[2][0]) / s;
        q.y = (r[1][2] + r[2][1]) / s;
        q.z = 0.25f * s;
    }
    return q.normalized();
}

Matrix3D Quaternion::toMatrix() const {
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;
    return Matrix3D({{
        {{1 - 2 * (yy + zz), 2 * (xy - wz), 2 * (xz + wy)}},
        {{2 * (xy + wz), 1 - 2 * (xx + zz), 2 * (yz - wx)}},
        {{2 * (xz - wy), 2 * (yz + wx), 1 - 2 * (xx + yy)}},
    }});
}

float Quaternion::angle() const {
    // Use |w| so the angle is in [0, pi] (q and -q are the same rotation)
    return 2.0f * std::acos(std::clamp(std::fabs(w), 0.0f, 1.0f));
}

Vector3D Quaternion::axis() const {
    float s = std::sqrt(x * x + y * y + z * z);
    if (s < 1e-6f) return Vector3D(1, 0, 0);
    Vector3D a(x / s, y / s, z / s);
    return w < 0 ? a * -1.0f : a;
}

Quaternion Quaternion::normalized() const {
    float n = std::sqrt(w * w + x * x + y * y + z * z);
    if (n < 1e-12f) return identity();
    return {w / n, x / n, y / n, z / n};
}

Quaternion Quaternion::slerp(const Quaternion& a, const Quaternion& bIn, float t) {
    Quaternion b = bIn;
    float d = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
    if (d < 0.0f) {  // take the shorter way around
        b = {-b.w, -b.x, -b.y, -b.z};
        d = -d;
    }
    if (d > 0.9995f) {  // nearly identical: linear interpolation is accurate
        return Quaternion{a.w + (b.w - a.w) * t, a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                          a.z + (b.z - a.z) * t}.normalized();
    }
    float theta = std::acos(d);
    float sa = std::sin((1 - t) * theta) / std::sin(theta);
    float sb = std::sin(t * theta) / std::sin(theta);
    return Quaternion{a.w * sa + b.w * sb, a.x * sa + b.x * sb, a.y * sa + b.y * sb, a.z * sa + b.z * sb}
        .normalized();
}

} // namespace linalg
