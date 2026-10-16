#pragma once

#include "Matrix3D.hpp"
#include "Vector3D.hpp"
#include <array>

namespace linalg {

// PA = LU (partial pivoting). L is unit lower triangular, U upper triangular.
struct LUResult {
    Matrix3D P;
    Matrix3D L;
    Matrix3D U;
    bool singular = false;
};

// A = QR. Q is orthogonal, R is upper triangular.
struct QRResult {
    Matrix3D Q;
    Matrix3D R;
};

// One eigenvalue (possibly complex). Real eigenvalues usually come with
// an eigenvector; defective repeated roots may not.
struct Eigenvalue {
    float re = 0.0f;
    float im = 0.0f;
    bool hasVector = false;
    Vector3D vector;

    bool isReal() const { return im == 0.0f; }
};

// Eigenvalues sorted with real ones first (descending), then complex pairs.
struct EigenResult {
    std::array<Eigenvalue, 3> values;
};

// A = U * S * V^T. S is diagonal with singular values in descending order.
struct SVDResult {
    Matrix3D U;
    Matrix3D S;
    Matrix3D V;
};

LUResult luDecompose(const Matrix3D& a);
QRResult qrDecompose(const Matrix3D& a);
EigenResult eigen3(const Matrix3D& a);
SVDResult svd3(const Matrix3D& a);

} // namespace linalg
