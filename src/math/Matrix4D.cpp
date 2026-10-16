#include "math/Matrix4D.hpp"
#include <cmath>
#include <utility>

namespace linalg {

Matrix4D::Matrix4D() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}

Matrix4D::Matrix4D(const std::array<std::array<float, 4>, 4>& values) : m(values) {}

Matrix4D Matrix4D::operator+(const Matrix4D& other) const {
    Matrix4D result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m[i][j] = m[i][j] + other.m[i][j];
        }
    }
    return result;
}

Matrix4D Matrix4D::operator-(const Matrix4D& other) const {
    Matrix4D result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m[i][j] = m[i][j] - other.m[i][j];
        }
    }
    return result;
}

Matrix4D Matrix4D::operator*(const Matrix4D& other) const {
    Matrix4D result = Matrix4D::zero();
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.m[i][j] += m[i][k] * other.m[k][j];
            }
        }
    }
    return result;
}

Matrix4D Matrix4D::operator*(float scalar) const {
    Matrix4D result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m[i][j] = m[i][j] * scalar;
        }
    }
    return result;
}

float Matrix4D::determinant() const {
    // Gaussian elimination with partial pivoting (in double precision)
    double a[4][4];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            a[i][j] = m[i][j];

    double det = 1.0;
    for (int col = 0; col < 4; col++) {
        int pivot = col;
        for (int r = col + 1; r < 4; r++) {
            if (std::fabs(a[r][col]) > std::fabs(a[pivot][col])) pivot = r;
        }
        if (a[pivot][col] == 0.0) return 0.0f;
        if (pivot != col) {
            for (int j = 0; j < 4; j++) std::swap(a[col][j], a[pivot][j]);
            det = -det;
        }
        det *= a[col][col];
        for (int r = col + 1; r < 4; r++) {
            double f = a[r][col] / a[col][col];
            for (int j = col; j < 4; j++) a[r][j] -= f * a[col][j];
        }
    }
    return static_cast<float>(det);
}

float Matrix4D::trace() const {
    return m[0][0] + m[1][1] + m[2][2] + m[3][3];
}

Matrix4D Matrix4D::transposed() const {
    Matrix4D result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m[i][j] = m[j][i];
        }
    }
    return result;
}

std::optional<Matrix4D> Matrix4D::inverse(float epsilon) const {
    if (std::fabs(determinant()) < epsilon) return std::nullopt;

    // Gauss-Jordan elimination on [A | I]
    double a[4][8];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            a[i][j] = m[i][j];
            a[i][j + 4] = (i == j) ? 1.0 : 0.0;
        }
    }

    for (int col = 0; col < 4; col++) {
        int pivot = col;
        for (int r = col + 1; r < 4; r++) {
            if (std::fabs(a[r][col]) > std::fabs(a[pivot][col])) pivot = r;
        }
        if (pivot != col) {
            for (int j = 0; j < 8; j++) std::swap(a[col][j], a[pivot][j]);
        }
        double p = a[col][col];
        for (int j = 0; j < 8; j++) a[col][j] /= p;
        for (int r = 0; r < 4; r++) {
            if (r == col) continue;
            double f = a[r][col];
            for (int j = 0; j < 8; j++) a[r][j] -= f * a[col][j];
        }
    }

    Matrix4D result;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            result.m[i][j] = static_cast<float>(a[i][j + 4]);
    return result;
}

Vector3D Matrix4D::transform(const Vector3D& v) const {
    float w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3];
    return Vector3D(
        (m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3]) / w,
        (m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3]) / w,
        (m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3]) / w
    );
}

Vector3D Matrix4D::transformDirection(const Vector3D& v) const {
    return Vector3D(
        m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
        m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
        m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z
    );
}

Matrix4D Matrix4D::identity() {
    return Matrix4D();
}

Matrix4D Matrix4D::zero() {
    Matrix4D result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m[i][j] = 0.0f;
        }
    }
    return result;
}

Matrix4D Matrix4D::translation(float tx, float ty, float tz) {
    return Matrix4D({{
        {{1, 0, 0, tx}},
        {{0, 1, 0, ty}},
        {{0, 0, 1, tz}},
        {{0, 0, 0, 1}}
    }});
}

Matrix4D Matrix4D::rotationX(float angleRadians) {
    float c = std::cos(angleRadians);
    float s = std::sin(angleRadians);
    return Matrix4D({{
        {{1, 0,  0, 0}},
        {{0, c, -s, 0}},
        {{0, s,  c, 0}},
        {{0, 0,  0, 1}}
    }});
}

Matrix4D Matrix4D::rotationY(float angleRadians) {
    float c = std::cos(angleRadians);
    float s = std::sin(angleRadians);
    return Matrix4D({{
        {{ c, 0, s, 0}},
        {{ 0, 1, 0, 0}},
        {{-s, 0, c, 0}},
        {{ 0, 0, 0, 1}}
    }});
}

Matrix4D Matrix4D::rotationZ(float angleRadians) {
    float c = std::cos(angleRadians);
    float s = std::sin(angleRadians);
    return Matrix4D({{
        {{c, -s, 0, 0}},
        {{s,  c, 0, 0}},
        {{0,  0, 1, 0}},
        {{0,  0, 0, 1}}
    }});
}

Matrix4D Matrix4D::scale(float sx, float sy, float sz) {
    return Matrix4D({{
        {{sx, 0,  0,  0}},
        {{0,  sy, 0,  0}},
        {{0,  0,  sz, 0}},
        {{0,  0,  0,  1}}
    }});
}

std::ostream& operator<<(std::ostream& os, const Matrix4D& mat) {
    os << "[";
    for (int i = 0; i < 4; i++) {
        if (i > 0) os << " ";
        os << "[";
        for (int j = 0; j < 4; j++) {
            os << mat.m[i][j];
            if (j < 3) os << ", ";
        }
        os << "]";
        if (i < 3) os << "\n";
    }
    os << "]";
    return os;
}

Matrix4D operator*(float scalar, const Matrix4D& mat) {
    return mat * scalar;
}

} // namespace linalg
