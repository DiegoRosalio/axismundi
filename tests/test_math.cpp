// Unit tests for the math library (no graphics dependencies).
// Build and run with: make test

#include "animation/Playback.hpp"
#include "math/expr/Reference.hpp"
#include "math/Contour.hpp"
#include "export/LatexExport.hpp"
#include "export/PdfCompile.hpp"
#include "math/VectorField.hpp"
#include "math/Integrals.hpp"
#include "text/LabelLayout.hpp"
#include "text/TextLayout.hpp"
#include <fstream>
#include "math/Complex.hpp"
#include "math/Decompositions.hpp"
#include "math/Expression.hpp"
#include "math/Explain.hpp"
#include "math/expr/Expr.hpp"
#include "scene/Scene.hpp"
#include <map>
#include <memory>
#include "math/Breakdown.hpp"
#include "math/Quaternion.hpp"
#include "math/Matrix3D.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
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

void testPlayback() {
    // Every easing starts at 0 and ends at 1
    for (int i = 0; i < kEasingCount; i++) {
        Easing e = static_cast<Easing>(i);
        CHECK_NEAR(ease(e, 0.0f), 0.0f, 1e-5f);
        CHECK_NEAR(ease(e, 1.0f), 1.0f, 1e-5f);
        CHECK(std::string(easingName(e)).size() > 0);
    }
    // The monotonic ones never go backwards
    for (Easing e : {Easing::Linear, Easing::EaseIn, Easing::EaseOut, Easing::EaseInOut, Easing::Sine, Easing::Step}) {
        float prev = 0.0f;
        for (int k = 1; k <= 100; k++) {
            float v = ease(e, k / 100.0f);
            CHECK(v >= prev - 1e-6f);
            prev = v;
        }
    }
    CHECK(ease(Easing::Back, 0.8f) > 1.0f);         // overshoots
    CHECK_NEAR(ease(Easing::EaseInOut, 0.5f), 0.5f, 1e-5f);
    CHECK(ease(Easing::EaseIn, 0.5f) < 0.5f);
    CHECK(ease(Easing::EaseOut, 0.5f) > 0.5f);

    Playback p;
    p.duration = 2.0f;
    p.repeat = Repeat::Once;
    p.restart();
    p.advance(1.0f);
    CHECK_NEAR(p.phase, 0.5f, 1e-5f);
    p.advance(5.0f);
    CHECK_NEAR(p.phase, 1.0f, 1e-6f);
    CHECK(!p.playing);
    p.toggle();  // replays from the start
    CHECK(p.playing && p.phase == 0.0f);

    p.repeat = Repeat::Loop;
    p.advance(3.0f);  // 1.5 runs
    CHECK_NEAR(p.phase, 0.5f, 1e-5f);

    p.repeat = Repeat::PingPong;
    p.restart();
    p.advance(3.0f);  // to the end and half way back
    CHECK_NEAR(p.phase, 0.5f, 1e-5f);
    CHECK(p.direction == -1);
    p.advance(1.5f);  // past the start, forwards again
    CHECK_NEAR(p.phase, 0.25f, 1e-5f);
    CHECK(p.direction == 1);

    // Pure phase from a clock (scrubbable)
    Playback c;
    c.duration = 2.0f;
    c.repeat = Repeat::Once;
    CHECK_NEAR(c.phaseAt(0.0), 0.0f, 1e-6f);
    CHECK_NEAR(c.phaseAt(1.0), 0.5f, 1e-6f);
    CHECK_NEAR(c.phaseAt(9.0), 1.0f, 1e-6f);
    c.repeat = Repeat::Loop;
    CHECK_NEAR(c.phaseAt(3.0), 0.5f, 1e-6f);
    CHECK_NEAR(c.phaseAt(4.0), 1.0f, 1e-6f);
    c.repeat = Repeat::PingPong;
    CHECK_NEAR(c.phaseAt(3.0), 0.5f, 1e-6f);   // on the way back
    CHECK_NEAR(c.phaseAt(2.0), 1.0f, 1e-6f);
    CHECK_NEAR(c.phaseAt(5.0), 0.5f, 1e-6f);   // forwards again
    c.easing = Easing::EaseIn;
    CHECK_NEAR(c.valueAt(1.0), 0.25f, 1e-6f);

    SceneData data;
    p.easing = Easing::Bounce;
    p.save(data, "x.");
    Playback q;
    q.load(data, "x.");
    CHECK(q.easing == Easing::Bounce && q.repeat == Repeat::PingPong);
    CHECK_NEAR(q.duration, 2.0f, 1e-5f);
}

// Text of the last Result line in an explanation
std::string lastResult(const Explanation& e) {
    std::string r;
    for (const auto& l : e)
        if (l.kind == ExplainLine::Kind::Result) r = l.text;
    return r;
}

bool contains(const Explanation& e, const std::string& needle) {
    for (const auto& l : e)
        if (l.text.find(needle) != std::string::npos) return true;
    return false;
}

void testExplain() {
    CHECK(explainNumber(2.0f) == "2");
    CHECK(explainNumber(0.70710678f) == "0.707");
    CHECK(explainNumber(-1.5f) == "−1.5");
    CHECK(explainNumber(1e-6f) == "0");

    Explanation dot = explainDot(Vector3D(1, 2, 3), Vector3D(4, -5, 6));
    CHECK(contains(dot, "A · B = 12"));
    CHECK(contains(dot, "2·(−5)"));  // negative factors in parentheses

    Explanation cross = explainCross(Vector3D(1, 0, 0), Vector3D(0, 1, 0));
    CHECK(lastResult(cross) == "A × B = (0, 0, 1)");

    Matrix4D m = Matrix4D::identity();
    m.m[0][1] = 2;  // shear
    Explanation mv = explainMatVec(m, false, Vector3D(1, 1, 0));
    CHECK(lastResult(mv) == "M · A = (3, 1, 0)");
    Explanation mv4 = explainMatVec(Matrix4D::translation(1, 2, 3), true, Vector3D(0, 0, 0));
    CHECK(lastResult(mv4) == "M · A = (1, 2, 3)");

    Matrix3D a({{{{2, 1, 0}}, {{1, 3, 1}}, {{0, 1, 4}}}});
    Matrix4D a4 = Matrix4D::identity();
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) a4.m[i][j] = a.m[i][j];
    CHECK(lastResult(explainDeterminant(a4, false)) == "det M = 18");
    Matrix4D s4 = Matrix4D::scale(2, 3, 4);
    CHECK(lastResult(explainDeterminant(s4, true)) == "det M = 24");

    Explanation inv = explainInverse(a4, false);
    CHECK(contains(inv, "det M = 18"));
    CHECK(lastResult(inv) == "M⁻¹ · M = I");
    Matrix4D singular = Matrix4D::scale(1, 1, 0);
    CHECK(lastResult(explainInverse(singular, false)).find("no inverse") != std::string::npos);

    Explanation eig = explainEigen(Matrix3D({{{{2, 0, 0}}, {{0, 3, 0}}, {{0, 0, 5}}}}));
    CHECK(contains(eig, "λ³ − 10λ² + 31λ − 30 = 0"));
    CHECK(contains(eig, "λ₁ = 5"));

    Explanation prod = explainComplexProduct(Complex(1, 2), Complex(3, -1));
    CHECK(contains(prod, "z₁z₂ = 5 + 5i"));
    CHECK(contains(explainPolar(Complex(0, 2), "z"), "arg z = atan2(2, 0) = 90°"));
    CHECK(contains(explainEuler(3.14159265f), "e^(iθ) = −1 + 0i"));
}

void testQuaternion() {
    Quaternion q = Quaternion::fromAxisAngle(Vector3D(0, 0, 1), 3.14159265f / 2);
    CHECK(near(q.toMatrix(), Matrix3D::rotation(3.14159265f / 2)));
    CHECK_NEAR(q.angle(), 3.14159265f / 2, 1e-5f);
    CHECK_NEAR(q.axis().z, 1.0f, 1e-5f);

    // Round trip through a matrix, including a 180° turn
    for (float angle : {0.3f, 1.7f, 3.14159265f}) {
        Quaternion r = Quaternion::fromAxisAngle(Vector3D(1, 2, 2), angle);
        Quaternion back = Quaternion::fromMatrix(r.toMatrix());
        CHECK(near(back.toMatrix(), r.toMatrix()));
    }

    // Slerp halfway is half the angle, about the same axis
    Quaternion half = Quaternion::slerp(Quaternion::identity(), q, 0.5f);
    CHECK_NEAR(half.angle(), 3.14159265f / 4, 1e-5f);
    CHECK(near(Quaternion::slerp(Quaternion::identity(), q, 1.0f).toMatrix(), q.toMatrix()));
}

void testBreakdown() {
    std::vector<Matrix4D> samples;
    for (const auto& m3 : sampleMatrices()) {
        Matrix4D m = Matrix4D::identity();
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) m.m[i][j] = m3.m[i][j];
        samples.push_back(m);
    }
    samples.push_back(Matrix4D::translation(1, -2, 0.5f) * Matrix4D::rotationY(0.8f) * Matrix4D::scale(2, 1, 0.5f));
    samples.push_back(Matrix4D::scale(3, 3, 3));
    samples.push_back(Matrix4D::scale(-2, 2, 2));

    for (const auto& m : samples) {
        TransformBreakdown b = breakdown(m);
        // All steps together rebuild M (affine part)
        CHECK(near(b.at(static_cast<float>(b.steps.size())), m, 2e-3f));
        CHECK(near(b.at(0.0f), Matrix4D::identity()));
        for (const auto& s : b.steps) {
            if (s.kind != BreakdownStep::Kind::Rotation) continue;
            Matrix4D r = s.matrix();
            CHECK_NEAR(r.determinant(), 1.0f, 1e-4f);
            // Halfway through a rotation is still a rotation
            CHECK_NEAR(s.partial(0.5f).determinant(), 1.0f, 1e-4f);
        }
    }

    // A pure rotation is one step; identity has none
    CHECK(breakdown(Matrix4D::rotationZ(0.7f)).steps.size() == 1);
    CHECK(breakdown(Matrix4D::identity()).steps.empty());

    // A plain mirror is a single "flip x"
    TransformBreakdown mirror = breakdown(Matrix4D::scale(-1, 1, 1));
    CHECK(mirror.steps.size() == 1);
    if (mirror.steps.size() == 1) {
        CHECK(mirror.steps[0].title == "Mirror");
        CHECK(mirror.steps[0].description == "flip x");
    }

    // Uniform scale: one "Enlarge" step
    TransformBreakdown big = breakdown(Matrix4D::scale(2, 2, 2));
    CHECK(big.steps.size() == 1 && big.steps[0].title == "Enlarge");

    // Translation comes last
    TransformBreakdown moved = breakdown(Matrix4D::translation(1, 2, 3));
    CHECK(moved.steps.size() == 1 && moved.steps[0].kind == BreakdownStep::Kind::Translation);

    // Singular: a squash
    TransformBreakdown flat = breakdown(Matrix4D::scale(1, 1, 0));
    bool squashed = false;
    for (const auto& s : flat.steps)
        if (s.title == "Flatten") squashed = true;
    CHECK(squashed);
}

struct MapResolver : expr::Resolver {
    std::map<std::string, expr::Value> values;
    bool lookup(const std::string& name, expr::Value& out) const override {
        auto it = values.find(name);
        if (it == values.end()) return false;
        out = it->second;
        return true;
    }
};

expr::Value evalText(const std::string& text, const expr::Resolver* r = nullptr) {
    expr::Program p;
    if (!p.compile(text)) return expr::Value::fail(p.error());
    return p.eval(r);
}

void testVectorField() {
    using namespace field;
    auto program = [](const char* text) {
        auto p = std::make_unique<expr::Program>();
        CHECK(p->compile(text, expr::kVarZ));
        return p;
    };
    Box box{{-4, -4, -2}, {4, 4, 2}};

    // Arrows: grid centers, the longest fits 90% of a cell, zero vectors skipped
    auto rot = program("[-y, x, 0]");
    FieldFunction plane(*rot, nullptr, 0.0, true);
    ArrowSet arrows = sampleArrows(plane, box, 8, 8, 1, false);
    CHECK(arrows.arrows.size() == 64);
    CHECK_NEAR(arrows.cell, 1.0, 1e-9);
    double longest = 0;
    for (const Arrow& a : arrows.arrows) longest = std::max(longest, static_cast<double>(a.vec.magnitude()));
    CHECK_NEAR(longest, 0.9, 1e-5);
    CHECK_NEAR(arrows.maxMagnitude, std::sqrt(3.5 * 3.5 * 2), 1e-5);
    CHECK_NEAR(arrows.arrows[0].base.x, -3.5, 1e-6);
    CHECK_NEAR(arrows.arrows[0].base.z, 0.0, 1e-9);
    ArrowSet same = sampleArrows(plane, box, 8, 8, 1, true);
    CHECK_NEAR(same.arrows[0].vec.magnitude(), same.arrows[10].vec.magnitude(), 1e-5);
    auto zero = program("[x - x, 0, 0]");
    CHECK(sampleArrows(FieldFunction(*zero, nullptr, 0, true), box, 4, 4, 1, false).arrows.empty());
    // Volume: nx * ny * nz arrows; plane fields drop F's z component
    auto up = program("[0, 0, 1 + z*z]");
    CHECK(sampleArrows(FieldFunction(*up, nullptr, 0, false), box, 3, 3, 4, false).arrows.size() == 36);
    CHECK(sampleArrows(FieldFunction(*up, nullptr, 0, true), box, 3, 3, 4, false).arrows.empty());

    // Flow line of a rotation: a closed circle that keeps its radius, duration ≈ 2π
    FlowLine circle = traceFlowLine(plane, {2, 0, 0}, box, 0.05, 2000);
    CHECK(circle.closed);
    double worst = 0;
    for (const Vector3D& p : circle.points) worst = std::max(worst, std::fabs(p.magnitude() - 2.0));
    CHECK(worst < 1e-3);
    CHECK_NEAR(circle.duration(), 2 * 3.14159265358979, 0.05);
    CHECK_NEAR(particleAt(circle, circle.duration() / 2).x, -2.0, 0.02);  // half a turn later

    // Source: the line runs out of the box (both ends clipped at the edge or stopped at 0)
    auto source = program("[x, y, 0]");
    FieldFunction src(*source, nullptr, 0, true);
    FlowLine out = traceFlowLine(src, {1, 0.5, 0}, box, 0.05, 2000);
    CHECK(!out.closed);
    CHECK(out.points.size() > 10);
    CHECK_NEAR(std::max(std::fabs(out.points.back().x), std::fabs(out.points.back().y)), 4.0, 1e-3);
    CHECK(out.points.front().magnitude() < 0.05);  // traced back to the source at the origin
    for (size_t i = 1; i < out.times.size(); i++) CHECK(out.times[i] >= out.times[i - 1]);

    // Kept apart: spirals into a sink don't pile up; a rotation still gives whole circles
    auto sink = program("[-y - x/4, x - y/4, 0]");
    FieldFunction vortex(*sink, nullptr, 0, true);
    auto seeds = flowSeeds(box, true, 5);
    auto apart = traceFlowLines(vortex, seeds, box, 0.05, 600, 0.4);
    CHECK(!apart.empty() && apart.size() <= seeds.size());
    bool spaced = true;
    for (size_t a = 0; a < apart.size(); a++)
        for (size_t b = a + 1; b < apart.size(); b++)
            for (const Vector3D& p : apart[b].points)
                for (const Vector3D& q : apart[a].points)
                    if ((p - q).magnitude() < 0.4 - 1e-4) spaced = false;
    CHECK(spaced);
    Box big{{-6, -6, -2}, {6, 6, 2}};  // every circle through the seeds fits
    auto rings = traceFlowLines(plane, flowSeeds({{-4, -4, 0}, {4, 4, 0}}, true, 4), big, 0.05, 2000, 0.3);
    bool allClosed = !rings.empty();
    for (const FlowLine& r : rings) allClosed = allClosed && r.closed;
    CHECK(allClosed);
    CHECK(rings.size() == 3);  // radii √2, √10, √18: seeds on a circle already drawn are skipped

    // Seeds and particle times (periodic in the scene time, spread along the line)
    CHECK(flowSeeds(box, true, 5).size() == 25);
    CHECK(flowSeeds(box, false, 3).size() == 27);
    double T = circle.duration();
    CHECK_NEAR(particleTime(circle, 0, 3, 0.0, 1.0), 0.0, 1e-12);
    CHECK_NEAR(particleTime(circle, 1, 3, 0.0, 1.0), T / 3, 1e-9);
    CHECK_NEAR(particleTime(circle, 0, 3, T + 0.25, 1.0), 0.25, 1e-9);
    CHECK_NEAR(particleTime(circle, 0, 3, 0.25, 2.0), 0.5, 1e-9);
}

void testIntegrals() {
    using namespace integrals;
    constexpr double pi = 3.14159265358979;
    Surface xy = [](double x, double y, double& z) { z = x * y; return true; };
    Rect unit{0, 1, 0, 1};
    bool ok = false;
    CHECK_NEAR(simpson2D(xy, unit, 10, ok), 0.25, 1e-12);
    CHECK(ok);
    // Midpoint boxes are exact for x·y; left boxes undershoot
    RiemannSum mid = riemann2D(xy, unit, 4, 4, Rule::Middle);
    CHECK(mid.boxes.size() == 16);
    CHECK_NEAR(mid.sum, 0.25, 1e-12);
    CHECK(riemann2D(xy, unit, 4, 4, Rule::Left).sum < 0.25);
    CHECK(riemann2D(xy, unit, 4, 4, Rule::Right).sum > 0.25);
    // The midpoint error shrinks about 4x when the boxes double (second order)
    Surface bowl = [](double x, double y, double& z) { z = x * x + y * y; return true; };
    Rect sq{-1, 1, -1, 1};
    double exact = 8.0 / 3.0;
    double e4 = std::fabs(riemann2D(bowl, sq, 4, 4, Rule::Middle).sum - exact);
    double e8 = std::fabs(riemann2D(bowl, sq, 8, 8, Rule::Middle).sum - exact);
    CHECK(e8 > 0 && e4 / e8 > 3.5 && e4 / e8 < 4.5);
    CHECK_NEAR(simpson2D(bowl, sq, 20, ok), exact, 1e-9);
    // Sign change: x over [−1, 1] integrates to 0 (boxes below the floor count negative)
    Surface lin = [](double x, double, double& z) { z = x; return true; };
    CHECK_NEAR(riemann2D(lin, sq, 6, 6, Rule::Middle).sum, 0.0, 1e-12);
    // Undefined cells are skipped (and reported)
    Surface hole = [](double x, double y, double& z) { z = 1.0; return x * x + y * y > 0.1; };
    RiemannSum h = riemann2D(hole, sq, 4, 4, Rule::Middle);
    CHECK(h.skipped == 0);  // centers ±0.25, ±0.75: none within the hole
    simpson2D(hole, sq, 4, ok);
    CHECK(!ok);  // the grid hits (0, 0)

    // Line integrals: circulation of a rotation around the unit circle = 2π
    Field rot = [](const D3& p, D3& o) { o = {-p.y, p.x, 0}; return true; };
    Path circle = [](double u, D3& o) { o = {std::cos(u), std::sin(u), 0}; return true; };
    CHECK_NEAR(lineIntegral(rot, circle, 0, 2 * pi, 1.0, 200, ok), 2 * pi, 1e-6);
    CHECK(ok);
    CHECK_NEAR(lineIntegral(rot, circle, 0, 2 * pi, 0.5, 200, ok), pi, 1e-6);  // halfway: half
    // A gradient field ∇(x·y·z) = [yz, xz, xy]: the work is the endpoint difference, on any path
    Field grad = [](const D3& p, D3& o) { o = {p.y * p.z, p.x * p.z, p.x * p.y}; return true; };
    Path straight = [](double u, D3& o) { o = {u, u, u}; return true; };
    Path wiggly = [](double u, D3& o) { o = {u, u * u, u + std::sin(pi * u)}; return true; };
    CHECK_NEAR(lineIntegral(grad, straight, 0, 1, 1.0, 100, ok), 1.0, 1e-8);
    CHECK_NEAR(lineIntegral(grad, wiggly, 0, 1, 1.0, 200, ok), 1.0, 1e-6);
}

void testQuaternions() {
    using namespace expr;
    auto ev = [](const char* text) {
        Program p;
        if (!p.compile(text)) return Value::fail("parse: " + p.error());
        return p.eval();
    };
    auto near = [](const Value& v, double w, double x, double y, double z) {
        return v.type == Type::Quaternion && std::fabs(v.re - w) < 1e-9 && std::fabs(v.qi - x) < 1e-9 &&
               std::fabs(v.qj - y) < 1e-9 && std::fabs(v.qk - z) < 1e-9;
    };
    // Hamilton's rules: i·j = k, j·i = −k, i² = −1
    CHECK(near(ev("quat(0, 1, 0, 0) * quat(0, 0, 1, 0)"), 0, 0, 0, 1));
    CHECK(near(ev("quat(0, 0, 1, 0) * quat(0, 1, 0, 0)"), 0, 0, 0, -1));
    CHECK(near(ev("quat(0, 1, 0, 0)^2"), -1, 0, 0, 0));
    CHECK(near(ev("2 * quat(1, 2, 3, 4) - quat(1, 2, 3, 4)"), 1, 2, 3, 4));
    CHECK(near(ev("quat(1, 2, 3, 4) + 1"), 2, 2, 3, 4));
    CHECK(near(ev("conj(quat(1, 2, 3, 4))"), 1, -2, -3, -4));
    CHECK(near(ev("quat(1, 2, 3, 4) * inv(quat(1, 2, 3, 4))"), 1, 0, 0, 0));
    CHECK(near(ev("quat(1, 2, 3, 4) / quat(1, 2, 3, 4)"), 1, 0, 0, 0));
    CHECK_NEAR(ev("abs(quat(1, 1, 1, 1))").re, 2, 1e-12);
    CHECK_NEAR(ev("abs(unit(quat(1, 2, 3, 4)))").re, 1, 1e-12);
    CHECK_NEAR(ev("quat(1, 2, 3, 4).y").re, 3, 1e-12);
    CHECK_NEAR(ev("quat(1, 2, 3, 4).w").re, 1, 1e-12);
    CHECK(ev("quat(1, 0, 0, 0.7071)").summary() == "1 + 0.707k");
    CHECK(ev("quat(0, -1, 0, 0)").summary() == "−i");
    CHECK(ev("quat(0, 0, 0, 0)").summary() == "0");
    // Rotations: qaxis agrees with rotz, rotate turns vectors, angle/axis read it back
    const double s2 = std::sqrt(0.5);
    CHECK(near(ev("qaxis([0, 0, 2], pi/2)"), s2, 0, 0, s2));
    Value turned = ev("rotate(qaxis([0, 0, 1], pi/2), [1, 0, 0])");
    CHECK(turned.type == Type::Vector && std::fabs(turned.vector.y - 1) < 1e-6 && std::fabs(turned.vector.x) < 1e-6);
    Value m = ev("mat(qaxis([0, 0, 1], 0.7)) - rotz(0.7)");
    double worst = 0;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) worst = std::max(worst, static_cast<double>(std::fabs(m.matrix.m[i][j])));
    CHECK(worst < 1e-6);
    CHECK_NEAR(ev("angle(qaxis([1, 1, 0], 2))").re, 2, 1e-6);  // vectors are single precision
    CHECK_NEAR(ev("angle(-qaxis([1, 1, 0], 2))").re, 2, 1e-6);  // −q is the same rotation
    CHECK_NEAR(ev("axis(qaxis([0, 3, 0], 1)).y").re, 1, 1e-6);
    CHECK(ev("axis(quat(1, 0, 0, 0))").error.find("no rotation") == 0);
    // quat(M) round trip (w ≥ 0), and only for rotation matrices
    CHECK(near(ev("quat(mat(qaxis([1, 2, 2], 1.2)))"), ev("qaxis([1, 2, 2], 1.2)").re,
               ev("qaxis([1, 2, 2], 1.2)").qi, ev("qaxis([1, 2, 2], 1.2)").qj, ev("qaxis([1, 2, 2], 1.2)").qk) ||
          std::fabs(ev("abs(quat(mat(qaxis([1, 2, 2], 1.2))) - qaxis([1, 2, 2], 1.2))").re) < 1e-5);
    CHECK(!ev("quat(scale(2))").ok());
    // Order matters: rotating about x then z differs from z then x
    CHECK(ev("abs(qaxis([0, 0, 1], pi/2) * qaxis([1, 0, 0], pi/2) - qaxis([1, 0, 0], pi/2) * qaxis([0, 0, 1], pi/2))").re > 0.5);
    // slerp: halfway between the identity and a half turn about z is a quarter turn; lerp/keys slerp too
    CHECK(near(ev("slerp(quat(1, 0, 0, 0), qaxis([0, 0, 1], pi), 0.5)"), s2, 0, 0, s2));
    CHECK(near(ev("lerp(quat(1, 0, 0, 0), qaxis([0, 0, 1], pi), 0.5)"), s2, 0, 0, s2));
    CHECK(near(ev("keys(0.5, quat(1, 0, 0, 0), qaxis([0, 0, 1], pi/2), qaxis([0, 0, 1], pi))"), s2, 0, 0, s2));
    CHECK_NEAR(ev("abs(slerp(quat(2, 0, 0, 0), quat(0, 0, 0, 3), 0.3))").re, 1, 1e-9);  // unit result
    // Mixing with other kinds explains itself
    CHECK(ev("qaxis([0, 0, 1], 1) * [1, 0, 0]").error.find("rotate(q, v)") != std::string::npos);
    CHECK(ev("qaxis([0, 0, 1], 1) * rotz(1)").error.find("mat(q)") != std::string::npos);
    CHECK(ev("qaxis([0, 0, 1], 1) + (1 + 2i)").error.find("quat(re, im, 0, 0)") != std::string::npos);
    CHECK(!ev("quat(1, 2, 3, 4)^0.5").ok());
    // Common words stay usable as object names: angle( is the function, angle alone a name
    CHECK(isValidName("angle") && isValidName("axis") && isValidName("mat") && isValidName("rotate"));
    CHECK(!isValidName("qaxis") && !isValidName("slerp"));
    Program named;
    CHECK(named.compile("2*angle + angle(qaxis([0, 0, 1], 1))"));
    CHECK(named.names().count("angle") == 1);
}

void testFieldVariable() {
    using namespace expr;
    // z is a variable only with kVarZ; otherwise a name (objects may be called z)
    Program f;
    CHECK(f.compile("[-y, x, z/2]", kVarZ));
    CHECK(f.usesX() && f.usesY() && f.usesZ());
    CHECK(f.names().empty());
    Slots s;
    s.x = 1, s.y = 2, s.z = 4;
    s.hasX = s.hasY = s.hasZ = true;
    Value v = f.eval(nullptr, &s);
    CHECK(v.type == Type::Vector);
    CHECK_NEAR(v.vector.x, -2, 1e-12);
    CHECK_NEAR(v.vector.y, 1, 1e-12);
    CHECK_NEAR(v.vector.z, 2, 1e-12);
    Slots noZ = s;
    noZ.hasZ = false;
    CHECK(f.eval(nullptr, &noZ).error == "z is only available in vector fields");

    Program plain;
    CHECK(plain.compile("z + 1"));
    CHECK(!plain.usesZ());
    CHECK(plain.names().count("z") == 1);
    Program both;
    CHECK(both.compile("u + z", kVarU | kVarZ));
    CHECK(both.usesU() && both.usesZ() && both.names().empty());
    CHECK(both.compile("u + z", true));  // true = curves: u only
    CHECK(both.usesU() && !both.usesZ() && both.names().count("z") == 1);

    // Field formulas must give vectors
    CHECK(scene::checkExpect(scene::Expect::Field, Value::vec({1, 2, 3})).empty());
    CHECK(!scene::checkExpect(scene::Expect::Field, Value::scalar(1)).empty());
}

void testExprEngine() {
    using expr::Type;
    using expr::Value;

    // Numbers and complex numbers
    CHECK_NEAR(evalText("2 + 3*4").re, 14.0, 1e-12);
    Value z = evalText("(1 + 2i)(3 - i)");
    CHECK(z.type == Type::Complex);
    CHECK_NEAR(z.re, 5.0, 1e-12);
    CHECK_NEAR(z.im, 5.0, 1e-12);
    Value i2 = evalText("i^2");
    CHECK_NEAR(i2.re, -1.0, 1e-12);
    CHECK_NEAR(i2.im, 0.0, 1e-12);
    Value euler = evalText("exp(i pi)");
    CHECK_NEAR(euler.re, -1.0, 1e-9);
    CHECK_NEAR(std::fabs(euler.im), 0.0, 1e-9);
    CHECK_NEAR(evalText("abs(3 + 4i)").re, 5.0, 1e-12);
    CHECK_NEAR(evalText("arg(i)").re, 3.14159265 / 2, 1e-6);
    Value p = evalText("polar(2, pi/2)");
    CHECK_NEAR(p.im, 2.0, 1e-9);

    // Vectors
    Value v = evalText("[1, 2, 3] + 2[0, 1, 0]");
    CHECK(v.type == Type::Vector);
    CHECK_NEAR(v.vector.y, 4.0f, 1e-6f);
    CHECK(evalText("cross([1,0,0], [0,1,0])").vector.z == 1.0f);
    CHECK_NEAR(evalText("dot([1,2,3], [4,5,6])").re, 32.0, 1e-9);
    CHECK_NEAR(evalText("norm([3, 4, 0])").re, 5.0, 1e-6);
    CHECK_NEAR(evalText("[1, 2, 3].y").re, 2.0, 1e-9);
    CHECK(!evalText("[1,0,0] * [0,1,0]").ok());  // suggests dot / cross
    CHECK(evalText("[1,0,0] * [0,1,0]").error.find("dot") != std::string::npos);

    // Matrices
    Value m = evalText("[[1, 2, 0], [0, 1, 0], [0, 0, 1]]");
    CHECK(m.type == Type::Matrix && m.size == 3);
    CHECK_NEAR(m.matrix.m[0][1], 2.0f, 1e-6f);
    Value mv = evalText("[[0,-1,0],[1,0,0],[0,0,1]] * [1, 0, 0]");
    CHECK_NEAR(mv.vector.y, 1.0f, 1e-6f);
    CHECK_NEAR(evalText("det(scale(2))").re, 8.0, 1e-6);
    CHECK_NEAR(evalText("tr(eye(4))").re, 4.0, 1e-9);
    Value r = evalText("rotz(pi/2) * [1, 0, 0]");
    CHECK_NEAR(r.vector.y, 1.0f, 1e-6f);
    Value ra = evalText("rot([0, 0, 1], pi/2) * [1, 0, 0]");
    CHECK_NEAR(ra.vector.y, 1.0f, 1e-6f);
    Value cols = evalText("[[1,0,0],[0,1,0],[0,0,1]]");
    CHECK(cols.ok());
    Value fromColumns = evalText("[[0,1,0], [1,0,0], [0,0,1]]");
    CHECK(fromColumns.ok());
    Value colMatrix = evalText("[cross([1,0,0],[0,1,0]), [1,0,0], [0,1,0]]");  // columns
    CHECK(colMatrix.type == Type::Matrix);
    CHECK_NEAR(colMatrix.matrix.m[2][0], 1.0f, 1e-6f);
    Value t = evalText("translate(1, 2, 3) * [0, 0, 0]");
    CHECK_NEAR(t.vector.z, 3.0f, 1e-6f);
    CHECK(!evalText("inv([[1,1,0],[1,1,0],[0,0,1]])").ok());
    Value mi = evalText("scale(2)^-1 * [2, 4, 6]");
    CHECK_NEAR(mi.vector.z, 3.0f, 1e-5f);
    CHECK(!evalText("[[1,2],[3,4]]").ok());           // 2x2 not supported
    CHECK(!evalText("[[1,2,3],[4,5,6]]").ok());       // not square

    // Names, dependencies, slots
    MapResolver res;
    res.values["a"] = Value::vec(Vector3D(1, 0, 0));
    res.values["b"] = Value::vec(Vector3D(0, 2, 0));
    res.values["M"] = Value::mat3(Matrix3D::scale(3, 3));
    res.values["theta"] = Value::scalar(3.14159265 / 2);
    expr::Program prog;
    CHECK(prog.compile("M*(a + b) + cross(a, b) * cos(theta)"));
    CHECK(prog.names() == std::set<std::string>({"M", "a", "b", "theta"}));
    Value pv = prog.eval(&res);
    CHECK(pv.type == Type::Vector);
    CHECK_NEAR(pv.vector.x, 3.0f, 1e-5f);
    CHECK_NEAR(pv.vector.y, 6.0f, 1e-5f);
    CHECK(evalText("a + q", &res).error.find("'q'") != std::string::npos);

    expr::Program plot;
    CHECK(plot.compile("x^2 + y*t + theta"));
    CHECK(plot.usesX() && plot.usesY() && plot.usesT());
    CHECK(plot.names() == std::set<std::string>({"theta"}));
    expr::Slots slots;
    slots.x = 2;
    slots.y = 3;
    slots.t = 1;
    slots.hasX = slots.hasY = slots.hasT = true;
    CHECK_NEAR(plot.eval(&res, &slots).re, 4 + 3 + 3.14159265 / 2, 1e-6);
    CHECK(!plot.eval(&res).ok());  // x outside a plot

    // Parse errors and names
    expr::Program bad;
    CHECK(!bad.compile("dot([1,0,0])"));   // wrong argument count
    CHECK(!bad.compile("[1, 2"));
    CHECK(!bad.compile("a.u"));
    CHECK(bad.compile("[1, 0, 0].w"));  // .w is for quaternions: an evaluation error on a vector
    CHECK(bad.eval().error.find(".w is for quaternions") == 0);
    CHECK(expr::isValidName("z1"));
    CHECK(expr::isValidName("M_2"));
    CHECK(!expr::isValidName("sin"));
    CHECK(!expr::isValidName("2a"));
    CHECK(!expr::isValidName("x"));
    CHECK(evalText("[1, 2, 3]").summary() == "(1, 2, 3)");
    CHECK(evalText("1 - 2i").summary() == "1 − 2i");
    CHECK(evalText("-i").summary() == "−i");
}

void testBlendFunctions() {
    using namespace expr;
    auto run = [](const std::string& text) {
        Program p;
        if (!p.compile(text)) return Value::fail(p.error());
        return p.eval(nullptr);
    };
    CHECK_NEAR(run("lerp(2, 4, 0.25)").re, 2.5, 1e-12);
    CHECK_NEAR(run("mod(7, 5)").re, 2.0, 1e-12);
    CHECK_NEAR(run("mod(-1, 5)").re, 4.0, 1e-12);
    CHECK(!run("mod(1, 0)").ok());
    Value v = run("lerp([0, 0, 0], [2, 4, 6], 0.5)");
    CHECK(v.type == Type::Vector && std::fabs(v.vector.y - 2.0f) < 1e-6f);
    v = run("lerp(0, [2, 4, 6], 0.5)");  // 0 = zero vector
    CHECK(v.type == Type::Vector && std::fabs(v.vector.z - 3.0f) < 1e-6f);
    v = run("lerp(1, i, 0.5)");
    CHECK(v.type == Type::Complex && std::fabs(v.re - 0.5) < 1e-12 && std::fabs(v.im - 0.5) < 1e-12);
    v = run("lerp(eye(), 3eye(), 0.5)");
    CHECK(v.type == Type::Matrix && std::fabs(v.matrix.m[1][1] - 2.0f) < 1e-6f && std::fabs(v.matrix.m[0][1]) < 1e-6f);
    CHECK(!run("lerp([1, 0, 0], eye(), 0.5)").ok());
    CHECK(!run("lerp(1, 2, i)").ok());
    // keys: s = 0, 0.5, 1 hit the values; in between it blends
    CHECK_NEAR(run("keys(0, 1, 5, 3)").re, 1.0, 1e-12);
    CHECK_NEAR(run("keys(0.5, 1, 5, 3)").re, 5.0, 1e-12);
    CHECK_NEAR(run("keys(1, 1, 5, 3)").re, 3.0, 1e-12);
    CHECK_NEAR(run("keys(0.75, 1, 5, 3)").re, 4.0, 1e-12);
    CHECK_NEAR(run("keys(2, 1, 5, 3)").re, 3.0, 1e-12);  // clamped
    v = run("keys(0.25, eye(), rotz(pi/2), 2eye())");
    CHECK(v.type == Type::Matrix);
}

void testUsability() {
    using namespace scene;
    Scene sc;
    Space& d = sc.space(SpaceKind::Euclidean3D);
    auto* m = static_cast<MatrixObject*>(d.find("M"));
    sc.evaluate();
    // Presets add up: rotate 45 twice = 90; then scale acts on the turned matrix
    m->applyPreset(Matrix3D::rotation(3.14159265f / 4));
    sc.evaluate();
    m->applyPreset(Matrix3D::rotation(3.14159265f / 4));
    sc.evaluate();
    Matrix3D r = m->value().matrix3();
    CHECK_NEAR(r.m[0][0], 0.0f, 1e-5f);
    CHECK_NEAR(r.m[1][0], 1.0f, 1e-5f);
    m->applyPreset(Matrix3D::scale(2, 2));
    sc.evaluate();
    r = m->value().matrix3();
    CHECK_NEAR(r.m[1][0], 2.0f, 1e-5f);  // scale after rotate
    CHECK_NEAR(r.m[2][2], 1.0f, 1e-6f);
    // Identity starts over
    m->applyPreset(Matrix3D::identity());
    sc.evaluate();
    CHECK_NEAR(m->value().matrix3().m[0][1], 0.0f, 1e-6f);
    CHECK_NEAR(m->value().matrix3().m[1][1], 1.0f, 1e-6f);
    // A formula matrix becomes cells holding the composed value
    m->setByFormula(true);
    m->setField("value", "scale(3)");
    sc.evaluate();
    m->applyPreset(Matrix3D::scale(-1, 1));
    sc.evaluate();
    CHECK(!m->byFormula());
    CHECK_NEAR(m->value().matrix3().m[0][0], -3.0f, 1e-6f);
    CHECK_NEAR(m->value().matrix3().m[1][1], 3.0f, 1e-6f);
    // 4x4 matrices keep their translation
    m->setByFormula(true);
    m->setField("value", "translate(1, 2, 3)");
    sc.evaluate();
    m->applyPreset(Matrix3D::rotation(3.14159265f / 2));
    sc.evaluate();
    CHECK(m->value().size == 4);
    CHECK_NEAR(m->value().matrix.m[0][3], -2.0f, 1e-5f);  // the rotation turns the translation too
    CHECK_NEAR(m->value().matrix.m[2][3], 3.0f, 1e-5f);

    // Empty start, clean history; the demo scene is one undo step
    sc.clearAll();
    sc.clearHistory();
    CHECK(d.objects().empty() && !sc.canUndo());
    sc.loadDefaultScene();
    CHECK(d.find("M") && sc.canUndo());
    sc.undo();
    CHECK(d.objects().empty());
    // New vectors have length 1
    auto* v = sc.add(SpaceKind::Euclidean3D, "vector", "v");
    sc.evaluate();
    CHECK_NEAR(v->value().vector.magnitude(), 1.0f, 1e-6f);
}

void testLabelLayout() {
    using text::Box;
    using text::LabelRequest;
    CHECK(text::overlaps(Box{0, 0, 10, 10}, Box{5, 5, 10, 10}));
    CHECK(!text::overlaps(Box{0, 0, 10, 10}, Box{12, 0, 10, 10}));
    CHECK(text::overlaps(Box{0, 0, 10, 10}, Box{11, 0, 10, 10}, 2.0f));  // within the gap

    // A lone label keeps its preferred spot
    auto one = text::layoutLabels({LabelRequest{100, 100, 40, 16, 8, -16, 0}});
    CHECK(one.size() == 1);
    CHECK_NEAR(one[0].x, 108, 1e-6);
    CHECK_NEAR(one[0].y, 84, 1e-6);

    // Two labels at the same anchor: the second moves, nothing overlaps, order is kept
    std::vector<LabelRequest> same = {{100, 100, 40, 16, 8, -16, 0}, {100, 100, 50, 16, 8, -16, 0}};
    auto two = text::layoutLabels(same);
    CHECK_NEAR(two[0].y, 84, 1e-6);  // first one in place
    CHECK(!text::overlaps(two[0], two[1], 2.0f));
    CHECK_NEAR(two[1].w, 50, 1e-6);

    // Priority wins the preferred spot regardless of order
    same[1].priority = 1;
    auto prio = text::layoutLabels(same);
    CHECK_NEAR(prio[1].x, 108, 1e-6);
    CHECK_NEAR(prio[1].y, 84, 1e-6);
    CHECK(!text::overlaps(prio[0], prio[1], 2.0f));

    // Many labels at one point: all placed without overlaps (stacking after the six spots)
    std::vector<LabelRequest> many(10, LabelRequest{200, 200, 30, 14, 8, -14, 0});
    auto boxes = text::layoutLabels(many);
    bool clean = true;
    for (size_t i = 0; i < boxes.size(); i++)
        for (size_t j = i + 1; j < boxes.size(); j++)
            if (text::overlaps(boxes[i], boxes[j], 2.0f)) clean = false;
    CHECK(clean);
}

void testTextLayout() {
    using namespace text;
    std::string s = "sin(sqrt(x^2 + y^2))/(1 + 0.05(x^2 + y^2))";
    auto starts = wrapLines(s, 16);
    CHECK(starts.size() >= 3);
    CHECK(starts[0] == 0);
    for (size_t i = 0; i < starts.size(); i++) {
        size_t end = i + 1 < starts.size() ? starts[i + 1] : s.size();
        CHECK(charCount(s, starts[i], end) <= 16);
    }
    // Breaks prefer operators or spaces: no line except the last ends in the middle of "sqrt"
    CHECK(s.substr(starts[1] - 1, 1).find_first_of(" +-*/,^)") != std::string::npos);
    CHECK(wrapLines("short", 40).size() == 1);
    CHECK(wrapLines("", 10).size() == 1);
    // A long word without break characters is split hard
    auto hard = wrapLines("abcdefghijklmnop", 5);
    CHECK(hard.size() == 4 && hard[1] == 5);
    // Cursor <-> line/column round trip
    size_t line = 0, col = 0;
    cursorToLineCol(s, starts, starts[1] + 2, line, col);
    CHECK(line == 1 && col == 2);
    CHECK(lineColToCursor(s, starts, 1, 2) == starts[1] + 2);
    CHECK(lineColToCursor(s, starts, 0, 999) < starts[1]);       // clamped to the line
    CHECK(lineColToCursor(s, starts, 99, 0) == starts.back());    // clamped to the last line
    cursorToLineCol(s, starts, s.size(), line, col);
    CHECK(line == starts.size() - 1);
    // UTF-8: never split a character
    std::string u = "a\xCF\x80" "b";  // a pi b
    CHECK(nextChar(u, 1) == 3 && prevChar(u, 3) == 1);
    CHECK(charCount(u, 0, u.size()) == 3);
}

void testContour() {
    // Circle x^2 + y^2 = 4 on a grid: points lie on the circle, segments chain into one closed line
    int n = 40;
    std::vector<double> v((n + 1) * (n + 1));
    for (int j = 0; j <= n; j++)
        for (int i = 0; i <= n; i++) {
            double x = -3 + 6.0 * i / n, y = -3 + 6.0 * j / n;
            v[j * (n + 1) + i] = x * x + y * y - 4;
        }
    auto segs = marchingSquares(v, n, n, -3, 3, -3, 3);
    CHECK(segs.size() > 20);
    for (const auto& sg : segs) CHECK_NEAR(std::hypot(sg.x0, sg.y0), 2.0, 0.05);
    auto lines = chainSegments(segs, 1e-6);
    CHECK(lines.size() == 1);
    if (!lines.empty()) {
        CHECK(lines[0].size() == segs.size() + 1);
        CHECK_NEAR(lines[0].front().first, lines[0].back().first, 1e-9);  // closed
    }
}

void testPdfCompile() {
    // Error lines of a LaTeX log: "!" lines and where they happened
    std::string log = "This is pdfTeX\n(./figure.tex\n! Undefined control sequence.\nl.12 \\foo\n  \n"
                      "! Emergency stop.\n<*> figure.tex\n";
    std::string errors = latex::firstLatexErrors(log);
    CHECK(errors.find("! Undefined control sequence.") == 0);
    CHECK(errors.find("l.12 \\foo") != std::string::npos);
    CHECK(errors.find("! Emergency stop.") != std::string::npos);
    CHECK(latex::firstLatexErrors(log, 1) == "! Undefined control sequence.");
    CHECK(latex::firstLatexErrors("all fine\n").empty());

    // Tools in PATH, no shell
    CHECK(!latex::findTool("sh").empty());
    CHECK(latex::findTool("surely-not-a-command-axismundi").empty());
    latex::PdfResult none = latex::compilePdf("x", "/tmp/never.pdf", 5, "");
    CHECK(!none.ok);
    CHECK(none.message.find("No LaTeX compiler") != std::string::npos);

    // A real compile (slow): PDF_TEST=1 make test, with pdflatex installed
    const char* run = std::getenv("PDF_TEST");
    if (!run || std::string(run) != "1" || latex::latexCompiler().empty()) return;
    std::string out = (std::filesystem::temp_directory_path() / "axismundi-test figure.pdf").string();
    latex::PdfResult ok = latex::compilePdf(
        "\\documentclass{standalone}\n\\usepackage{tikz}\n\\begin{document}\n"
        "\\tikz \\draw (0,0) -- (1,1);\n\\end{document}\n", out);
    CHECK(ok.ok);
    std::ifstream pdf(out, std::ios::binary);
    char head[5] = {};
    pdf.read(head, 4);
    CHECK(std::string(head) == "%PDF");
    std::filesystem::remove(out);
    latex::PdfResult bad = latex::compilePdf("\\documentclass{standalone}\n\\begin{document}\\nosuchmacro\n"
                                             "\\end{document}\n", out);
    CHECK(!bad.ok);
    CHECK(bad.message.find("!") == 0);
    CHECK(!std::filesystem::exists(out));
    std::printf("  (compiled a PDF with %s)\n", latex::latexCompiler().c_str());
}

void testLatexExport() {
    using namespace scene;
    CHECK(latex::nameToLatex("theta") == "\\theta");
    CHECK(latex::nameToLatex("z1") == "z_{1}");
    CHECK(latex::nameToLatex("Ma") == "\\mathit{Ma}");
    CHECK(latex::nameToLatex("a_b") == "a_{b}");
    CHECK(latex::nameToLatex("v12") == "v_{12}");
    CHECK(latex::nameToLatex("alpha2") == "\\alpha_{2}");

    double az = 0, el = 0;
    latex::viewFromDirection(Vector3D(0, -1, 0), az, el);  // viewer in front (negative y)
    CHECK_NEAR(az, 0.0, 1e-9);
    CHECK_NEAR(el, 0.0, 1e-9);
    latex::viewFromDirection(Vector3D(0, 0, 1), az, el);   // from above
    CHECK_NEAR(el, 90.0, 1e-9);
    latex::viewFromDirection(Vector3D(1, 0, 0), az, el);   // from +x
    CHECK_NEAR(az, 90.0, 1e-9);

    CHECK(std::string(latex::styleFor(0).dash) == "solid");
    CHECK(std::string(latex::styleFor(1).dash) == "dashed");
    CHECK(std::string(latex::styleFor(5).gray) == "black!65");

    const char* outDir = std::getenv("LATEX_OUT");
    auto save = [&](const char* name, const std::string& code) {
        if (!outDir) return;
        std::ofstream(std::string(outDir) + "/" + name + ".tex") << code;
    };
    auto noColors = [](const std::string& code) {
        for (const char* c : {"red", "blue", "green", "yellow", "orange", "cyan", "magenta"})
            if (code.find(c) != std::string::npos) return false;
        return true;
    };

    // Vector fields: quiver tables; a plane field is a 2D figure, a volume field a 3D one
    {
        Scene fsc;
        fsc.clearAll();
        auto* F = static_cast<VectorFieldObject*>(fsc.add(SpaceKind::Euclidean3D, "field", "F"));
        F->setField("F", "[-y - x/4, x - y/4, 0]");
        F->setProp("lines", true);
        F->setProp("particles", true);
        fsc.evaluate();
        latex::Options fo;
        fo.standalone = true;
        latex::Result fr = latex::exportLatex(fsc.space(SpaceKind::Euclidean3D), fsc.selection(SpaceKind::Euclidean3D), fo);
        CHECK(fr.exported == 1 && fr.flat);
        CHECK(fr.code.find("quiver={u=\\thisrow{u}, v=\\thisrow{v}}") != std::string::npos);
        CHECK(fr.code.find("table[row sep=crcr] {x y u v\\\\") != std::string::npos);
        CHECK(fr.code.find("forget plot") != std::string::npos);  // flow lines
        CHECK(fr.code.find("only marks") != std::string::npos);   // particles
        CHECK(fr.code.find("axis equal image") != std::string::npos);
        CHECK(noColors(fr.code));
        save("field-plane", fr.code);
        F->setProp("domain", "volume");
        F->setField("F", "[-y, x, 0.4]");
        F->setProp("particles", false);
        fsc.evaluate();
        fr = latex::exportLatex(fsc.space(SpaceKind::Euclidean3D), fsc.selection(SpaceKind::Euclidean3D), fo);
        CHECK(!fr.flat);
        CHECK(fr.code.find("\\addplot3[") != std::string::npos);
        CHECK(fr.code.find("w=\\thisrow{w}") != std::string::npos);
        CHECK(fr.code.find("only marks") == std::string::npos);
        save("field-volume", fr.code);
    }

    // Quaternions: a 3D figure with the axis, the arc and the moved basis; a turn about z with no basis
    // is flat
    {
        Scene qsc;
        qsc.clearAll();
        auto* q = static_cast<QuaternionObject*>(qsc.add(SpaceKind::Euclidean3D, "quaternion", "q"));
        q->setField("value", "qaxis([1, 1, 1], 2pi/3)");
        q->setProp("act", true);
        q->setField("v", "[1.5, 0, 0.5]");
        qsc.evaluate();
        latex::Options qo;
        qo.standalone = true;
        latex::Result qr = latex::exportLatex(qsc.space(SpaceKind::Euclidean3D), qsc.selection(SpaceKind::Euclidean3D), qo);
        CHECK(qr.exported == 1 && !qr.flat);
        CHECK(qr.code.find("q\\,e_{3}") != std::string::npos);      // moved basis
        CHECK(qr.code.find("densely dashed") != std::string::npos);    // the axis
        CHECK(qr.code.find("\\vec v") != std::string::npos);         // acting on a vector
        CHECK(noColors(qr.code));
        save("quaternion-3d", qr.code);
        q->setField("value", "qaxis([0, 0, 1], pi/2)");
        q->setProp("basis", false);
        q->setField("v", "[1.5, 0, 0]");
        qsc.evaluate();
        qr = latex::exportLatex(qsc.space(SpaceKind::Euclidean3D), qsc.selection(SpaceKind::Euclidean3D), qo);
        CHECK(qr.flat);
        save("quaternion-flat", qr.code);
    }

    // Integrals: boxes and the value; a flat work integral is a 2D figure
    {
        Scene isc;
        isc.clearAll();
        auto* V = isc.add(SpaceKind::Euclidean3D, "doubleintegral", "V");
        V->setField("n", "4");
        isc.evaluate();
        latex::Options io;
        io.standalone = true;
        latex::Result ir = latex::exportLatex(isc.space(SpaceKind::Euclidean3D), isc.selection(SpaceKind::Euclidean3D), io);
        CHECK(ir.exported == 1 && !ir.flat);
        CHECK(ir.code.find("\\fill[black!10]") != std::string::npos);
        CHECK(ir.code.find("V \\approx") != std::string::npos);
        save("integral-volume", ir.code);
        isc.clearAll();
        isc.add(SpaceKind::Euclidean3D, "lineintegral", "W");
        isc.evaluate();
        ir = latex::exportLatex(isc.space(SpaceKind::Euclidean3D), isc.selection(SpaceKind::Euclidean3D), io);
        CHECK(ir.flat && ir.code.find("W = 25.1") != std::string::npos);
        save("integral-work", ir.code);
    }

    // Default 3D scene: vectors, matrix, eigenvectors -> 3D axis with arrows and labels
    Scene sc;
    sc.evaluate();
    latex::Options opt;
    latex::Result r = latex::exportLatex(sc.space(SpaceKind::Euclidean3D), sc.selection(SpaceKind::Euclidean3D), opt);
    CHECK(!r.flat);
    CHECK(r.exported >= 7);
    CHECK(r.code.find("\\begin{tikzpicture}") != std::string::npos);
    CHECK(r.code.find("view={25}{30}") != std::string::npos);
    CHECK(r.code.find("\\vec{a}") != std::string::npos);
    CHECK(r.code.find("\\lambda") != std::string::npos);
    CHECK(r.code.find("\\usepackage{pgfplots}") != std::string::npos);  // preamble in a comment
    CHECK(r.code.find("\\documentclass") == std::string::npos);
    CHECK(noColors(r.code));
    opt.standalone = true;
    save("scene3d", latex::exportLatex(sc.space(SpaceKind::Euclidean3D), sc.selection(SpaceKind::Euclidean3D), opt).code);

    // Only flat objects -> 2D axis; selection scope
    Scene flat;
    Space& d = flat.space(SpaceKind::Euclidean3D);
    d.clear();
    auto* fn = flat.add(SpaceKind::Euclidean3D, "function", "f");
    fn->setField("f", "sin(x)");
    auto* im = flat.add(SpaceKind::Euclidean3D, "implicit", "g");
    im->setField("f", "x^2/9 + y^2/4 - 1");
    auto* ve = static_cast<VectorObject*>(flat.add(SpaceKind::Euclidean3D, "vector", "v"));
    ve->setField("x", "2");
    ve->setField("y", "1");
    flat.add(SpaceKind::Euclidean3D, "parameter", "k");
    flat.evaluate();
    r = latex::exportLatex(d, flat.selection(SpaceKind::Euclidean3D), latex::Options{});
    CHECK(r.flat);
    CHECK(r.exported == 3);
    CHECK(r.skipped.size() == 1);  // the parameter
    CHECK(r.code.find("\\addlegendentry{$y = f(x)$}") != std::string::npos);
    CHECK(r.code.find("view=") == std::string::npos);
    CHECK(r.code.find("solid") != std::string::npos && r.code.find("dashed") != std::string::npos);
    latex::Options sopt;
    sopt.standalone = true;
    sopt.grid = true;
    save("flat", latex::exportLatex(d, flat.selection(SpaceKind::Euclidean3D), sopt).code);
    flat.selection(SpaceKind::Euclidean3D).set(ve->id());
    latex::Options selOpt;
    selOpt.scope = latex::Scope::Selection;
    r = latex::exportLatex(d, flat.selection(SpaceKind::Euclidean3D), selOpt);
    CHECK(r.exported == 1);
    CHECK(latex::styleIndexOf(d, flat.selection(SpaceKind::Euclidean3D), latex::Scope::Visible, ve->id()) == 2);

    // Surface: mesh rows match the samples
    Scene surf;
    surf.space(SpaceKind::Euclidean3D).clear();
    auto* sf = surf.add(SpaceKind::Euclidean3D, "surface", "S");
    sf->setField("f", "sin(sqrt(x^2 + y^2))");
    surf.evaluate();
    latex::Options so;
    so.surfaceSamples = 20;
    r = latex::exportLatex(surf.space(SpaceKind::Euclidean3D), surf.selection(SpaceKind::Euclidean3D), so);
    CHECK(!r.flat);
    CHECK(r.code.find("mesh/rows=21") != std::string::npos);
    CHECK(r.code.find("faceted color=black") != std::string::npos);
    so.standalone = true;
    save("surface", latex::exportLatex(surf.space(SpaceKind::Euclidean3D), surf.selection(SpaceKind::Euclidean3D), so).code);
    so.grayShading = true;
    save("surface_gray", latex::exportLatex(surf.space(SpaceKind::Euclidean3D), surf.selection(SpaceKind::Euclidean3D), so).code);

    // Complex plane: always 2D, unit circle, labels as math
    r = latex::exportLatex(sc.space(SpaceKind::Complex), sc.selection(SpaceKind::Complex), latex::Options{});
    CHECK(r.flat);
    CHECK(r.code.find("z_{1}") != std::string::npos);
    CHECK(r.code.find("unit circle") != std::string::npos);
    latex::Options co;
    co.standalone = true;
    save("complex", latex::exportLatex(sc.space(SpaceKind::Complex), sc.selection(SpaceKind::Complex), co).code);

    // A curve in 3D
    Scene cv;
    cv.space(SpaceKind::Euclidean3D).clear();
    cv.add(SpaceKind::Euclidean3D, "curve", "c");
    cv.evaluate();
    latex::Options cvo;
    cvo.standalone = true;
    save("curve", latex::exportLatex(cv.space(SpaceKind::Euclidean3D), cv.selection(SpaceKind::Euclidean3D), cvo).code);
}

void testReference() {
    using namespace expr;
    // Every example compiles; those that need no objects evaluate without errors
    Slots slots;
    slots.x = 1;
    slots.y = 2;
    slots.t = 0.5;
    slots.u = 0.5;
    slots.z = 3;
    slots.hasX = slots.hasY = slots.hasT = slots.hasU = slots.hasZ = true;
    std::string documented;
    int examples = 0;
    for (const auto& section : reference()) {
        CHECK(!section.title.empty() && !section.entries.empty());
        for (const auto& e : section.entries) {
            documented += " " + e.syntax + " ";
            if (e.example.empty()) continue;
            examples++;
            Program p;
            bool ok = p.compile(e.example, kVarU | kVarZ);
            if (!ok) fprintf(stderr, "  reference example does not compile: %s (%s)\n", e.example.c_str(), p.error().c_str());
            CHECK(ok);
            if (!ok || e.usesObjects) continue;
            Value v = p.eval(nullptr, &slots);
            if (!v.ok()) fprintf(stderr, "  reference example fails: %s -> %s\n", e.example.c_str(), v.error.c_str());
            CHECK(v.ok());
        }
    }
    CHECK(examples > 40);
    // Every built-in function is documented somewhere
    auto isWordChar = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };
    for (const auto& name : functionNames()) {
        bool found = false;
        for (size_t at = documented.find(name); at != std::string::npos && !found; at = documented.find(name, at + 1)) {
            bool before = at > 0 && isWordChar(documented[at - 1]);
            bool after = at + name.size() < documented.size() && isWordChar(documented[at + name.size()]);
            found = !before && !after;
        }
        if (!found) fprintf(stderr, "  function not in the reference: %s\n", name.c_str());
        CHECK(found);
    }
    // A few results the text promises
    auto run = [&](const char* text) {
        Program p;
        p.compile(text, true);
        return p.eval(nullptr, &slots);
    };
    CHECK_NEAR(run("2^3^2").re, 512.0, 1e-9);
    CHECK_NEAR(run("-2^2").re, -4.0, 1e-12);
    CHECK_NEAR(run("[1, 2, 3].y").re, 2.0, 1e-12);
    CHECK_NEAR(run("atan2(1, -1)").re, 3 * 3.14159265358979 / 4, 1e-9);
    CHECK_NEAR(run("keys(0.25, 0, 10, 0)").re, 5.0, 1e-9);
}

void testSceneTime() {
    using namespace scene;
    Scene sc;
    Space& d = sc.space(SpaceKind::Euclidean3D);
    d.clear();
    // t is the scene time in any formula
    auto* v = static_cast<VectorObject*>(sc.add(SpaceKind::Euclidean3D, "vector", "v"));
    v->setByFormula(true);
    v->setField("value", "[cos(t), sin(t), 1]");
    sc.setTime(0.0);
    sc.evaluate();
    CHECK(v->error().empty());
    CHECK_NEAR(v->value().vector.x, 1.0f, 1e-6f);
    sc.timeline().length = 10.0;
    sc.setTime(3.14159265 / 2);
    sc.evaluate();
    CHECK_NEAR(v->value().vector.y, 1.0f, 1e-5f);
    auto* m = static_cast<MatrixObject*>(sc.add(SpaceKind::Euclidean3D, "matrix", "R"));
    m->setByFormula(true);
    m->setField("value", "rotz(t)");
    sc.evaluate();
    CHECK(m->error().empty());

    // Plots using t change with the time; plots without t do not
    auto* wave = sc.add(SpaceKind::Euclidean3D, "function", "w");
    wave->setField("f", "sin(x - t)");
    auto* still = sc.add(SpaceKind::Euclidean3D, "function", "g");
    still->setField("f", "sin(x)");
    sc.evaluate();
    std::uint64_t w0 = wave->changedAt(), g0 = still->changedAt();
    sc.evaluate();  // same time: nothing changes
    CHECK(wave->changedAt() == w0);
    sc.setTime(2.0);
    sc.evaluate();
    CHECK(wave->changedAt() > w0);
    CHECK(still->changedAt() == g0);
    CHECK(wave->error().empty());

    // Curves: u is their own variable, t the scene time
    auto* c = static_cast<PlotObject*>(sc.add(SpaceKind::Euclidean3D, "curve", "c"));
    c->setField("x", "cos(u + t)");
    sc.evaluate();
    CHECK(c->error().empty());
    CHECK(c->field("x")->program.usesU() && c->field("x")->program.usesT());
    CHECK(c->field("umin") && !c->field("tmin"));

    // An object named u stays a normal name (and a dependency) outside curves
    auto* u = static_cast<VectorObject*>(sc.add(SpaceKind::Euclidean3D, "vector", "u"));
    u->setField("x", "2");
    auto* w = static_cast<VectorObject*>(sc.add(SpaceKind::Euclidean3D, "vector", "q"));
    w->setByFormula(true);
    w->setField("value", "3u");
    sc.evaluate();
    CHECK(w->error().empty());
    CHECK(w->dependencies().count("u") == 1);
    CHECK_NEAR(w->value().vector.x, 6.0f, 1e-6f);
    CHECK(c->dependencies().count("u") == 0);  // inside the curve, u is the curve variable

    // Old files: curves with t and tmin/tmax become u and umin/umax
    SceneData data;
    sc.save(data);
    std::string cp;
    for (auto& [key, val] : data)
        if (val == "c" && key.size() > 5 && key.compare(key.size() - 5, 5, ".name") == 0) cp = key.substr(0, key.size() - 4);
    CHECK(!cp.empty());
    data.erase(cp + "field.umin");
    data.erase(cp + "field.umax");
    data[cp + "field.tmin"] = "-1";
    data[cp + "field.tmax"] = "pi";
    data[cp + "field.x"] = "cos(t) + 2t";
    data[cp + "field.y"] = "tan(t)";
    Scene old;
    std::string err;
    CHECK(old.load(data, err));
    auto* c2 = old.space(SpaceKind::Euclidean3D).find("c");
    CHECK(c2 && c2->field("x")->text == "cos(u) + 2u" && c2->field("y")->text == "tan(u)");
    CHECK(c2 && c2->field("umin")->text == "-1" && c2->field("umax")->text == "pi");
    CHECK(c2 && c2->error().empty());

    // Engine messages
    expr::Program p;
    CHECK(p.compile("t + 1"));
    CHECK(!p.eval(nullptr).ok());
    CHECK(p.compile("u", true) && p.usesU() && p.names().empty());
    CHECK(p.compile("u") && !p.usesU() && p.names().count("u"));
}

void testScene() {
    using namespace scene;
    Scene sc;
    Space& d = sc.space(SpaceKind::Euclidean3D);
    sc.evaluate();

    // Default picture: derived vectors follow their inputs
    const SceneObject* c = d.find("c");
    CHECK(c && c->value().type == expr::Type::Vector);
    if (c) CHECK_NEAR(c->value().vector.z, 1.0f, 1e-6f);
    d.find("b")->setField("y", "2");
    sc.evaluate();
    CHECK_NEAR(d.find("c")->value().vector.z, 2.0f, 1e-6f);
    CHECK_NEAR(d.find("s")->value().vector.y, 2.0f, 1e-6f);

    // Formula matrix driven by a parameter
    auto* theta = static_cast<ParameterObject*>(sc.add(SpaceKind::Euclidean3D, "parameter", "theta"));
    auto* m = static_cast<MatrixObject*>(d.find("M"));
    m->setByFormula(true);
    m->setField("value", "rotz(theta)");
    theta->setNumber(3.14159265 / 2);
    sc.evaluate();
    CHECK_NEAR(d.find("Ma")->value().vector.y, 1.0f, 1e-5f);
    std::uint64_t before = d.find("Ma")->changedAt();
    theta->setMode(ParameterObject::Mode::Animate);
    sc.setTime(0.5);
    sc.evaluate();
    CHECK(d.find("Ma")->changedAt() > before);  // change propagated

    // Plot detail per unit
    {
        Scene ps;
        Space& e = ps.space(SpaceKind::Euclidean3D);
        auto* f = static_cast<PlotObject*>(ps.add(SpaceKind::Euclidean3D, "function", "f"));
        f->setField("xmin", "-5");
        f->setField("xmax", "5");
        f->setDensity(30);
        ps.evaluate();
        CHECK(f->samples().nx == 300);
        f->setField("xmax", "15");  // twice the range, twice the samples
        ps.evaluate();
        CHECK(f->samples().nx == 600);
        CHECK(!f->samples().capped);
        f->setField("xmax", "2000");
        ps.evaluate();
        CHECK(f->samples().capped && f->samples().nx == PlotObject::densityRange(PlotObject::Kind::Function).cap);
        auto* sf = static_cast<PlotObject*>(ps.add(SpaceKind::Euclidean3D, "surface", "S"));
        sf->setField("xmin", "-2");
        sf->setField("xmax", "2");
        sf->setField("ymin", "0");
        sf->setField("ymax", "1");
        sf->setDensity(10);
        ps.evaluate();
        CHECK(sf->samples().nx == 40 && sf->samples().ny == 10);
        sf->setDensity(1000);  // clamped to the slider range
        CHECK_NEAR(sf->density(), PlotObject::densityRange(PlotObject::Kind::Surface).max, 1e-9);
        // Old files: resolution 60 on a surface over 12 units -> 5 cells per unit
        SceneData data;
        sf->setField("xmin", "-6");
        sf->setField("xmax", "6");
        sf->setField("ymin", "-6");
        sf->setField("ymax", "6");
        ps.evaluate();
        ps.save(data);
        for (auto it = data.begin(); it != data.end();)
            it = it->first.find(".prop.density") != std::string::npos ? data.erase(it) : std::next(it);
        for (auto& [key, v] : data)
            if (v == "S" && key.size() > 5 && key.compare(key.size() - 5, 5, ".name") == 0)
                data[key.substr(0, key.size() - 4) + "prop.resolution"] = "60";
        Scene old;
        std::string err;
        CHECK(old.load(data, err));
        auto* s2 = static_cast<PlotObject*>(old.space(SpaceKind::Euclidean3D).find("S"));
        CHECK(s2 && !s2->props().count("resolution"));
        if (s2) CHECK_NEAR(s2->density(), 5.0, 1e-9);
        (void)e;
    }

    // Integral objects: values, boxes, partial work, and the value cache
    {
        Scene is;
        is.clearAll();
        auto* V = static_cast<DoubleIntegralObject*>(is.add(SpaceKind::Euclidean3D, "doubleintegral", "V"));
        V->setField("f", "4 - x^2 - y^2");
        for (auto [k, v] : {std::pair{"xmin", "-1"}, std::pair{"xmax", "1"}, std::pair{"ymin", "-1"}, std::pair{"ymax", "1"}})
            V->setField(k, v);
        auto* k = static_cast<ParameterObject*>(is.add(SpaceKind::Euclidean3D, "parameter", "k"));
        k->setNumber(4);
        V->setField("n", "k");
        is.evaluate();
        CHECK(V->error().empty());
        CHECK_NEAR(V->value().re, 40.0 / 3.0, 1e-6);
        CHECK(V->riemann().boxes.size() == 16);
        CHECK(std::fabs(V->riemann().sum - 40.0 / 3.0) < 0.3);
        std::uint64_t version = V->geometryVersion();
        is.evaluate();
        CHECK(V->geometryVersion() == version);  // nothing changed: not recomputed
        k->setNumber(6);  // within the slider's default range 0..2π
        is.evaluate();
        CHECK(V->geometryVersion() != version && V->riemann().boxes.size() == 36);
        V->setField("xmin", "3");
        is.evaluate();
        CHECK(V->error().find("must be less than") != std::string::npos);
        V->setField("xmin", "-1");
        V->setField("f", "1/x");  // undefined at x = 0 on the precise grid
        is.evaluate();
        CHECK(V->error().find("undefined") != std::string::npos);

        auto* W = static_cast<LineIntegralObject*>(is.add(SpaceKind::Euclidean3D, "lineintegral", "W"));
        is.evaluate();
        CHECK(W->error().empty());
        CHECK_NEAR(W->value().re, 8 * 3.14159265358979, 1e-5);  // ∮ [−y, x, 0] on the circle of radius 2
        W->setField("upto", "0.25");
        is.evaluate();
        CHECK_NEAR(W->value().re, 2 * 3.14159265358979, 1e-5);
        CHECK_NEAR(W->total(), 8 * 3.14159265358979, 1e-5);
        CHECK(!W->path().empty() && W->maxAlong() > 1.9);
        CHECK(std::fabs(W->marker().y - 2.0) < 1e-6);  // a quarter of the way round: (0, 2, 0)
        // Following the clock re-evaluates
        W->setField("upto", "mod(t/4, 1)");
        is.timeline().time = 1.0;
        is.evaluate();
        CHECK_NEAR(W->value().re, 2 * 3.14159265358979, 1e-5);
    }

    // Quaternion objects: their value feeds other objects; the acts-on field only when switched on
    {
        Scene qs;
        qs.clearAll();
        auto* q = static_cast<QuaternionObject*>(qs.add(SpaceKind::Euclidean3D, "quaternion", "q"));
        CHECK(q && q->name() == "q");
        auto* w = static_cast<VectorObject*>(qs.add(SpaceKind::Euclidean3D, "vector", "w"));
        w->setProp("mode", "formula");
        w->setField("value", "rotate(q, [1, 0, 0])");
        auto* M = qs.add(SpaceKind::Euclidean3D, "matrix", "M");
        M->setProp("mode", "formula");
        M->setField("value", "mat(q)");
        qs.evaluate();
        CHECK(q->error().empty() && q->value().type == expr::Type::Quaternion);
        CHECK(w->error().empty());
        CHECK_NEAR(w->value().vector.y, 1.0, 1e-6);  // a quarter turn about z sends x to y
        CHECK(M->error().empty() && std::fabs(M->value().matrix.m[1][0] - 1.0f) < 1e-6f);
        expr::Rotation r;
        CHECK(expr::rotationOf(q->value(), r) && r.hasAxis && std::fabs(r.angle - 3.14159265358979 / 2) < 1e-9);
        // "v" is inactive until "acts on" is switched on (a bad v is then reported)
        q->setField("v", "[1, 2]");
        qs.evaluate();
        CHECK(q->error().empty());
        q->setProp("act", true);
        qs.evaluate();
        CHECK(!q->error().empty());
        q->setField("v", "[0, 1, 0]");
        q->setField("value", "[1, 0, 0]");  // a vector is not a quaternion
        qs.evaluate();
        CHECK(q->error().find("should be a quaternion") != std::string::npos);
        // Following the clock
        q->setField("value", "qaxis([0, 0, 1], t)");
        qs.timeline().time = 1.0;
        qs.evaluate();
        CHECK(q->followsClock());
        CHECK(expr::rotationOf(q->value(), r) && std::fabs(r.angle - 1.0) < 1e-9);
    }

    // Vector fields: formula of x, y, z using other objects, counts, ranges, saving
    {
        Scene fs;
        auto* k = static_cast<ParameterObject*>(fs.add(SpaceKind::Euclidean3D, "parameter", "k"));
        k->setField("value", "2");
        auto* F = static_cast<VectorFieldObject*>(fs.add(SpaceKind::Euclidean3D, "field", "F"));
        CHECK(F && F->name() == "F");
        F->setField("F", "k*[-y, x, z]");
        fs.evaluate();
        CHECK(F->error().empty());
        CHECK(F->field("F")->program.usesZ());
        CHECK(F->dependencies().count("k") == 1);
        VectorFieldObject::Counts c = F->counts();
        CHECK(c.nx == 8 && c.ny == 8 && c.nz == 1);  // plane, 1 arrow per unit over -4..4
        CHECK(!F->followsClock());
        F->setProp("particles", "1");
        CHECK(F->followsClock());
        // z range only matters in Volume
        F->setField("zmin", "3");
        fs.evaluate();
        CHECK(F->error().empty());
        F->setProp("domain", "volume");
        fs.evaluate();
        CHECK(F->error().find("must be less than") != std::string::npos);
        F->setField("zmin", "-2");
        F->setProp("density", "100");  // capped
        fs.evaluate();
        CHECK(F->error().empty());
        c = F->counts();
        CHECK(c.capped && c.nx == VectorFieldObject::kVolumeCap && c.nz == VectorFieldObject::kVolumeCap);
        F->setField("F", "[1, 2]");  // not a vector of three? [1, 2] is invalid
        fs.evaluate();
        CHECK(!F->error().empty());
        F->setField("F", "x + y");  // a number, not a vector
        fs.evaluate();
        CHECK(F->error().find("should be a vector") != std::string::npos);
        F->setField("F", "[-y, x, z]");
        fs.evaluate();
        SceneData data;
        fs.save(data);
        Scene back;
        std::string err;
        CHECK(back.load(data, err));
        auto* G = dynamic_cast<VectorFieldObject*>(back.space(SpaceKind::Euclidean3D).find("F"));
        CHECK(G && G->volume() && G->particles());
        if (G) {
            back.evaluate();
            CHECK(G->error().empty() && G->field("F")->program.usesZ());
        }
    }

    // Parameter modes driven by the scene clock
    {
        Scene ps;
        auto* k = static_cast<ParameterObject*>(ps.add(SpaceKind::Euclidean3D, "parameter", "k"));
        auto* q = static_cast<ParameterObject*>(ps.add(SpaceKind::Euclidean3D, "parameter", "q"));
        // Formula of the scene time, using another (static) parameter
        q->setMode(ParameterObject::Mode::Formula);
        q->setField("formula", "2t + k");
        k->setNumber(1.0);
        ps.setTime(1.5);
        ps.evaluate();
        CHECK(q->error().empty());
        CHECK_NEAR(q->number(), 4.0, 1e-9);
        CHECK_NEAR(q->value().re, 4.0, 1e-9);
        ps.setTime(0.5);  // scrubbing back gives the earlier value
        ps.evaluate();
        CHECK_NEAR(q->number(), 2.0, 1e-9);
        // A slider is static: time does not change it, and its inactive formula is ignored
        k->setField("formula", "nonsense(");
        ps.setTime(3.0);
        ps.evaluate();
        CHECK(k->error().empty());
        CHECK_NEAR(k->number(), 1.0, 1e-12);
        // Animate: a pure function of time with easing, repeat and a start delay
        k->setMode(ParameterObject::Mode::Animate);
        k->setProp("min", 2.0);
        k->setProp("max", 4.0);
        k->setProp("anim.start", 1.0);
        Playback pb = k->playback();
        pb.duration = 2.0f;
        pb.repeat = Repeat::PingPong;
        pb.easing = Easing::EaseInOut;
        k->setPlayback(pb);
        CHECK_NEAR(k->animatedValue(0.5), 2.0, 1e-9);  // before its start
        CHECK_NEAR(k->animatedValue(2.0), 3.0, 1e-5);  // half way: ease-in-out(0.5) = 0.5
        CHECK_NEAR(k->animatedValue(3.0), 4.0, 1e-5);  // the end
        CHECK_NEAR(k->animatedValue(4.0), 3.0, 1e-5);  // half way back
        ps.setTime(2.0);
        ps.evaluate();
        CHECK_NEAR(k->value().re, 3.0, 1e-5);
        CHECK_NEAR(q->number(), 7.0, 1e-5);  // 2t + k follows k
        // Automatic length: start 1 s + ping-pong 2 x 2 s
        CHECK_NEAR(ps.autoLength(), 5.0, 1e-9);
        // The clock: loop wraps, otherwise it stops at the end
        ps.setTime(0.0);
        ps.timeline().playing = true;
        ps.advance(6.0);
        CHECK_NEAR(ps.timeline().time, 1.0, 1e-9);
        ps.timeline().loop = false;
        ps.advance(10.0);
        CHECK_NEAR(ps.timeline().time, 5.0, 1e-9);
        CHECK(!ps.timeline().playing);
        ps.timeline().length = 8.0;
        CHECK_NEAR(ps.timelineLength(), 8.0, 1e-12);
        // Back to a slider keeps the current value
        ps.setTime(2.0);
        ps.evaluate();
        k->setMode(ParameterObject::Mode::Slider);
        CHECK_NEAR(k->number(), 3.0, 1e-5);
        k->setMode(ParameterObject::Mode::Animate);
        // Undo snapshots leave the clock alone
        ps.commit();
        double tNow = ps.timeline().time;
        k->setProp("max", 9.0);
        ps.commit();
        ps.setTime(1.0);
        ps.undo();
        CHECK_NEAR(ps.timeline().time, 1.0, 1e-12);
        CHECK_NEAR(static_cast<ParameterObject*>(ps.space(SpaceKind::Euclidean3D).find("k"))->maximum(), 4.0, 1e-12);
        (void)tNow;
        // Round trip keeps modes, formula, style and the clock
        SceneData data;
        ps.timeline().playing = true;
        ps.save(data);
        Scene back;
        std::string err;
        CHECK(back.load(data, err));
        auto* q2 = static_cast<ParameterObject*>(back.space(SpaceKind::Euclidean3D).find("q"));
        auto* k2 = static_cast<ParameterObject*>(back.space(SpaceKind::Euclidean3D).find("k"));
        CHECK(q2 && q2->mode() == ParameterObject::Mode::Formula && q2->field("formula")->text == "2t + k");
        CHECK(k2 && k2->playback().easing == Easing::EaseInOut && k2->playback().repeat == Repeat::PingPong);
        CHECK(k2 && std::fabs(k2->startDelay() - 1.0) < 1e-9);
        CHECK(back.timeline().playing && std::fabs(back.timeline().length - 8.0) < 1e-6);
        // Old files: own clocks (speed + loop + playing), no mode and no timeline
        for (auto it = data.begin(); it != data.end();) {
            bool drop = it->first.find(".prop.mode") != std::string::npos ||
                        it->first.find(".prop.anim.") != std::string::npos || it->first.rfind("time.", 0) == 0;
            it = drop ? data.erase(it) : std::next(it);
        }
        std::string kp;
        for (auto& [key, v] : data)
            if (v == "k" && key.size() > 5 && key.compare(key.size() - 5, 5, ".name") == 0) kp = key.substr(0, key.size() - 4);
        CHECK(!kp.empty());
        data[kp + "prop.speed"] = "0.25";
        data[kp + "prop.loop"] = "0";
        data[kp + "prop.playing"] = "1";
        Scene old;
        CHECK(old.load(data, err));
        auto* k3 = static_cast<ParameterObject*>(old.space(SpaceKind::Euclidean3D).find("k"));
        CHECK(k3 && k3->mode() == ParameterObject::Mode::Animate);
        if (k3) {
            CHECK_NEAR(k3->playback().duration, 4.0f, 1e-4f);
            CHECK(k3->playback().repeat == Repeat::Once);
            CHECK(!k3->props().count("playing") && !k3->props().count("speed"));
        }
        CHECK(old.timeline().playing);
    }

    // Unique names and rename with reference rewriting
    SceneObject* v = sc.add(SpaceKind::Euclidean3D, "vector", "a");
    CHECK(v->name() == "a2");
    std::string err;
    CHECK(!d.rename(v->id(), "sin", err));
    CHECK(!d.rename(v->id(), "b", err));
    CHECK(d.rename(d.find("a")->id(), "p", err));
    CHECK(d.find("s")->field("value")->text == "p + b");
    CHECK(d.find("c")->field("value")->text == "cross(p, b)");
    sc.evaluate();
    CHECK(d.find("s")->error().empty());
    CHECK(renameInText("2a + ab + a.x + b_a", "a", "q") == "2q + ab + q.x + b_a");

    // Missing names and cycles
    d.remove(d.find("b")->id());
    sc.evaluate();
    CHECK(d.find("s")->error().find("'b'") != std::string::npos);
    SceneObject* u = sc.add(SpaceKind::Euclidean3D, "vector", "u");
    SceneObject* w = sc.add(SpaceKind::Euclidean3D, "vector", "w");
    static_cast<VectorObject*>(u)->setByFormula(true);
    static_cast<VectorObject*>(w)->setByFormula(true);
    u->setField("value", "w + [1, 0, 0]");
    w->setField("value", "2u");
    sc.evaluate();
    CHECK(u->error().find("circular") != std::string::npos);
    CHECK(w->error().find("circular") != std::string::npos);
    w->setField("value", "[0, 1, 0]");
    sc.evaluate();
    CHECK(u->error().empty() && w->error().empty());
    CHECK_NEAR(u->value().vector.x, 1.0f, 1e-6f);

    // Type checks and plots
    SceneObject* bad = sc.add(SpaceKind::Euclidean3D, "matrix", "N");
    static_cast<MatrixObject*>(bad)->setByFormula(true);
    bad->setField("value", "[1, 2, 3]");
    sc.evaluate();
    CHECK(bad->error().find("should be a matrix") != std::string::npos);
    SceneObject* f = sc.add(SpaceKind::Euclidean3D, "surface", "S");
    f->setField("f", "sin(x) * theta");
    sc.evaluate();
    CHECK(f->error().empty());
    f->setField("xmin", "10");
    sc.evaluate();
    CHECK(!f->error().empty());
    CHECK(d.find("S")->dependencies().count("theta"));

    // Save, load, undo
    SceneData data;
    sc.save(data);
    Scene copy;
    std::string loadError;
    CHECK(copy.load(data, loadError));
    CHECK(copy.space(SpaceKind::Euclidean3D).objects().size() == d.objects().size());
    CHECK(copy.space(SpaceKind::Euclidean3D).find("u") != nullptr);
    copy.evaluate();
    CHECK_NEAR(copy.space(SpaceKind::Euclidean3D).find("u")->value().vector.x, 1.0f, 1e-6f);

    Scene h;
    size_t n0 = h.space(SpaceKind::Euclidean3D).objects().size();
    h.add(SpaceKind::Euclidean3D, "vector", "k");
    h.commit();
    CHECK(h.space(SpaceKind::Euclidean3D).objects().size() == n0 + 1);
    h.undo();
    CHECK(h.space(SpaceKind::Euclidean3D).objects().size() == n0);
    h.redo();
    CHECK(h.space(SpaceKind::Euclidean3D).find("k") != nullptr);

    // Import of a v1 scene file
    SceneData v1;
    v1["matrix.use4x4"] = "0";
    v1["matrix3.00"] = "2";
    v1["vector_a.1"] = "5";
    v1["complex.z1.re"] = "3";
    v1["complex.z1.im"] = "-1";
    v1["plot.type"] = "2";
    v1["plot.surface"] = "x*y";
    Scene old;
    CHECK(old.load(v1, loadError));
    old.evaluate();
    const Space& od = old.space(SpaceKind::Euclidean3D);
    CHECK_NEAR(od.find("M")->value().matrix.m[0][0], 2.0f, 1e-6f);
    CHECK_NEAR(od.find("a")->value().vector.y, 5.0f, 1e-6f);
    bool hasSurface = false;
    for (const auto& o : od.objects())
        if (o->typeId() == "surface" && o->field("f")->text == "x*y") hasSurface = true;
    CHECK(hasSurface);
    const expr::Value& z1 = old.space(SpaceKind::Complex).find("z1")->value();
    CHECK_NEAR(z1.re, 3.0, 1e-6);
    CHECK_NEAR(z1.im, -1.0, 1e-6);

    // Complex plane defaults
    const Space& cp = sc.space(SpaceKind::Complex);
    const expr::Value& wv = cp.find("w")->value();
    CHECK_NEAR(wv.re, 0.5, 1e-6);   // (1 + i)(1 + 0.5i) = 0.5 + 1.5i
    CHECK_NEAR(wv.im, 1.5, 1e-6);
    CHECK(cp.find("u")->error().empty());
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
        {"Playback", testPlayback},
        {"Blend functions", testBlendFunctions},
        {"Scene time", testSceneTime},
        {"Reference", testReference},
        {"Contour", testContour},
        {"Text layout", testTextLayout},
        {"Field variable z", testFieldVariable},
        {"Quaternions", testQuaternions},
        {"Integrals", testIntegrals},
        {"Vector field", testVectorField},
        {"Label layout", testLabelLayout},
        {"LaTeX export", testLatexExport},
        {"PDF compile", testPdfCompile},
        {"Usability", testUsability},
        {"Explain", testExplain},
        {"Quaternion", testQuaternion},
        {"Breakdown", testBreakdown},
        {"ExprEngine", testExprEngine},
        {"Scene", testScene},
    };

    for (const auto& t : tests) {
        int before = g_failures;
        t.fn();
        std::printf("%-12s %s\n", t.name, g_failures == before ? "ok" : "FAILED");
    }
    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
