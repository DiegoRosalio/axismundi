#include "math/expr/Value.hpp"
#include <cmath>
#include <cstdio>

namespace linalg::expr {

namespace {

std::string num(double v) {
    if (std::fabs(v) < 5e-5) v = 0.0;
    char buf[32];
    if (std::fabs(v - std::round(v)) < 5e-5) snprintf(buf, sizeof(buf), "%.0f", v);
    else snprintf(buf, sizeof(buf), "%.3g", v);
    return buf;
}

} // namespace

Value Value::scalar(double v) {
    Value r;
    r.type = Type::Scalar;
    r.re = v;
    return r;
}

Value Value::complex(double re, double im) {
    Value r;
    r.type = Type::Complex;
    r.re = re;
    r.im = im;
    return r;
}

Value Value::vec(const Vector3D& v) {
    Value r;
    r.type = Type::Vector;
    r.vector = v;
    return r;
}

Value Value::mat3(const Matrix3D& m) {
    Value r;
    r.type = Type::Matrix;
    r.size = 3;
    r.matrix = Matrix4D::identity();
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) r.matrix.m[i][j] = m.m[i][j];
    return r;
}

Value Value::mat4(const Matrix4D& m) {
    Value r;
    r.type = Type::Matrix;
    r.size = 4;
    r.matrix = m;
    return r;
}

Value Value::quat(double w, double x, double y, double z) {
    Value r;
    r.type = Type::Quaternion;
    r.re = w;
    r.qi = x;
    r.qj = y;
    r.qk = z;
    return r;
}

Value Value::fail(const std::string& message) {
    Value r;
    r.type = Type::Error;
    r.error = message;
    return r;
}

bool Value::isRealNumber() const {
    return type == Type::Scalar || (type == Type::Complex && std::fabs(im) < 1e-12);
}

Matrix3D Value::matrix3() const {
    Matrix3D m;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) m.m[i][j] = matrix.m[i][j];
    return m;
}

Matrix4D Value::matrix4() const {
    if (size == 4) return matrix;
    Matrix4D m = Matrix4D::identity();
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) m.m[i][j] = matrix.m[i][j];
    return m;
}

std::string typeName(Type type) {
    switch (type) {
        case Type::Scalar:  return "number";
        case Type::Complex: return "complex number";
        case Type::Vector:  return "vector";
        case Type::Matrix:  return "matrix";
        case Type::Quaternion: return "quaternion";
        case Type::Error:   return "error";
    }
    return "?";
}

std::string Value::typeName() const {
    if (type == Type::Matrix) return size == 4 ? "4×4 matrix" : "3×3 matrix";
    return expr::typeName(type);
}

std::string Value::summary() const {
    switch (type) {
        case Type::Scalar:
            return num(re);
        case Type::Complex: {
            if (std::fabs(im) < 5e-5) return num(re);
            std::string r = std::fabs(re) < 5e-5 ? "" : num(re) + (im < 0 ? " − " : " + ");
            std::string i = num(std::fabs(im));
            if (r.empty() && im < 0) r = "−";
            return r + (i == "1" ? "" : i) + "i";
        }
        case Type::Vector:
            return "(" + num(vector.x) + ", " + num(vector.y) + ", " + num(vector.z) + ")";
        case Type::Matrix:
            return typeName();
        case Type::Quaternion: {
            // w + xi + yj + zk, leaving out zero terms
            std::string out;
            const double parts[4] = {re, qi, qj, qk};
            const char* units[4] = {"", "i", "j", "k"};
            for (int n = 0; n < 4; n++) {
                double v = parts[n];
                if (std::fabs(v) < 5e-5) continue;
                std::string mag = num(std::fabs(v));
                if (n > 0 && mag == "1") mag = "";
                if (out.empty()) out = (v < 0 ? "−" : "") + mag + units[n];
                else out += (v < 0 ? " − " : " + ") + mag + units[n];
            }
            return out.empty() ? "0" : out;
        }
        case Type::Error:
            return error;
    }
    return "";
}

} // namespace linalg::expr
