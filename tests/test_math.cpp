// Unit tests for the math library (no graphics dependencies).
// Build and run with: make test

#include "animation/Animator.hpp"
#include "animation/Playback.hpp"
#include "math/Complex.hpp"
#include "math/Decompositions.hpp"
#include "math/Expression.hpp"
#include "math/Explain.hpp"
#include "math/expr/Expr.hpp"
#include "scene/Scene.hpp"
#include <map>
#include "math/Breakdown.hpp"
#include "math/Quaternion.hpp"
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
    CHECK(!bad.compile("a.w"));
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
    theta->setPlaying(true);
    theta->advance(0.5);
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

    // Parameter modes
    {
        Scene ps;
        auto* k = static_cast<ParameterObject*>(ps.add(SpaceKind::Euclidean3D, "parameter", "k"));
        auto* q = static_cast<ParameterObject*>(ps.add(SpaceKind::Euclidean3D, "parameter", "q"));
        // Formula of time, using another object
        q->setMode(ParameterObject::Mode::Formula);
        q->setField("formula", "2t + k");
        k->setNumber(1.0);
        q->setPlaying(true);
        q->advance(1.5);
        ps.evaluate();
        CHECK(q->error().empty());
        CHECK_NEAR(q->number(), 4.0, 1e-9);
        CHECK_NEAR(q->value().re, 4.0, 1e-9);
        q->setProp("period", 2.0);
        q->advance(1.0);  // 2.5 s wraps to 0.5 s
        ps.evaluate();
        CHECK_NEAR(q->clock(), 0.5, 1e-9);
        q->restart();
        CHECK_NEAR(q->clock(), 0.0, 1e-12);
        // A slider ignores t in its (inactive) formula field
        k->setField("formula", "nonsense(");
        ps.evaluate();
        CHECK(k->error().empty());
        // Animate: eased value stays in range, ping-pong comes back
        k->setMode(ParameterObject::Mode::Animate);
        k->setProp("min", 2.0);
        k->setProp("max", 4.0);
        Playback pb = k->playback();
        pb.duration = 2.0f;
        pb.repeat = Repeat::PingPong;
        pb.easing = Easing::EaseInOut;
        pb.phase = 0.0f;
        k->setPlayback(pb);
        k->setPlaying(true);
        k->advance(1.0);
        CHECK_NEAR(k->number(), 3.0, 1e-5);  // ease-in-out(0.5) = 0.5
        k->advance(0.5);
        CHECK(k->number() > 3.0 && k->number() <= 4.0);
        k->advance(1.0);  // reached the end and went back half way to 0.75 -> eased
        CHECK(k->playback().direction == -1);
        CHECK(k->number() >= 2.0 && k->number() <= 4.0);
        // Round trip keeps mode, formula and style
        SceneData data;
        ps.save(data);
        Scene back;
        std::string err;
        CHECK(back.load(data, err));
        auto* q2 = static_cast<ParameterObject*>(back.space(SpaceKind::Euclidean3D).find("q"));
        auto* k2 = static_cast<ParameterObject*>(back.space(SpaceKind::Euclidean3D).find("k"));
        CHECK(q2 && q2->mode() == ParameterObject::Mode::Formula && q2->field("formula")->text == "2t + k");
        CHECK(k2 && k2->playback().easing == Easing::EaseInOut && k2->playback().repeat == Repeat::PingPong);
        // Old files: speed + loop, no mode
        for (auto it = data.begin(); it != data.end();) {
            if (it->first.find(".prop.mode") != std::string::npos || it->first.find(".prop.anim.") != std::string::npos)
                it = data.erase(it);
            else
                ++it;
        }
        for (auto& [key, v] : data) {
            if (key.size() > 13 && key.compare(key.size() - 13, 13, ".prop.playing") == 0) v = "1";
        }
        std::string kp;
        for (auto& [key, v] : data)
            if (v == "k" && key.size() > 5 && key.compare(key.size() - 5, 5, ".name") == 0) kp = key.substr(0, key.size() - 4);
        CHECK(!kp.empty());
        data[kp + "prop.speed"] = "0.25";
        data[kp + "prop.loop"] = "0";
        Scene old;
        CHECK(old.load(data, err));
        auto* k3 = static_cast<ParameterObject*>(old.space(SpaceKind::Euclidean3D).find("k"));
        CHECK(k3 && k3->mode() == ParameterObject::Mode::Animate);
        if (k3) {
            CHECK_NEAR(k3->playback().duration, 4.0f, 1e-4f);
            CHECK(k3->playback().repeat == Repeat::Once);
        }
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
        {"Animator", testAnimator},
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
