#include "math/Matrix3D.hpp"
#include <cmath>

namespace linalg {

Matrix3D::Matrix3D() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            m[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}

Matrix3D::Matrix3D(const std::array<std::array<float, 3>, 3>& values) : m(values) {}

Matrix3D Matrix3D::operator+(const Matrix3D& other) const {
    Matrix3D result;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result.m[i][j] = m[i][j] + other.m[i][j];
        }
    }
    return result;
}

Matrix3D Matrix3D::operator-(const Matrix3D& other) const {
    Matrix3D result;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result.m[i][j] = m[i][j] - other.m[i][j];
        }
    }
    return result;
}

Matrix3D Matrix3D::operator*(const Matrix3D& other) const {
    Matrix3D result = Matrix3D::zero();
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 3; k++) {
                result.m[i][j] += m[i][k] * other.m[k][j];
            }
        }
    }
    return result;
}

Matrix3D Matrix3D::operator*(float scalar) const {
    Matrix3D result;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result.m[i][j] = m[i][j] * scalar;
        }
    }
    return result;
}

float Matrix3D::determinant() const {
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
         - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
         + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

float Matrix3D::trace() const {
    return m[0][0] + m[1][1] + m[2][2];
}

Matrix3D Matrix3D::transposed() const {
    Matrix3D result;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result.m[i][j] = m[j][i];
        }
    }
    return result;
}

std::optional<Matrix3D> Matrix3D::inverse(float epsilon) const {
    float det = determinant();
    if (std::fabs(det) < epsilon) return std::nullopt;

    // Adjugate (transposed cofactor matrix) divided by the determinant
    Matrix3D adj;
    adj.m[0][0] =  (m[1][1] * m[2][2] - m[1][2] * m[2][1]);
    adj.m[0][1] = -(m[0][1] * m[2][2] - m[0][2] * m[2][1]);
    adj.m[0][2] =  (m[0][1] * m[1][2] - m[0][2] * m[1][1]);
    adj.m[1][0] = -(m[1][0] * m[2][2] - m[1][2] * m[2][0]);
    adj.m[1][1] =  (m[0][0] * m[2][2] - m[0][2] * m[2][0]);
    adj.m[1][2] = -(m[0][0] * m[1][2] - m[0][2] * m[1][0]);
    adj.m[2][0] =  (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    adj.m[2][1] = -(m[0][0] * m[2][1] - m[0][1] * m[2][0]);
    adj.m[2][2] =  (m[0][0] * m[1][1] - m[0][1] * m[1][0]);
    return adj * (1.0f / det);
}

Vector3D Matrix3D::transform(const Vector3D& v) const {
    return Vector3D(
        m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
        m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
        m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z
    );
}

Matrix3D Matrix3D::identity() {
    return Matrix3D();
}

Matrix3D Matrix3D::zero() {
    Matrix3D result;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result.m[i][j] = 0.0f;
        }
    }
    return result;
}

Matrix3D Matrix3D::rotation(float angleRadians) {
    float c = std::cos(angleRadians);
    float s = std::sin(angleRadians);
    return Matrix3D({{
        {{c, -s, 0}},
        {{s,  c, 0}},
        {{0,  0, 1}}
    }});
}

Matrix3D Matrix3D::scale(float sx, float sy) {
    return Matrix3D({{
        {{sx, 0,  0}},
        {{0,  sy, 0}},
        {{0,  0,  1}}
    }});
}

Matrix3D Matrix3D::shear(float shx, float shy) {
    return Matrix3D({{
        {{1,   shx, 0}},
        {{shy, 1,   0}},
        {{0,   0,   1}}
    }});
}

std::ostream& operator<<(std::ostream& os, const Matrix3D& mat) {
    os << "[";
    for (int i = 0; i < 3; i++) {
        if (i > 0) os << " ";
        os << "[";
        for (int j = 0; j < 3; j++) {
            os << mat.m[i][j];
            if (j < 2) os << ", ";
        }
        os << "]";
        if (i < 2) os << "\n";
    }
    os << "]";
    return os;
}

Matrix3D operator*(float scalar, const Matrix3D& mat) {
    return mat * scalar;
}

} // namespace linalg
