#pragma once

#include "Vector3D.hpp"
#include <array>
#include <optional>
#include <ostream>

namespace linalg {

// 3x3 Matrix for 2D transformations in 3D space
class Matrix3D {
public:
    // Row-major storage: m[row][col]
    std::array<std::array<float, 3>, 3> m;

    Matrix3D();
    Matrix3D(const std::array<std::array<float, 3>, 3>& values);

    // Element access
    float& at(int row, int col) { return m[row][col]; }
    const float& at(int row, int col) const { return m[row][col]; }

    // Matrix operations
    Matrix3D operator+(const Matrix3D& other) const;
    Matrix3D operator-(const Matrix3D& other) const;
    Matrix3D operator*(const Matrix3D& other) const;
    Matrix3D operator*(float scalar) const;

    // Properties
    float determinant() const;
    float trace() const;
    Matrix3D transposed() const;

    // Inverse, or std::nullopt when the matrix is singular (|det| < epsilon)
    std::optional<Matrix3D> inverse(float epsilon = 1e-6f) const;

    // Transform a 2D vector (treats as homogeneous with w=1)
    Vector3D transform(const Vector3D& v) const;

    // Static factory methods
    static Matrix3D identity();
    static Matrix3D zero();
    static Matrix3D rotation(float angleRadians);  // 2D rotation
    static Matrix3D scale(float sx, float sy);
    static Matrix3D shear(float shx, float shy);

    // Output
    friend std::ostream& operator<<(std::ostream& os, const Matrix3D& mat);
};

Matrix3D operator*(float scalar, const Matrix3D& mat);

} // namespace linalg
