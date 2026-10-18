// Tests: Vectors, matrices, decompositions (LU, QR, SVD, eigen), complex numbers, quaternions, the transformation breakdown and the worked explanations
#include "TestSupport.hpp"

namespace {

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

} // namespace

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
