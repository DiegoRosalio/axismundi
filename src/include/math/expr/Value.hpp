#pragma once

#include "math/Matrix3D.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"
#include <string>

namespace linalg::expr {

enum class Type { Scalar, Complex, Vector, Matrix, Quaternion, Error };

// Result of evaluating an expression: a number, a complex number, a 3D
// vector, a 3x3 or 4x4 matrix, a quaternion, or an error message. New kinds
// (tensors, functions) can be added here as the math grows.
struct Value {
    Type type = Type::Error;
    double re = 0.0;            // Scalar, or real part of Complex
    double im = 0.0;            // imaginary part of Complex
    Vector3D vector;            // Vector
    Matrix4D matrix;            // Matrix (3x3 is stored in the upper-left block)
    int size = 3;               // Matrix: 3 or 4
    double qi = 0.0, qj = 0.0, qk = 0.0;  // Quaternion: re + qi·i + qj·j + qk·k
    std::string error;          // Error

    static Value scalar(double v);
    static Value complex(double re, double im);
    static Value vec(const Vector3D& v);
    static Value mat3(const Matrix3D& m);
    static Value mat4(const Matrix4D& m);
    static Value quat(double w, double x, double y, double z);
    static Value fail(const std::string& message);

    bool ok() const { return type != Type::Error; }
    bool isScalar() const { return type == Type::Scalar; }
    // A complex number with no imaginary part also counts as a number
    bool isRealNumber() const;
    bool isNumber() const { return type == Type::Scalar || type == Type::Complex; }

    Matrix3D matrix3() const;   // upper-left 3x3 block
    Matrix4D matrix4() const;   // 3x3 embedded in a 4x4 identity

    // "number", "complex number", "vector", "3×3 matrix", "quaternion", "error"
    std::string typeName() const;
    // Short human-readable form: "2.5", "1 + 2i", "(1, 0, 0)", "3×3 matrix", "0.707 + 0.707k"
    std::string summary() const;
};

std::string typeName(Type type);

} // namespace linalg::expr
