#include "ui/InfoPanel.hpp"
#include "math/expr/Value.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>

namespace linalg {

namespace {

std::string fmt(const char* format, ...) __attribute__((format(printf, 1, 2)));

std::string fmt(const char* format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    return buf;
}

float clean(float v) { return std::fabs(v) < 5e-5f ? 0.0f : v; }

std::string vecStr(const Vector3D& v) {
    return fmt("(%.2f, %.2f, %.2f)", clean(v.x), clean(v.y), clean(v.z));
}

const char* subscript(int i) {
    static const char* digits[] = {"₀", "₁", "₂", "₃", "₄"};
    return digits[std::clamp(i, 0, 4)];
}

} // namespace

InfoPanel::InfoPanel() : m_eigen{} {}

void InfoPanel::addMatrix(Section& s, const char* name, const float* values, int n) {
    s.lines.push_back({std::string(name) + " =", Tone::Muted, ui::FontRole::Body});
    for (int i = 0; i < n; i++) {
        std::string row = "[";
        for (int j = 0; j < n; j++) row += fmt("%8.3f", clean(values[i * n + j]));
        row += " ]";
        s.lines.push_back({row, Tone::Normal, ui::FontRole::Mono});
    }
}

void InfoPanel::update(const scene::Space& space, const scene::Selection& selection) {
    // Gather what is selected, in selection order
    std::vector<const scene::SceneObject*> picked;
    for (scene::ObjectId id : selection.ids)
        if (const scene::SceneObject* o = space.find(id)) picked.push_back(o);

    const scene::SceneObject* matrixObj = nullptr;
    std::vector<const scene::SceneObject*> vectors, numbers, quats, integralObjs;
    for (const auto* o : picked) {
        if (!o->value().ok()) continue;
        const std::string& ty = o->typeId();
        if (ty == "frame" || ty == "tangent" || ty == "tangentplane" || ty == "velocity" || ty == "newton") {
            integralObjs.push_back(o);
        } else if (o->value().type == expr::Type::Quaternion) {
            if (quats.size() < 2) quats.push_back(o);
        } else if (o->value().type == expr::Type::Matrix) {
            matrixObj = o;  // the most recently selected matrix wins
        } else if (o->value().type == expr::Type::Vector && vectors.size() < 2) vectors.push_back(o);
        else if (o->value().isNumber() && (o->typeId() == "complex" || o->typeId() == "product") && numbers.size() < 2)
            numbers.push_back(o);
        else if (o->typeId() == "doubleintegral" || o->typeId() == "lineintegral" || o->typeId() == "taylor")
            integralObjs.push_back(o);
    }

    // Rebuild sections, keeping their expanded state by key
    std::vector<Section> previous = std::move(m_sections);
    m_sections.clear();
    auto section = [&](const std::string& key, const std::string& title, bool def) -> Section& {
        bool expanded = def;
        for (const auto& p : previous)
            if (p.key == key) expanded = p.expanded;
        m_sections.push_back({key, title, expanded, {}});
        return m_sections.back();
    };
    auto body = [](Section& s, const std::string& text, Tone tone = Tone::Normal, const char* tip = nullptr) {
        s.lines.push_back({text, tone, ui::FontRole::Body, tip});
    };
    auto tipped = [&](Section& s, const std::string& text, const char* tip) { body(s, text, Tone::Normal, tip); };
    auto note = [](Section& s, const std::string& text) {
        s.lines.push_back({text, Tone::Muted, ui::FontRole::Small});
    };
    m_eigen = EigenResult{};
    m_hasMatrix = false;

    if (!matrixObj && vectors.empty() && numbers.empty() && quats.empty() && integralObjs.empty()) {
        Section& s = section("empty", "Nothing to analyse", true);
        note(s, "Select a matrix, one or two vectors or quaternions, or complex numbers (Ctrl+click for several).");
        return;
    }

    // --- Integrals -------------------------------------------------------------
    for (size_t k = 0; k < integralObjs.size(); k++) {
        const scene::SceneObject* o = integralObjs[k];
        Section& s = section("integral." + std::to_string(k), o->name(), true);
        if (o->typeId() == "frame") {
            auto* fo = static_cast<const scene::FrameObject*>(o);
            const geometry::Frame& f = fo->frame();
            auto v3 = [](const geometry::D3& d) {
                return vecStr(integrals::toVector(d));
            };
            s.tip = "The Frenet frame at the point: how the curve moves, turns and twists there.";
            body(s, fmt("at u = %.3f: r = %s", fo->u0(), v3(f.p).c_str()));
            tipped(s, "T = " + v3(f.T), "Unit tangent: the direction of motion, r'/|r'|.");
            if (f.hasNormal) {
                tipped(s, "N = " + v3(f.N), "Unit normal: towards the center of the turn.");
                tipped(s, "B = " + v3(f.B), "Binormal T × N: at right angles to the plane the curve turns in.");
            } else {
                note(s, "Straight here: no normal or binormal.");
            }
            tipped(s, fmt("κ = %.4f   τ = %.4f", f.kappa, std::fabs(f.tau) < 5e-5 ? 0.0 : f.tau),
                   "Curvature κ = |r' × r''| / |r'|³; torsion τ = (r' × r'') · r''' / |r' × r''|².");
            if (f.kappa > 1e-9) note(s, fmt("radius of the osculating circle 1/κ = %.3f", 1.0 / f.kappa));
            continue;
        }
        if (o->typeId() == "tangent") {
            auto* tg = static_cast<const scene::TangentObject*>(o);
            s.tip = "The derivative at a point: the slope of the tangent line.";
            tipped(s, fmt("f(%.4g) = %.4f", tg->x0(), tg->y0()), "The point on the graph.");
            tipped(s, fmt("f'(%.4g) = %.4f", tg->x0(), tg->slope()),
                   "The slope: the limit of (f(x0 + h) − f(x0)) / h as h → 0 (computed with a tiny step).");
            if (tg->hasSecant())
                tipped(s, fmt("secant, h = %.3g: %.4f", tg->secantH(), tg->secantSlope()),
                       "(f(x0 + h) − f(x0)) / h: closer to f' as h shrinks.");
            if (tg->hasSecond()) note(s, fmt("f''(x0) = %.3f", std::fabs(tg->second()) < 1e-4 ? 0.0 : tg->second()));
            double angle = std::atan(tg->slope()) * 180.0 / 3.14159265358979;
            note(s, fmt("the tangent rises at %.1f° to the x axis", angle));
            continue;
        }
        if (o->typeId() == "tangentplane") {
            auto* g = static_cast<const scene::TangentPlaneObject*>(o);
            auto p = g->point();
            s.tip = "Partial derivatives: the slopes of the surface along x and along y.";
            body(s, fmt("at (%.3g, %.3g): z = %.4f", p.x, p.y, p.z));
            tipped(s, fmt("∂f/∂x = %.4f   ∂f/∂y = %.4f", g->fx(), g->fy()),
                   "Hold y fixed and take the slope in x, then the other way round.");
            tipped(s, fmt("grad f = (%.4f, %.4f)", g->fx(), g->fy()),
                   "Points the way the surface climbs fastest; its length is that climb rate.");
            note(s, fmt("steepest climb %.4f per unit", std::hypot(g->fx(), g->fy())));
            continue;
        }
        if (o->typeId() == "taylor") {
            auto* tp = static_cast<const scene::TaylorObject*>(o);
            const auto& se = tp->series();
            s.tip = "f(x) ≈ c0 + c1 (x − a) + c2 (x − a)² + …, with c_k = f⁽ᵏ⁾(a) / k!";
            body(s, fmt("about a = %.4g, degree %d", se.center, static_cast<int>(se.coeffs.size()) - 1));
            for (size_t j = 0; j < se.coeffs.size() && j < 8; j++)
                tipped(s, fmt("c%zu = %.6g", j, std::fabs(se.coeffs[j]) < 1e-12 ? 0.0 : se.coeffs[j]),
                       "The k-th derivative of f at the center, divided by k!.");
            if (std::isfinite(tp->maxError())) note(s, fmt("largest gap to f on the range: %.3g", tp->maxError()));
            continue;
        }
        if (o->typeId() == "newton") {
            auto* nw = static_cast<const scene::NewtonObject*>(o);
            s.tip = "x_{k+1} = x_k − f(x_k) / f'(x_k): follow the tangent down to the x axis.";
            const auto& steps = nw->run().steps;
            for (size_t j = 0; j < steps.size() && j < 8; j++)
                body(s, fmt("x%zu = %.10g", j, steps[j].x), j + 1 == steps.size() ? Tone::Normal : Tone::Muted);
            continue;
        }
        if (o->typeId() == "velocity") {
            auto* r = static_cast<const scene::VelocityObject*>(o);
            auto v3 = [](const integrals::D3& d) {
                return vecStr(integrals::toVector(d));
            };
            s.tip = "Derivatives of a moving point: velocity r' and acceleration r''.";
            body(s, fmt("at u = %.3f: r = %s", r->u0(), v3(r->point()).c_str()));
            tipped(s, "r' = " + v3(r->velocity()), "Velocity: where the point is heading; its length is the speed.");
            tipped(s, "r'' = " + v3(r->acceleration()), "Acceleration: how the velocity changes.");
            note(s, fmt("speed %.4f · along the motion %.4f · across it %.4f", geometry::length(r->velocity()),
                        geometry::length(r->tangential()), geometry::length(r->normal())));
            continue;
        }
        if (o->typeId() == "doubleintegral") {
            auto* d = static_cast<const scene::DoubleIntegralObject*>(o);
            s.tip = "The volume between the floor and z = f(x, y), and the boxes that approximate it.";
            int n = d->boxesPerSide();
            tipped(s, fmt("%s = ∫∫ f dA ≈ %.4f", o->name().c_str(), d->exact()),
                   "Computed precisely (Simpson's rule on a fine grid).");
            tipped(s, fmt("boxes %d × %d: %.4f", n, n, d->riemann().sum), "Σ f(sample point) · area of the box.");
            note(s, fmt("error %.4f; doubling n makes it about 4× smaller (middle rule)", d->riemann().sum - d->exact()));
        } else {
            auto* w = static_cast<const scene::LineIntegralObject*>(o);
            s.tip = "The work of F along the path: how much F pushes along the motion, added up.";
            tipped(s, fmt("%s = ∫ F·dr = %.4f", o->name().c_str(), o->value().re), "Up to the marker.");
            body(s, fmt("whole path %.4f", w->total()), Tone::Muted);
            note(s, "Positive: F mostly helps the motion; zero around a closed path for a gradient field.");
        }
    }

    // --- Quaternions ---------------------------------------------------------
    for (size_t k = 0; k < quats.size(); k++) {
        const scene::SceneObject* q = quats[k];
        const expr::Value& v = q->value();
        const char* N = q->name().c_str();
        Section& s = section("quat." + std::to_string(k), "Quaternion " + q->name(), true);
        s.tip = "The rotation this quaternion stands for, as an axis and angle, a matrix and yaw/pitch/roll.";
        body(s, q->name() + " = " + v.summary());
        tipped(s, fmt("w = %.3f   x = %.3f   y = %.3f   z = %.3f", v.re, v.qi, v.qj, v.qk),
               "The four numbers: w = cos(angle/2), (x, y, z) = axis · sin(angle/2) for a unit quaternion.");
        expr::Rotation r;
        if (!expr::rotationOf(v, r)) {
            note(s, "The zero quaternion is not a rotation.");
            continue;
        }
        tipped(s, fmt("|%s| = %.3f", N, r.norm), "Length √(w² + x² + y² + z²). Rotations use length 1.");
        if (r.hasAxis)
            tipped(s, fmt("angle %.1f° about %s", r.angle * RAD2DEG, vecStr(r.axis).c_str()),
                   "Turns by this angle about the axis, counterclockwise when the axis points at you.");
        else
            body(s, "no rotation (angle 0)");
        float vals[9];
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) vals[i * 3 + j] = r.matrix.m[i][j];
        addMatrix(s, ("mat(" + q->name() + ")").c_str(), vals, 3);
        // Yaw, pitch, roll (z, then y, then x: R = Rz(yaw)·Ry(pitch)·Rx(roll))
        const Matrix3D& m = r.matrix;
        double yaw = std::atan2(m.m[1][0], m.m[0][0]);
        double pitch = std::asin(std::clamp(-static_cast<double>(m.m[2][0]), -1.0, 1.0));
        double roll = std::atan2(m.m[2][1], m.m[2][2]);
        tipped(s, fmt("yaw %.1f°   pitch %.1f°   roll %.1f°", clean(static_cast<float>(yaw * RAD2DEG)),
                      clean(static_cast<float>(pitch * RAD2DEG)), clean(static_cast<float>(roll * RAD2DEG))),
               "The same rotation as three turns: roll about x, then pitch about y, then yaw about z.");
        if (std::fabs(r.norm - 1.0) > 1e-3) note(s, "Not length 1: drawn as the rotation of q/|q| (unit(q)).");
    }
    if (quats.size() == 2) {
        const expr::Value& a = quats[0]->value();
        const expr::Value& b = quats[1]->value();
        const std::string& na = quats[0]->name();
        const std::string& nb = quats[1]->name();
        auto mul = [](const expr::Value& x, const expr::Value& y) {
            return expr::Value::quat(x.re * y.re - x.qi * y.qi - x.qj * y.qj - x.qk * y.qk,
                                     x.re * y.qi + x.qi * y.re + x.qj * y.qk - x.qk * y.qj,
                                     x.re * y.qj - x.qi * y.qk + x.qj * y.re + x.qk * y.qi,
                                     x.re * y.qk + x.qi * y.qj - x.qj * y.qi + x.qk * y.re);
        };
        expr::Value ab = mul(a, b), ba = mul(b, a);
        Section& s = section("quat.pair", na + " and " + nb, true);
        s.tip = "Composing two rotations: the product does the right one first.";
        tipped(s, na + "·" + nb + " = " + ab.summary(), "The rotation: first the right factor, then the left.");
        body(s, nb + "·" + na + " = " + ba.summary());
        double diff = std::fabs(ab.re - ba.re) + std::fabs(ab.qi - ba.qi) + std::fabs(ab.qj - ba.qj) + std::fabs(ab.qk - ba.qk);
        note(s, diff > 1e-6 ? "Different: the order of rotations matters." : "The same here (same axis, or one is no turn).");
        expr::Rotation ra, rb;
        if (expr::rotationOf(a, ra) && expr::rotationOf(b, rb)) {
            double na2 = std::sqrt(a.re * a.re + a.qi * a.qi + a.qj * a.qj + a.qk * a.qk);
            double nb2 = std::sqrt(b.re * b.re + b.qi * b.qi + b.qj * b.qj + b.qk * b.qk);
            double d = std::fabs((a.re * b.re + a.qi * b.qi + a.qj * b.qj + a.qk * b.qk) / (na2 * nb2));
            tipped(s, fmt("angle between them %.1f°", 2 * std::acos(std::min(1.0, d)) * RAD2DEG),
                   "How far you turn to get from one rotation to the other (what slerp travels).");
        }
    }

    // --- Vectors -----------------------------------------------------------
    if (vectors.size() == 1) {
        const std::string& n = vectors[0]->name();
        Vector3D a = vectors[0]->value().vector;
        Section& s = section("vector", "Vector " + n, true);
        s.tip = "Length and direction of the selected vector. Select a second one for products and angles.";
        body(s, n + " = " + vecStr(a));
        tipped(s, fmt("|%s| = %.3f", n.c_str(), a.magnitude()), "Length of a vector: |a| = √(a·a).");
        if (a.magnitude() > 1e-6f) {
            tipped(s, "unit vector = " + vecStr(a.normalized()), "Same direction, length 1: a / |a|.");
        }
    } else if (vectors.size() == 2) {
        const std::string& na = vectors[0]->name();
        const std::string& nb = vectors[1]->name();
        Vector3D a = vectors[0]->value().vector, b = vectors[1]->value().vector;
        const char* A = na.c_str();
        const char* B = nb.c_str();
        Section& s = section("vectors", "Vectors " + na + ", " + nb, true);
        s.tip = "Products and angles of the two selected vectors.";
        float dot = a.dot(b);
        float la = a.magnitude(), lb = b.magnitude();
        Vector3D cross = a.cross(b);
        tipped(s, fmt("%s · %s = %.3f", A, B, clean(dot)),
               "Dot product: a·b = |a||b| cos θ = axbx + ayby + azbz. Zero means a and b are perpendicular.");
        tipped(s, fmt("|%s| = %.3f     |%s| = %.3f", A, la, B, lb), "Length of a vector: |a| = √(a·a).");
        if (la > 1e-6f && lb > 1e-6f) {
            float c = std::clamp(dot / (la * lb), -1.0f, 1.0f);
            tipped(s, fmt("angle(%s, %s) = %.1f°", A, B, std::acos(c) * RAD2DEG),
                   "θ = arccos(a·b / (|a||b|)), the angle between the two arrows.");
            if (std::fabs(dot) < 1e-5f) note(s, na + " and " + nb + " are perpendicular");
        }
        tipped(s, na + " × " + nb + " = " + vecStr(cross),
               "Cross product: perpendicular to both (right-hand rule). Its length is the area of the "
               "parallelogram they span.");
        note(s, fmt("|%s × %s| = %.3f, the area of the parallelogram", A, B, cross.magnitude()));
    }

    // --- Complex numbers ---------------------------------------------------
    for (size_t k = 0; k < numbers.size(); k++) {
        const scene::SceneObject* z = numbers[k];
        const expr::Value& v = z->value();
        Section& s = section("complex." + std::to_string(k), z->name(), true);
        s.tip = "Rectangular and polar form of the selected complex number.";
        body(s, z->name() + " = " + v.summary());
        tipped(s, fmt("|%s| = %.3f", z->name().c_str(), std::hypot(v.re, v.im)), "Length: √(re² + im²).");
        tipped(s, fmt("arg %s = %.1f°", z->name().c_str(), clean(static_cast<float>(std::atan2(v.im, v.re) * RAD2DEG))),
               "Angle from the positive real axis.");
        body(s, "conj = " + expr::Value::complex(v.re, -v.im).summary(), Tone::Muted);
    }
    if (numbers.size() == 2) {
        const expr::Value& a = numbers[0]->value();
        const expr::Value& b = numbers[1]->value();
        Section& s = section("complex.pair", numbers[0]->name() + " and " + numbers[1]->name(), true);
        double pr = a.re * b.re - a.im * b.im, pi = a.re * b.im + a.im * b.re;
        double d = b.re * b.re + b.im * b.im;
        body(s, "product = " + expr::Value::complex(pr, pi).summary());
        if (d > 1e-12) body(s, "quotient = " + expr::Value::complex((a.re * b.re + a.im * b.im) / d, (a.im * b.re - a.re * b.im) / d).summary());
        note(s, "Multiplying multiplies lengths and adds angles.");
    }

    if (!matrixObj) return;

    // --- Matrix properties -------------------------------------------------
    const std::string mname = matrixObj->name();
    const char* M = mname.c_str();
    Matrix4D matrix = matrixObj->value().matrix4();
    bool is4x4 = matrixObj->value().size == 4;
    Matrix3D linear = matrixObj->value().matrix3();
    m_hasMatrix = true;

    {
        Section& s = section("matrix", "Matrix " + mname, true);
        s.tip = "Properties of the selected transformation.";
        float det = is4x4 ? matrix.determinant() : linear.determinant();
        float trace = is4x4 ? matrix.trace() : linear.trace();
        tipped(s, fmt("det %s = %.4f", M, clean(det)),
               "The determinant is how much the matrix scales volumes. Negative: orientation flips (a mirror). "
               "Zero: space is squashed into a plane, line or point, and it cannot be undone.");
        if (std::fabs(det) < 1e-6f) note(s, "Space is squashed flat: the matrix is singular");
        else if (det < 0.0f) note(s, fmt("Orientation flips; volumes scale by %.3f", std::fabs(det)));
        else note(s, fmt("Volumes scale by %.3f", det));
        tipped(s, fmt("tr %s = %.4f", M, clean(trace)),
               "Trace: the sum of the diagonal entries. It always equals the sum of the eigenvalues.");

        int n = is4x4 ? 4 : 3;
        float vals[16];
        bool invertible = false;
        if (is4x4) {
            if (auto inv = matrix.inverse()) {
                invertible = true;
                for (int i = 0; i < 4; i++)
                    for (int j = 0; j < 4; j++) vals[i * 4 + j] = inv->m[i][j];
            }
        } else if (auto inv = linear.inverse()) {
            invertible = true;
            for (int i = 0; i < 3; i++)
                for (int j = 0; j < 3; j++) vals[i * 3 + j] = inv->m[i][j];
        }
        if (invertible) {
            addMatrix(s, (mname + "⁻¹").c_str(), vals, n);
            s.lines[s.lines.size() - n - 1].tip = "The inverse undoes the matrix: M⁻¹M = I. It exists exactly when det ≠ 0.";
        } else {
            body(s, mname + "⁻¹ does not exist (det = 0)", Tone::Danger);
        }
    }

    // --- Eigen -------------------------------------------------------------
    m_eigen = eigen3(linear);
    {
        Section& s = section("eigen", "Eigenvalues of " + mname, true);
        s.tip = "Roots of det(M − λI) = 0 and the directions the matrix only stretches.";
        if (is4x4) note(s, "Of the upper-left 3×3 (linear) part");
        int index = 1;
        bool anyComplex = false, anyMissing = false;
        for (const auto& ev : m_eigen.values) {
            if (ev.isReal()) {
                tipped(s, fmt("λ%s = %.3f", subscript(index), clean(ev.re)),
                       "Eigenvalue: M v = λ v. Along its eigenvector v, the matrix only stretches by λ (negative λ "
                       "also flips).");
                if (ev.hasVector) {
                    s.lines.push_back({fmt("v%s = ", subscript(index)) + vecStr(ev.vector), Tone::Eigen,
                                       ui::FontRole::Body,
                                       "Eigenvector (unit length). Add an Eigenvectors object to see these "
                                       "directions in the scene."});
                } else {
                    anyMissing = true;
                }
            } else {
                tipped(s, fmt("λ%s = %.3f %c %.3fi", subscript(index), clean(ev.re), ev.im < 0 ? '-' : '+',
                              std::fabs(ev.im)),
                       "Complex eigenvalues come in conjugate pairs: the matrix rotates a plane by arg λ and "
                       "scales it by |λ|, so no real direction stays fixed there.");
                anyComplex = true;
            }
            index++;
        }
        if (anyComplex) note(s, "A complex pair means the matrix rotates a plane");
        if (anyMissing) note(s, "Defective: fewer eigenvectors than eigenvalues");
    }

    // --- Decompositions ----------------------------------------------------
    auto toArray = [](const Matrix3D& m, float* out) {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) out[i * 3 + j] = m.m[i][j];
    };
    float vals[9];
    {
        Section& s = section("lu", "LU  ·  PA = LU", false);
        s.tip = "Gaussian elimination as factors: P swaps rows, L (lower triangular) records the elimination "
                "steps, U (upper triangular) is the result.";
        if (is4x4) note(s, "Of the 3×3 linear part");
        LUResult lu = luDecompose(linear);
        toArray(lu.P, vals); addMatrix(s, "P", vals, 3);
        toArray(lu.L, vals); addMatrix(s, "L", vals, 3);
        toArray(lu.U, vals); addMatrix(s, "U", vals, 3);
        if (lu.singular) note(s, "A zero pivot: the matrix is singular");
    }
    {
        Section& s = section("qr", "QR  ·  A = QR", false);
        s.tip = "Gram–Schmidt as factors: Q has orthonormal columns (a rotation, maybe with a mirror) and R is "
                "upper triangular.";
        if (is4x4) note(s, "Of the 3×3 linear part");
        QRResult qr = qrDecompose(linear);
        toArray(qr.Q, vals); addMatrix(s, "Q", vals, 3);
        toArray(qr.R, vals); addMatrix(s, "R", vals, 3);
    }
    {
        Section& s = section("svd", "SVD  ·  A = UΣV^T", false);
        s.tip = "Every matrix is a rotation (V^T), then stretches along the axes by the singular values σ (Σ), "
                "then another rotation (U).";
        if (is4x4) note(s, "Of the 3×3 linear part");
        SVDResult svd = svd3(linear);
        body(s, fmt("σ = %.3f, %.3f, %.3f", svd.S.m[0][0], svd.S.m[1][1], svd.S.m[2][2]));
        toArray(svd.U, vals); addMatrix(s, "U", vals, 3);
        toArray(svd.V, vals); addMatrix(s, "V", vals, 3);
    }
}

float InfoPanel::drawContent(Rectangle area) {
    const ui::Theme& t = ui::theme();
    float x = area.x + ui::px(ui::kPad);
    float w = area.width - 2 * ui::px(ui::kPad);
    float y = area.y + ui::px(8.0f);
    float startY = area.y;

    for (auto& s : m_sections) {
        Rectangle headerRect = {x, y, w, ui::px(28.0f)};
        ui::sectionHeader(headerRect, s.title.c_str(), s.expanded, nullptr, true);
        if (s.tip) ui::tooltip(headerRect, s.tip, s.title.c_str());
        y += headerRect.height + ui::px(2.0f);

        if (s.expanded) {
            for (const auto& line : s.lines) {
                Color c = t.text;
                if (line.tone == Tone::Muted) c = t.textMuted;
                else if (line.tone == Tone::Eigen) c = t.eigen;
                else if (line.tone == Tone::Danger) c = t.danger;
                float lineH = std::round(ui::fontSize(line.role) * 1.45f);
                std::string shown = ui::ellipsize(line.text.c_str(), line.role, w - ui::px(14.0f));
                ui::text(shown.c_str(), x + ui::px(14.0f), y + (lineH - ui::fontSize(line.role)) / 2, line.role, c);
                if (line.tip) ui::tooltip({x, y, w, lineH}, line.tip);
                y += lineH;
            }
            y += ui::px(10.0f);
        }
    }
    return y - startY + ui::px(4.0f);
}

} // namespace linalg
