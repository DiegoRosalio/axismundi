#pragma once

// A 3D vector of floats in math coordinates (z is up) with +, −, scaling, dot, cross and length.
// The base type of the math layer; no raylib here (rendering/WorldMapping.hpp converts to raylib's axes).

#include <cmath>
#include <ostream>

namespace linalg {

class Vector3D {
public:
    float x, y, z;

    Vector3D() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3D(float x, float y, float z) : x(x), y(y), z(z) {}

    // Vector operations
    Vector3D operator+(const Vector3D& other) const;
    Vector3D operator-(const Vector3D& other) const;
    Vector3D operator*(float scalar) const;
    Vector3D operator/(float scalar) const;

    Vector3D& operator+=(const Vector3D& other);
    Vector3D& operator-=(const Vector3D& other);
    Vector3D& operator*=(float scalar);

    // Dot product
    float dot(const Vector3D& other) const;

    // Cross product
    Vector3D cross(const Vector3D& other) const;

    // Magnitude
    float magnitude() const;
    float magnitudeSquared() const;

    // Normalization
    Vector3D normalized() const;
    void normalize();

    // Static helpers
    static Vector3D zero() { return Vector3D(0, 0, 0); }
    static Vector3D unitX() { return Vector3D(1, 0, 0); }
    static Vector3D unitY() { return Vector3D(0, 1, 0); }
    static Vector3D unitZ() { return Vector3D(0, 0, 1); }

    // Output
    friend std::ostream& operator<<(std::ostream& os, const Vector3D& v);
};

// Scalar multiplication (scalar * vector)
Vector3D operator*(float scalar, const Vector3D& v);

} // namespace linalg
