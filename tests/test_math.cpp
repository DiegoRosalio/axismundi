// Unit tests for the math library (no graphics dependencies).
// Build and run with: make test

#include "animation/Animator.hpp"
#include "math/Complex.hpp"
#include "math/Decompositions.hpp"
#include "math/Expression.hpp"
#include "math/Matrix3D.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

using namespace linalg;

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const char* expr, const char* file, int line) {
    g_checks++;
    if (!ok) {
        g_failures++;
        std::printf("  FAIL %s:%d: %s\n", file, line, expr);
    }
}

#define CHECK(cond) check((cond), #cond, __FILE__, __LINE__)
#define CHECK_NEAR(a, b, tol) check(std::fabs((a) - (b)) <= (tol), #a " ~= " #b, __FILE__, __LINE__)

bool near(const Matrix3D& a, const Matrix3D& b, float tol = 1e-4f) {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (std::fabs(a.m[i][j] - b.m[i][j]) > tol) return false;
    return true;
}

bool near(const Matrix4D& a, const Matrix4D& b, float tol = 1e-4f) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (std::fabs(a.m[i][j] - b.m[i][j]) > tol) return false;
    return true;
}

bool isUpperTriangular(const Matrix3D& m, float tol = 1e-5f) {
    return std::fabs(m.m[1][0]) < tol && std::fabs(m.m[2][0]) < tol && std::fabs(m.m[2][1]) < tol;
}

const std::vector<Matrix3D>& sampleMatrices() {
    static const std::vector<Matrix3D> mats = {
        Matrix3D::identity(),
        Matrix3D({{{{2, 1, 0}}, {{1, 3, 1}}, {{0, 1, 4}}}}),       // symmetric
        Matrix3D({{{{0, 2, 1}}, {{1, 0, 3}}, {{4, 1, 0}}}}),       // needs pivoting
        Matrix3D::rotation(0.7f),                                 // complex eigenvalues
        Matrix3D({{{{1, 2, 3}}, {{2, 4, 6}}, {{1, 1, 1}}}}),       // singular (rank 2)
        Matrix3D({{{{1, 2, 3}}, {{2, 4, 6}}, {{3, 6, 9}}}}),       // rank 1
        Matrix3D({{{{2, 0, 0}}, {{0, 2, 0}}, {{0, 0, 3}}}}),       // repeated eigenvalue
        Matrix3D::shear(0.5f, 0.0f),                              // defective
        Matrix3D({{{{-1, 0, 0}}, {{0, 1, 0}}, {{0, 0, 1}}}}),      // reflection
    };
    return mats;
}

void testVector() {
    Vector3D a(1, 2, 3), b(4, 5, 6);
    CHECK_NEAR(a.dot(b), 32.0f, 1e-6f);
    Vector3D c = a.cross(b);
    CHECK_NEAR(c.x, -3.0f, 1e-6f);
    CHECK_NEAR(c.y, 6.0f, 1e-6f);
    CHECK_NEAR(c.z, -3.0f, 1e-6f);
    CHECK_NEAR(Vector3D(3, 4, 0).magnitude(), 5.0f, 1e-6f);
}

void testMatrix3D() {
    Matrix3D a({{{{2, 1, 0}}, {{1, 3, 1}}, {{0, 1, 4}}}});
    CHECK_NEAR(a.determinant(), 18.0f, 1e-4f);
    CHECK_NEAR(a.trace(), 9.0f, 1e-6f);
    CHECK(near(a.transposed().transposed(), a));

    auto inv = a.inverse();
    CHECK(inv.has_value());
    if (inv) CHECK(near(a * *inv, Matrix3D::identity()));

    Matrix3D singular({{{{1, 2, 3}}, {{2, 4, 6}}, {{1, 1, 1}}}});
    CHECK_NEAR(singular.determinant(), 0.0f, 1e-5f);
    CHECK(!singular.inverse().has_value());

    CHECK_NEAR(Matrix3D::rotation(1.1f).determinant(), 1.0f, 1e-5f);
}

void testMatrix4D() {
    Matrix4D t = Matrix4D::translation(1, 2, 3) * Matrix4D::rotationY(0.4f) * Matrix4D::scale(2, 3, 4);
    CHECK_NEAR(t.determinant(), 24.0f, 1e-3f);
    auto inv = t.inverse();
    CHECK(inv.has_value());
    if (inv) CHECK(near(t * *inv, Matrix4D::identity()));
    CHECK_NEAR(Matrix4D::identity().trace(), 4.0f, 1e-6f);

    Matrix4D z = Matrix4D::zero();
    CHECK(!z.inverse().has_value());
    CHECK(near(t.transposed().transposed(), t));
}

void testLU() {
    for (const auto& a : sampleMatrices()) {
        LUResult lu = luDecompose(a);
        CHECK(near(lu.P * a, lu.L * lu.U));
        CHECK(isUpperTriangular(lu.U));
        CHECK_NEAR(lu.L.m[0][0] + lu.L.m[1][1] + lu.L.m[2][2], 3.0f, 1e-6f);
    }
    CHECK(luDecompose(Matrix3D({{{{1, 2, 3}}, {{2, 4, 6}}, {{3, 6, 9}}}})).singular);
    CHECK(!luDecompose(Matrix3D::identity()).singular);
}

void testQR() {
    for (const auto& a : sampleMatrices()) {
        QRResult qr = qrDecompose(a);
        CHECK(near(qr.Q.transposed() * qr.Q, Matrix3D::identity()));
        CHECK(near(qr.Q * qr.R, a));
        CHECK(isUpperTriangular(qr.R));
    }
}

void testSVD() {
    for (const auto& a : sampleMatrices()) {
        SVDResult s = svd3(a);
        CHECK(near(s.U * s.S * s.V.transposed(), a, 1e-3f));
        CHECK(near(s.U.transposed() * s.U, Matrix3D::identity(), 1e-3f));
        CHECK(near(s.V.transposed() * s.V, Matrix3D::identity(), 1e-3f));
        CHECK(s.S.m[0][0] >= s.S.m[1][1] - 1e-5f);
        CHECK(s.S.m[1][1] >= s.S.m[2][2] - 1e-5f);
        CHECK(s.S.m[2][2] >= 0.0f);
    }
}

void testEigen() {
    for (const auto& a : sampleMatrices()) {
        EigenResult e = eigen3(a);
        // Sum of eigenvalues = trace (imaginary parts cancel)
        float sum = 0.0f;
        for (const auto& ev : e.values) sum += ev.re;
        CHECK_NEAR(sum, a.trace(), 1e-3f);
        // A v = λ v for every reported eigenvector
        for (const auto& ev : e.values) {
            if (!ev.hasVector) continue;
            Vector3D av = a.transform(ev.vector);
            Vector3D lv = ev.vector * ev.re;
            CHECK((av - lv).magnitude() < 1e-3f);
            CHECK_NEAR(ev.vector.magnitude(), 1.0f, 1e-4f);
        }
    }

    // Rotation about Z by θ: eigenvalues 1, e^(±iθ)
    EigenResult r = eigen3(Matrix3D::rotation(0.7f));
    CHECK(r.values[0].isReal());
    CHECK_NEAR(r.values[0].re, 1.0f, 1e-4f);
    CHECK_NEAR(r.values[1].re, std::cos(0.7f), 1e-4f);
    CHECK_NEAR(std::fabs(r.values[1].im), std::sin(0.7f), 1e-4f);

    // diag(2, 2, 3): two independent eigenvectors for λ = 2
    EigenResult d = eigen3(Matrix3D({{{{2, 0, 0}}, {{0, 2, 0}}, {{0, 0, 3}}}}));
    CHECK_NEAR(d.values[0].re, 3.0f, 1e-4f);
    CHECK(d.values[1].hasVector && d.values[2].hasVector);
    if (d.values[1].hasVector && d.values[2].hasVector)
        CHECK(d.values[1].vector.cross(d.values[2].vector).magnitude() > 0.9f);

    // Shear is defective: λ = 1 (x3) but only a 2D eigenspace
    EigenResult s = eigen3(Matrix3D::shear(0.5f, 0.0f));
    int withVector = 0;
    for (const auto& ev : s.values) {
        CHECK_NEAR(ev.re, 1.0f, 1e-3f);
        if (ev.hasVector) withVector++;
    }
    CHECK(withVector == 2);
}

void testComplex() {
    Complex a(1, 2), b(3, -1);
    Complex p = a * b;
    CHECK_NEAR(p.re, 5.0f, 1e-6f);
    CHECK_NEAR(p.im, 5.0f, 1e-6f);
    Complex q = p / b;
    CHECK_NEAR(q.re, 1.0f, 1e-5f);
    CHECK_NEAR(q.im, 2.0f, 1e-5f);

    // Multiplication multiplies moduli and adds arguments
    CHECK_NEAR(p.abs(), a.abs() * b.abs(), 1e-5f);
    CHECK_NEAR(p.arg(), a.arg() + b.arg(), 1e-5f);

    // Euler: e^(iπ) = -1
    Complex e = Complex::expi(3.14159265f);
    CHECK_NEAR(e.re, -1.0f, 1e-6f);
    CHECK_NEAR(e.im, 0.0f, 1e-6f);

    Complex z = Complex::fromPolar(2.0f, 0.5f);
    CHECK_NEAR(z.abs(), 2.0f, 1e-6f);
    CHECK_NEAR(z.arg(), 0.5f, 1e-6f);
    CHECK(Complex(1, -2).toRectString() == "1.00 - 2.00i");
    CHECK(Complex(0, 1).toPolarString() == "1.00 e^(i 90.00 deg)");
}

void testExpression() {
    Expression e;
    CHECK(e.parse("1 + 2 * 3"));
    CHECK_NEAR(e.eval(0), 7.0, 1e-12);

    CHECK(e.parse("2^3^2"));  // right-associative
    CHECK_NEAR(e.eval(0), 512.0, 1e-9);

    CHECK(e.parse("-x^2"));
    CHECK_NEAR(e.eval(3), -9.0, 1e-12);

    CHECK(e.parse("2x + 3(x+1)"));
    CHECK_NEAR(e.eval(2), 13.0, 1e-12);

    CHECK(e.parse("sin(pi/2) + cos(0) + sqrt(16) + abs(-2)"));
    CHECK_NEAR(e.eval(0), 8.0, 1e-12);

    CHECK(e.parse("x*y + t"));
    CHECK_NEAR(e.eval(2, 3, 4), 10.0, 1e-12);

    CHECK(e.parse("exp(-(x^2 + y^2))"));
    CHECK_NEAR(e.eval(0, 0), 1.0, 1e-12);

    CHECK(e.parse("1/x"));
    CHECK(std::isinf(e.eval(0)));

    CHECK(e.parse("2 ^ -1"));
    CHECK_NEAR(e.eval(0), 0.5, 1e-12);

    CHECK(e.parse("1.5e2"));
    CHECK_NEAR(e.eval(0), 150.0, 1e-12);

    // Errors
    CHECK(!e.parse("1 +"));
    CHECK(!e.error().empty());
    CHECK(!e.valid());
    CHECK(std::isnan(e.eval(0)));
    CHECK(!e.parse("sin x"));
    CHECK(!e.parse("(1 + 2"));
    CHECK(!e.parse("foo(1)"));
    CHECK(!e.parse("1 $ 2"));
    CHECK(!e.parse(""));
}

void testAnimator() {
    Animator a;
    CHECK(!a.active());

    // Morph: identity at t=0, target at t=1, halfway in between
    Matrix4D target = Matrix4D::scale(3, 3, 3);
    a.setMorphTarget(target);
    a.start(AnimationPreset::Morph);
    CHECK(a.active() && a.playing());
    CHECK(near(a.matrixAt(0.0f), Matrix4D::identity()));
    CHECK(near(a.matrixAt(1.0f), target));
    CHECK_NEAR(a.matrixAt(0.5f).m[0][0], 2.0f, 1e-6f);

    // Rotation stays a rotation (determinant 1) at every t
    a.start(AnimationPreset::Rotation);
    CHECK_NEAR(a.matrixAt(0.37f).determinant(), 1.0f, 1e-4f);
    CHECK(near(a.matrixAt(1.0f), Matrix4D::identity()));

    // Playback: one run takes kDuration seconds at 1x speed
    a.setLoop(false);
    a.update(Animator::kDuration / 2);
    CHECK_NEAR(a.time(), 0.5f, 1e-5f);
    a.update(Animator::kDuration);
    CHECK_NEAR(a.time(), 1.0f, 1e-6f);
    CHECK(!a.playing());
    a.togglePlay();  // restarts from the beginning
    CHECK(a.playing());
    CHECK_NEAR(a.time(), 0.0f, 1e-6f);

    // Looping wraps around
    a.setLoop(true);
    a.setSpeed(2.0f);
    a.update(Animator::kDuration * 0.75f);  // 1.5 runs
    CHECK_NEAR(a.time(), 0.5f, 1e-4f);

    // Keyframes are evenly spaced
    a.addKeyframe(Matrix4D::identity());
    a.addKeyframe(Matrix4D::scale(2, 2, 2));
    a.addKeyframe(Matrix4D::scale(4, 4, 4));
    a.start(AnimationPreset::Keyframes);
    CHECK_NEAR(a.matrixAt(0.25f).m[0][0], 1.5f, 1e-5f);
    CHECK_NEAR(a.matrixAt(0.75f).m[0][0], 3.0f, 1e-5f);
    CHECK_NEAR(a.matrixAt(1.0f).m[0][0], 4.0f, 1e-5f);
    a.clearKeyframes();
    CHECK(!a.active());

    a.start(AnimationPreset::Scaling);
    a.stop();
    CHECK(!a.active() && !a.playing());
}

} // namespace

int main() {
    struct Test { const char* name; std::function<void()> fn; };
    const Test tests[] = {
        {"Vector3D", testVector},
        {"Matrix3D", testMatrix3D},
        {"Matrix4D", testMatrix4D},
        {"LU", testLU},
        {"QR", testQR},
        {"SVD", testSVD},
        {"Eigen", testEigen},
        {"Complex", testComplex},
        {"Expression", testExpression},
        {"Animator", testAnimator},
    };

    for (const auto& t : tests) {
        int before = g_failures;
        t.fn();
        std::printf("%-12s %s\n", t.name, g_failures == before ? "ok" : "FAILED");
    }
    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
