#include "math/Vector3D.hpp"

namespace linalg {

Vector3D Vector3D::operator+(const Vector3D& other) const {
    return Vector3D(x + other.x, y + other.y, z + other.z);
}

Vector3D Vector3D::operator-(const Vector3D& other) const {
    return Vector3D(x - other.x, y - other.y, z - other.z);
}

Vector3D Vector3D::operator*(float scalar) const {
    return Vector3D(x * scalar, y * scalar, z * scalar);
}

Vector3D Vector3D::operator/(float scalar) const {
    return Vector3D(x / scalar, y / scalar, z / scalar);
}

Vector3D& Vector3D::operator+=(const Vector3D& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

Vector3D& Vector3D::operator-=(const Vector3D& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

Vector3D& Vector3D::operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

float Vector3D::dot(const Vector3D& other) const {
    return x * other.x + y * other.y + z * other.z;
}

Vector3D Vector3D::cross(const Vector3D& other) const {
    return Vector3D(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    );
}

float Vector3D::magnitude() const {
    return std::sqrt(x * x + y * y + z * z);
}

float Vector3D::magnitudeSquared() const {
    return x * x + y * y + z * z;
}

Vector3D Vector3D::normalized() const {
    float mag = magnitude();
    if (mag > 0.0f) {
        return *this / mag;
    }
    return *this;
}

void Vector3D::normalize() {
    float mag = magnitude();
    if (mag > 0.0f) {
        x /= mag;
        y /= mag;
        z /= mag;
    }
}

std::ostream& operator<<(std::ostream& os, const Vector3D& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

Vector3D operator*(float scalar, const Vector3D& v) {
    return v * scalar;
}

} // namespace linalg
