#pragma once

#include "Vector3D.hpp"
#include <array>
#include <optional>
#include <ostream>

namespace linalg {

// 4x4 Matrix for 3D transformations
class Matrix4D {
public:
    // Row-major storage: m[row][col]
    std::array<std::array<float, 4>, 4> m;

    Matrix4D();
    Matrix4D(const std::array<std::array<float, 4>, 4>& values);

    // Element access
    float& at(int row, int col) { return m[row][col]; }
    const float& at(int row, int col) const { return m[row][col]; }

    // Matrix operations
    Matrix4D operator+(const Matrix4D& other) const;
    Matrix4D operator-(const Matrix4D& other) const;
    Matrix4D operator*(const Matrix4D& other) const;
    Matrix4D operator*(float scalar) const;

    // Properties
    float determinant() const;
    float trace() const;
    Matrix4D transposed() const;

    // Inverse, or std::nullopt when the matrix is singular (|det| < epsilon)
    std::optional<Matrix4D> inverse(float epsilon = 1e-6f) const;

    // Transform a 3D vector (treats as homogeneous with w=1)
    Vector3D transform(const Vector3D& v) const;

    // Transform a direction vector (w=0, no translation)
    Vector3D transformDirection(const Vector3D& v) const;

    // Static factory methods
    static Matrix4D identity();
    static Matrix4D zero();
    static Matrix4D translation(float tx, float ty, float tz);
    static Matrix4D rotationX(float angleRadians);
    static Matrix4D rotationY(float angleRadians);
    static Matrix4D rotationZ(float angleRadians);
    static Matrix4D scale(float sx, float sy, float sz);

    // Output
    friend std::ostream& operator<<(std::ostream& os, const Matrix4D& mat);
};

Matrix4D operator*(float scalar, const Matrix4D& mat);

} // namespace linalg
