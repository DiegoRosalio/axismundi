#include "math/Decompositions.hpp"
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace linalg {

namespace {

using Mat = std::array<std::array<double, 3>, 3>;
using Vec = std::array<double, 3>;

constexpr double kPi = 3.14159265358979323846;

Mat toMat(const Matrix3D& a) {
    Mat r{};
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            r[i][j] = a.m[i][j];
    return r;
}

Matrix3D fromMat(const Mat& a) {
    Matrix3D r;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            r.m[i][j] = static_cast<float>(a[i][j]);
    return r;
}

double dot(const Vec& a, const Vec& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
double norm(const Vec& a) { return std::sqrt(dot(a, a)); }

Vec cross(const Vec& a, const Vec& b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

Vec scaled(const Vec& a, double s) { return {a[0] * s, a[1] * s, a[2] * s}; }
Vec sub(const Vec& a, const Vec& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }

// Any unit vector perpendicular to the unit vector u
Vec perpendicular(const Vec& u) {
    // Cross with the axis least aligned with u
    Vec axis{0, 0, 0};
    int k = 0;
    for (int i = 1; i < 3; i++)
        if (std::fabs(u[i]) < std::fabs(u[k])) k = i;
    axis[k] = 1.0;
    Vec p = cross(u, axis);
    return scaled(p, 1.0 / norm(p));
}

double maxAbs(const Mat& a) {
    double m = 0.0;
    for (const auto& row : a)
        for (double v : row) m = std::max(m, std::fabs(v));
    return m;
}

// Orthonormal basis of the null space of m (rows treated as vectors).
std::vector<Vec> nullSpace(const Mat& m, double tol) {
    std::vector<Vec> rows(m.begin(), m.end());

    // Pick the largest row as the first direction of the row space
    auto largest = [](const std::vector<Vec>& rs) {
        size_t best = 0;
        for (size_t i = 1; i < rs.size(); i++)
            if (norm(rs[i]) > norm(rs[best])) best = i;
        return best;
    };

    size_t i1 = largest(rows);
    double n1 = norm(rows[i1]);
    if (n1 < tol) {
        return {Vec{1, 0, 0}, Vec{0, 1, 0}, Vec{0, 0, 1}};
    }
    Vec u1 = scaled(rows[i1], 1.0 / n1);

    // Remove the u1 component from the other rows
    std::vector<Vec> rest;
    for (size_t i = 0; i < rows.size(); i++) {
        if (i == i1) continue;
        rest.push_back(sub(rows[i], scaled(u1, dot(rows[i], u1))));
    }
    size_t i2 = largest(rest);
    double n2 = norm(rest[i2]);
    if (n2 < tol) {
        Vec p = perpendicular(u1);
        return {p, cross(u1, p)};
    }
    Vec u2 = scaled(rest[i2], 1.0 / n2);
    Vec n = cross(u1, u2);
    return {scaled(n, 1.0 / norm(n))};
}

// Jacobi eigenvalue algorithm for a symmetric matrix.
// Returns eigenvalues (descending) and eigenvectors as the columns of vecs.
void jacobiSymmetric(Mat a, Vec& values, Mat& vecs) {
    vecs = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    for (int sweep = 0; sweep < 50; sweep++) {
        double off = a[0][1] * a[0][1] + a[0][2] * a[0][2] + a[1][2] * a[1][2];
        if (off < 1e-24) break;
        for (int p = 0; p < 2; p++) {
            for (int q = p + 1; q < 3; q++) {
                if (std::fabs(a[p][q]) < 1e-300) continue;
                double theta = (a[q][q] - a[p][p]) / (2.0 * a[p][q]);
                double t = (theta >= 0 ? 1.0 : -1.0) /
                           (std::fabs(theta) + std::sqrt(theta * theta + 1.0));
                double c = 1.0 / std::sqrt(t * t + 1.0);
                double s = t * c;
                for (int k = 0; k < 3; k++) {
                    double akp = a[k][p], akq = a[k][q];
                    a[k][p] = c * akp - s * akq;
                    a[k][q] = s * akp + c * akq;
                }
                for (int k = 0; k < 3; k++) {
                    double apk = a[p][k], aqk = a[q][k];
                    a[p][k] = c * apk - s * aqk;
                    a[q][k] = s * apk + c * aqk;
                }
                for (int k = 0; k < 3; k++) {
                    double vkp = vecs[k][p], vkq = vecs[k][q];
                    vecs[k][p] = c * vkp - s * vkq;
                    vecs[k][q] = s * vkp + c * vkq;
                }
            }
        }
    }

    // Sort by eigenvalue, descending
    std::array<int, 3> order{0, 1, 2};
    std::sort(order.begin(), order.end(), [&](int x, int y) { return a[x][x] > a[y][y]; });
    Mat sorted{};
    for (int c = 0; c < 3; c++) {
        values[c] = a[order[c]][order[c]];
        for (int r = 0; r < 3; r++) sorted[r][c] = vecs[r][order[c]];
    }
    vecs = sorted;
}

Vec column(const Mat& m, int c) { return {m[0][c], m[1][c], m[2][c]}; }

void setColumn(Mat& m, int c, const Vec& v) {
    for (int r = 0; r < 3; r++) m[r][c] = v[r];
}

} // namespace

LUResult luDecompose(const Matrix3D& input) {
    Mat u = toMat(input);
    Mat l{};
    std::array<int, 3> perm{0, 1, 2};
    LUResult result;
    double tol = 1e-9 * std::max(1.0, maxAbs(u));

    for (int col = 0; col < 3; col++) {
        int pivot = col;
        for (int r = col + 1; r < 3; r++)
            if (std::fabs(u[r][col]) > std::fabs(u[pivot][col])) pivot = r;
        if (pivot != col) {
            std::swap(u[col], u[pivot]);
            std::swap(l[col], l[pivot]);
            std::swap(perm[col], perm[pivot]);
        }
        if (std::fabs(u[col][col]) < tol) {
            result.singular = true;
            continue;  // Nothing to eliminate below a zero pivot
        }
        for (int r = col + 1; r < 3; r++) {
            double f = u[r][col] / u[col][col];
            l[r][col] = f;
            for (int j = col; j < 3; j++) u[r][j] -= f * u[col][j];
            u[r][col] = 0.0;
        }
    }

    Mat p{};
    for (int i = 0; i < 3; i++) {
        l[i][i] = 1.0;
        p[i][perm[i]] = 1.0;
    }
    result.P = fromMat(p);
    result.L = fromMat(l);
    result.U = fromMat(u);
    return result;
}

QRResult qrDecompose(const Matrix3D& input) {
    Mat a = toMat(input);
    Mat q{}, r{};
    double tol = 1e-9 * std::max(1.0, maxAbs(a));

    // Modified Gram-Schmidt on the columns
    for (int j = 0; j < 3; j++) {
        Vec v = column(a, j);
        for (int i = 0; i < j; i++) {
            Vec qi = column(q, i);
            r[i][j] = dot(qi, v);
            v = sub(v, scaled(qi, r[i][j]));
        }
        double n = norm(v);
        if (n < tol) {
            // Dependent column: keep Q orthogonal by choosing a fresh direction
            r[j][j] = 0.0;
            if (j == 0) {
                v = {1, 0, 0};
            } else if (j == 1) {
                v = perpendicular(column(q, 0));
            } else {
                v = cross(column(q, 0), column(q, 1));
            }
            n = norm(v);
        } else {
            r[j][j] = n;
        }
        setColumn(q, j, scaled(v, 1.0 / n));
    }
    return {fromMat(q), fromMat(r)};
}

EigenResult eigen3(const Matrix3D& input) {
    Mat a = toMat(input);
    double scale = std::max(1.0, maxAbs(a));

    // Characteristic polynomial: λ³ + c2 λ² + c1 λ + c0 = 0
    double tr = a[0][0] + a[1][1] + a[2][2];
    double minors = (a[0][0] * a[1][1] - a[0][1] * a[1][0]) +
                    (a[0][0] * a[2][2] - a[0][2] * a[2][0]) +
                    (a[1][1] * a[2][2] - a[1][2] * a[2][1]);
    double det = input.determinant();
    double c2 = -tr, c1 = minors, c0 = -det;

    // Depressed cubic t³ + p t + q = 0 with λ = t - c2/3
    double shift = -c2 / 3.0;
    double p = c1 - c2 * c2 / 3.0;
    double q = 2.0 * c2 * c2 * c2 / 27.0 - c2 * c1 / 3.0 + c0;
    double disc = (q / 2.0) * (q / 2.0) + (p / 3.0) * (p / 3.0) * (p / 3.0);

    struct Root { double re, im; };
    std::vector<Root> roots;
    double eps = 1e-9 * scale * scale * scale;

    if (disc > eps * eps) {
        // One real root and a complex-conjugate pair
        double sq = std::sqrt(disc);
        double u = std::cbrt(-q / 2.0 + sq);
        double v = std::cbrt(-q / 2.0 - sq);
        double im = std::sqrt(3.0) / 2.0 * (u - v);
        // A tiny imaginary part is rounding noise from a repeated real root
        if (std::fabs(im) < 1e-6 * scale) im = 0.0;
        roots.push_back({u + v + shift, 0.0});
        roots.push_back({-(u + v) / 2.0 + shift, std::fabs(im)});
        roots.push_back({-(u + v) / 2.0 + shift, -std::fabs(im)});
    } else if (std::fabs(p) < 1e-12 * scale * scale) {
        // Triple root
        double t = std::cbrt(-q);
        for (int i = 0; i < 3; i++) roots.push_back({t + shift, 0.0});
    } else {
        // Three real roots (trigonometric method)
        double r = 2.0 * std::sqrt(-p / 3.0);
        double arg = 3.0 * q / (2.0 * p) * std::sqrt(-3.0 / p);
        arg = std::clamp(arg, -1.0, 1.0);
        double phi = std::acos(arg) / 3.0;
        for (int k = 0; k < 3; k++)
            roots.push_back({r * std::cos(phi - 2.0 * kPi * k / 3.0) + shift, 0.0});
    }

    // Real roots first (descending), then the complex pair
    std::sort(roots.begin(), roots.end(), [](const Root& x, const Root& y) {
        bool xr = x.im == 0.0, yr = y.im == 0.0;
        if (xr != yr) return xr;
        if (x.re != y.re) return x.re > y.re;
        return x.im > y.im;
    });

    EigenResult result;
    double tol = 1e-4 * scale;
    for (size_t i = 0; i < 3; i++) {
        Eigenvalue& ev = result.values[i];
        ev.re = static_cast<float>(roots[i].re);
        ev.im = static_cast<float>(roots[i].im);
        if (roots[i].im != 0.0) continue;

        // Index of this root among equal roots seen so far (for repeated roots)
        int rank = 0;
        for (size_t j = 0; j < i; j++)
            if (roots[j].im == 0.0 && std::fabs(roots[j].re - roots[i].re) < tol) rank++;

        Mat m = a;
        for (int d = 0; d < 3; d++) m[d][d] -= roots[i].re;
        std::vector<Vec> ns = nullSpace(m, tol);
        if (rank < static_cast<int>(ns.size())) {
            const Vec& v = ns[rank];
            ev.hasVector = true;
            ev.vector = Vector3D(static_cast<float>(v[0]), static_cast<float>(v[1]),
                                 static_cast<float>(v[2]));
        }
    }
    return result;
}

SVDResult svd3(const Matrix3D& input) {
    Mat a = toMat(input);

    // AᵀA = V Σ² Vᵀ
    Mat ata{};
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            for (int k = 0; k < 3; k++) ata[i][j] += a[k][i] * a[k][j];

    Vec values{};
    Mat v{};
    jacobiSymmetric(ata, values, v);

    Vec sigma{};
    for (int i = 0; i < 3; i++) sigma[i] = std::sqrt(std::max(0.0, values[i]));

    // U columns: A vᵢ / σᵢ, completed to an orthonormal basis where σᵢ ≈ 0
    double tol = 1e-6 * std::max(1.0, sigma[0]);
    Mat u{};
    int filled = 0;
    for (int i = 0; i < 3; i++) {
        if (sigma[i] <= tol) break;
        Vec vi = column(v, i);
        Vec ui{0, 0, 0};
        for (int r = 0; r < 3; r++) ui[r] = dot(Vec{a[r][0], a[r][1], a[r][2]}, vi);
        // Re-orthogonalize against previous columns for stability
        for (int k = 0; k < i; k++) ui = sub(ui, scaled(column(u, k), dot(column(u, k), ui)));
        setColumn(u, i, scaled(ui, 1.0 / norm(ui)));
        filled++;
    }
    if (filled == 0) {
        setColumn(u, 0, Vec{1, 0, 0});
        filled = 1;
    }
    if (filled == 1) {
        setColumn(u, 1, perpendicular(column(u, 0)));
        filled = 2;
    }
    if (filled == 2) {
        setColumn(u, 2, cross(column(u, 0), column(u, 1)));
    }

    Mat s{};
    for (int i = 0; i < 3; i++) s[i][i] = (sigma[i] <= tol) ? 0.0 : sigma[i];

    return {fromMat(u), fromMat(s), fromMat(v)};
}

} // namespace linalg
