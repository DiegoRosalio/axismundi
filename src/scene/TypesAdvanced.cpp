// The richer object types: quaternions, vector fields, integrals, moving frames, Fourier (the basic types and
// the registry are in Types.cpp)
#include "scene/Types.hpp"
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <cstdio>
#include <limits>

namespace linalg::scene {

// --- Quaternion -----------------------------------------------------------------------

QuaternionObject::QuaternionObject() : SceneObject("quaternion") {
    setProp("axis", true);
    setProp("basis", true);
    setProp("act", false);
    addField("value", "q =", Expect::Quaternion, "qaxis([0, 0, 1], pi/2)");
    addField("v", "Acts on", Expect::Vector, "[1, 0, 0]");
}

bool QuaternionObject::fieldActive(const Field& f) const {
    return f.key != "v" || actsOnVector();
}

// --- Vector field ---------------------------------------------------------------------

VectorFieldObject::VectorFieldObject() : SceneObject("field") {
    setProp("domain", "plane");
    setProp("density", kDensityDefault);
    setProp("heatmap", true);
    setProp("same_length", false);
    setProp("lines", false);
    setProp("seeds", 6.0);
    setProp("particles", false);
    setProp("speed", 1.0);
    addField("F", "F(x, y, z) =", Expect::Field, "[-y, x, 0]");
    markFieldVariable("F");
    addField("xmin", "x from", Expect::Number, "-4");
    addField("xmax", "x to", Expect::Number, "4");
    addField("ymin", "y from", Expect::Number, "-4");
    addField("ymax", "y to", Expect::Number, "4");
    addField("zmin", "z from", Expect::Number, "-2");
    addField("zmax", "z to", Expect::Number, "2");
}

bool VectorFieldObject::fieldActive(const Field& f) const {
    if (f.key == "zmin" || f.key == "zmax") return volume();
    return true;
}

int VectorFieldObject::seeds() const {
    return static_cast<int>(std::clamp(propNumber("seeds", 6), 2.0, 16.0));
}

double VectorFieldObject::speed() const {
    return std::clamp(propNumber("speed", 1), 0.1, 5.0);
}

double VectorFieldObject::rangeValue(const char* key, double fallback) const {
    const Field* f = field(key);
    if (!f || !f->value.isRealNumber()) return fallback;
    return f->value.re;
}

double VectorFieldObject::density() const {
    return std::clamp(propNumber("density", kDensityDefault), kDensityMin, kDensityMax);
}

VectorFieldObject::Counts VectorFieldObject::counts() const {
    double d = density();
    int cap = volume() ? kVolumeCap : kPlaneCap;
    Counts c;
    auto count = [&](const char* lo, const char* hi, double a, double b) {
        double span = rangeValue(hi, b) - rangeValue(lo, a);
        if (!(span > 0.0) || !std::isfinite(span)) span = 1.0;
        int wanted = static_cast<int>(std::clamp(std::round(span * d), 1.0, 1e6));
        if (wanted > cap) c.capped = true;
        return std::min(wanted, cap);
    };
    c.nx = count("xmin", "xmax", -4, 4);
    c.ny = count("ymin", "ymax", -4, 4);
    c.nz = volume() ? count("zmin", "zmax", -2, 2) : 1;
    return c;
}

expr::Value VectorFieldObject::computeValue() const {
    for (auto [lo, hi] : {std::pair{"xmin", "xmax"}, std::pair{"ymin", "ymax"}, std::pair{"zmin", "zmax"}}) {
        if (!volume() && lo[0] == 'z') continue;
        if (!(rangeValue(lo, 0) < rangeValue(hi, 0)))
            return expr::Value::fail(std::string(field(lo)->label) + " must be less than " + field(hi)->label);
    }
    return expr::Value::fail("no value");  // drawn, not used in formulas
}

// --- Double integral ----------------------------------------------------------------

namespace {

// Evaluate a plot-like field at (x, y, z, u) with the object's resolver and time
bool evalNumber(const Field* f, const expr::Resolver* r, double time, double x, double y, double z, double u,
                double& out) {
    if (!f || !f->program.valid()) return false;
    expr::Slots s;
    s.x = x, s.y = y, s.z = z, s.u = u, s.t = time;
    s.hasX = s.hasY = s.hasZ = s.hasU = s.hasT = true;
    expr::Value v = f->program.eval(r, &s);
    if (!v.isRealNumber()) return false;
    out = v.re;
    return std::isfinite(out);
}

bool evalVector(const Field* f, const expr::Resolver* r, double time, double x, double y, double z,
                integrals::D3& out) {
    if (!f || !f->program.valid()) return false;
    expr::Slots s;
    s.x = x, s.y = y, s.z = z, s.t = time;
    s.hasX = s.hasY = s.hasZ = s.hasT = true;
    expr::Value v = f->program.eval(r, &s);
    if (v.type != expr::Type::Vector) return false;
    out = {v.vector.x, v.vector.y, v.vector.z};
    return std::isfinite(out.x) && std::isfinite(out.y) && std::isfinite(out.z);
}

std::string rangeError(const SceneObject& o, const char* lo, const char* hi) {
    const Field* a = o.field(lo);
    const Field* b = o.field(hi);
    if (!a || !b || !a->value.isRealNumber() || !b->value.isRealNumber()) return "";
    if (!(a->value.re < b->value.re)) return a->label + " must be less than " + b->label;
    return "";
}

double numberOf(const SceneObject& o, const char* key, double fallback) {
    const Field* f = o.field(key);
    return f && f->value.isRealNumber() ? f->value.re : fallback;
}

} // namespace

DoubleIntegralObject::DoubleIntegralObject() : SceneObject("doubleintegral") {
    setProp("rule", "middle");
    setProp("boxes", true);
    setProp("surface", true);
    addField("f", "z =", Expect::Plot, "2 - (x^2 + y^2)/4");
    addField("xmin", "x from", Expect::Number, "-2");
    addField("xmax", "x to", Expect::Number, "2");
    addField("ymin", "y from", Expect::Number, "-2");
    addField("ymax", "y to", Expect::Number, "2");
    addField("n", "Boxes per side", Expect::Number, "6");
}

integrals::Rect DoubleIntegralObject::rect() const {
    return {numberOf(*this, "xmin", -2), numberOf(*this, "xmax", 2), numberOf(*this, "ymin", -2),
            numberOf(*this, "ymax", 2)};
}

int DoubleIntegralObject::boxesPerSide() const {
    return static_cast<int>(std::clamp(std::round(numberOf(*this, "n", 6)), 1.0, 60.0));
}

integrals::Rule DoubleIntegralObject::rule() const {
    std::string r = prop("rule", "middle");
    return r == "left" ? integrals::Rule::Left : r == "right" ? integrals::Rule::Right : integrals::Rule::Middle;
}

expr::Value DoubleIntegralObject::computeValue() const {
    std::string err = rangeError(*this, "xmin", "xmax");
    if (err.empty()) err = rangeError(*this, "ymin", "ymax");
    if (!err.empty()) return expr::Value::fail(err);
    const Field* f = field("f");
    const expr::Resolver* res = evaluationResolver();
    double t = time();
    integrals::Surface fn = [&](double x, double y, double& z) { return evalNumber(f, res, t, x, y, 0, 0, z); };
    integrals::Rect r = rect();
    int n = boxesPerSide();
    m_riemann = integrals::riemann2D(fn, r, n, n, rule());
    bool ok = false;
    m_exact = integrals::simpson2D(fn, r, 120, ok);
    // The surface, for drawing: grid lines in both directions
    m_lines.clear();
    const int lines = 16, samples = 48;
    for (int dir = 0; dir < 2; dir++)
        for (int k = 0; k <= lines; k++) {
            std::vector<integrals::D3> line;
            for (int i = 0; i <= samples; i++) {
                double a = static_cast<double>(k) / lines, b = static_cast<double>(i) / samples;
                double x = r.x0 + (dir == 0 ? a : b) * (r.x1 - r.x0), y = r.y0 + (dir == 0 ? b : a) * (r.y1 - r.y0);
                double z = 0;
                if (fn(x, y, z)) line.push_back({x, y, z});
            }
            m_lines.push_back(std::move(line));
        }
    m_version++;
    if (!ok) return expr::Value::fail("f is undefined somewhere in the range");
    return expr::Value::scalar(m_exact);
}

// --- Line integral ------------------------------------------------------------------

LineIntegralObject::LineIntegralObject() : SceneObject("lineintegral") {
    addField("F", "F(x, y, z) =", Expect::Field, "[-y, x, 0]");
    markFieldVariable("F");
    addField("x", "x(u) =", Expect::Plot, "2cos(u)");
    addField("y", "y(u) =", Expect::Plot, "2sin(u)");
    addField("z", "z(u) =", Expect::Plot, "0");
    for (const char* k : {"x", "y", "z"}) markCurveVariable(k);
    addField("umin", "u from", Expect::Number, "0");
    addField("umax", "u to", Expect::Number, "2pi");
    addField("upto", "Up to", Expect::Number, "1");
}

expr::Value LineIntegralObject::computeValue() const {
    std::string err = rangeError(*this, "umin", "umax");
    if (!err.empty()) return expr::Value::fail(err);
    const expr::Resolver* res = evaluationResolver();
    double t = time();
    const Field *fx = field("x"), *fy = field("y"), *fz = field("z"), *fF = field("F");
    integrals::Path r = [&](double u, integrals::D3& o) {
        return evalNumber(fx, res, t, 0, 0, 0, u, o.x) && evalNumber(fy, res, t, 0, 0, 0, u, o.y) &&
               evalNumber(fz, res, t, 0, 0, 0, u, o.z);
    };
    integrals::Field F = [&](const integrals::D3& p, integrals::D3& o) { return evalVector(fF, res, t, p.x, p.y, p.z, o); };
    double a = numberOf(*this, "umin", 0), b = numberOf(*this, "umax", 6.28);
    m_upto = std::clamp(numberOf(*this, "upto", 1), 0.0, 1.0);
    bool ok = false;
    m_total = integrals::lineIntegral(F, r, a, b, 1.0, 400, ok);
    if (!ok) return expr::Value::fail("F or the curve is undefined somewhere along the path");
    double partial = integrals::lineIntegral(F, r, a, b, m_upto, 400, ok);
    // Path samples with F · T, a few F arrows, and the marker
    m_path.clear();
    m_arrows.clear();
    m_maxAlong = 0.0;
    const int n = 160;
    double h = std::max(1e-7, std::fabs(b - a) * 1e-5);
    for (int i = 0; i <= n; i++) {
        double u = a + (b - a) * i / n;
        integrals::D3 p, d, v;
        if (!r(u, p) || !integrals::derivative(r, u, h, d) || !F(p, v)) continue;
        double len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
        double along = len > 1e-12 ? (v.x * d.x + v.y * d.y + v.z * d.z) / len : 0.0;
        m_path.push_back({p, along});
        m_maxAlong = std::max(m_maxAlong, std::fabs(along));
        if (i % 10 == 5) m_arrows.push_back({p, v});
    }
    r(a + m_upto * (b - a), m_marker);
    m_version++;
    return expr::Value::scalar(partial);
}

// --- Moving frame -------------------------------------------------------------------

FrameObject::FrameObject() : SceneObject("frame") {
    setProp("frame", true);
    setProp("circle", true);
    setProp("comb", false);
    setProp("curve", true);
    addField("x", "x(u) =", Expect::Plot, "2cos(u)");
    addField("y", "y(u) =", Expect::Plot, "2sin(u)");
    addField("z", "z(u) =", Expect::Plot, "u/4");
    for (const char* k : {"x", "y", "z"}) markCurveVariable(k);
    addField("umin", "u from", Expect::Number, "0");
    addField("umax", "u to", Expect::Number, "4pi");
    addField("u0", "At u =", Expect::Number, "2");
}

expr::Value FrameObject::computeValue() const {
    std::string err = rangeError(*this, "umin", "umax");
    if (!err.empty()) return expr::Value::fail(err);
    const expr::Resolver* res = evaluationResolver();
    double t = time();
    const Field *fx = field("x"), *fy = field("y"), *fz = field("z");
    integrals::Path r = [&](double u, integrals::D3& o) {
        return evalNumber(fx, res, t, 0, 0, 0, u, o.x) && evalNumber(fy, res, t, 0, 0, 0, u, o.y) &&
               evalNumber(fz, res, t, 0, 0, 0, u, o.z);
    };
    double a = numberOf(*this, "umin", 0), b = numberOf(*this, "umax", 6.28);
    m_u0 = std::clamp(numberOf(*this, "u0", a), a, b);
    double h = std::max(1e-6, (b - a) * 2e-4);
    m_curve.clear();
    m_comb.clear();
    const int n = 240;
    for (int i = 0; i <= n; i++) {
        integrals::D3 p;
        if (r(a + (b - a) * i / n, p)) m_curve.push_back(p);
    }
    // Comb: κ along the curve, the longest tooth 1.2 units, on the outer side (−N)
    std::vector<geometry::Frame> frames;
    double kmax = 0;
    const int teeth = 80;
    for (int i = 0; i <= teeth; i++) {
        geometry::Frame f;
        if (geometry::frenet(r, a + (b - a) * i / teeth, h, f)) {
            frames.push_back(f);
            kmax = std::max(kmax, f.kappa);
        }
    }
    double combScale = kmax > 1e-9 ? 1.2 / kmax : 0.0;
    for (const auto& f : frames) m_comb.push_back({f.p, geometry::sub(f.p, geometry::scale(f.N, f.kappa * combScale))});
    m_version++;
    if (!geometry::frenet(r, m_u0, h, m_frame))
        return expr::Value::fail("the curve stops (r'(u) = 0) or is undefined at u = " + std::to_string(m_u0));
    return expr::Value::vec(Vector3D(static_cast<float>(m_frame.p.x), static_cast<float>(m_frame.p.y),
                                     static_cast<float>(m_frame.p.z)));
}

// --- Fourier epicycles ---------------------------------------------------------------

EpicyclesObject::EpicyclesObject() : SceneObject("epicycles") {
    addField("z", "z(u) =", Expect::ComplexNum, "(16sin(u)^3 + i*(13cos(u) - 5cos(2u) - 2cos(3u) - cos(4u)))/8");
    markCurveVariable("z");
    addField("N", "Circles", Expect::Number, "12");
    addField("speed", "Laps per second", Expect::Number, "0.1");
}

int EpicyclesObject::circles() const {
    return static_cast<int>(std::clamp(std::round(numberOf(*this, "N", 12)), 1.0, 100.0));
}

expr::Value EpicyclesObject::computeValue() const {
    if (inputsChangedNow() || m_terms.empty()) {
        const Field* fz = field("z");
        const expr::Resolver* res = evaluationResolver();
        double t = time();
        const int M = 512;
        m_target.clear();
        for (int j = 0; j < M; j++) {
            double u = 2 * 3.14159265358979323846 * j / M;
            expr::Slots s;
            s.u = u, s.t = t;
            s.hasU = s.hasT = true;
            expr::Value v = fz && fz->program.valid() ? fz->program.eval(res, &s) : expr::Value::fail("no formula");
            if (!v.isNumber() || !std::isfinite(v.re) || !std::isfinite(v.im))
                return expr::Value::fail("z(u) is undefined at u = " + std::to_string(u).substr(0, 5));
            m_target.push_back({v.re, v.type == expr::Type::Complex ? v.im : 0.0});
        }
        m_terms = fourier::curveCoefficients(m_target, kMaxK);
    }
    m_angle = 2 * 3.14159265358979323846 * numberOf(*this, "speed", 0.1) * time();
    fourier::cplx tip = fourier::evaluate(m_terms, m_angle, circles() + 1);
    return expr::Value::complex(tip.real(), tip.imag());
}

// --- Fourier series -------------------------------------------------------------------

FourierSeriesObject::FourierSeriesObject() : SceneObject("fourierseries") {
    setProp("original", true);
    setProp("harmonics", false);
    setProp("extend", true);
    addField("f", "f(x) =", Expect::Plot, "x");
    addField("a", "x from", Expect::Number, "-pi");
    addField("b", "x to", Expect::Number, "pi");
    addField("N", "Terms", Expect::Number, "5");
}

int FourierSeriesObject::terms() const {
    return static_cast<int>(std::clamp(std::round(numberOf(*this, "N", 5)), 0.0, 200.0));
}

expr::Value FourierSeriesObject::computeValue() const {
    std::string err = rangeError(*this, "a", "b");
    if (!err.empty()) return expr::Value::fail(err);
    const Field* f = field("f");
    const expr::Resolver* res = evaluationResolver();
    double t = time();
    double a = numberOf(*this, "a", -3.14159), b = numberOf(*this, "b", 3.14159);
    auto fn = [&](double x, double& y) { return evalNumber(f, res, t, x, 0, 0, 0, y); };
    int n = terms();
    m_series = fourier::seriesCoefficients(fn, a, b, n, std::max(2000, 40 * n), m_undefined);
    m_original.clear();
    m_sum.clear();
    m_harmonics.clear();
    const int samples = 400;
    for (int i = 0; i <= samples; i++) {
        double x = a + (b - a) * i / samples, y = 0;
        if (fn(x, y)) m_original.push_back({x, y});
        else m_original.push_back({x, std::numeric_limits<double>::quiet_NaN()});  // a gap
    }
    double lo = extended() ? a - (b - a) : a, hi = extended() ? b + (b - a) : b;
    int sumSamples = std::max(600, 30 * n) * (extended() ? 3 : 1);
    for (int i = 0; i <= sumSamples; i++) {
        double x = lo + (hi - lo) * i / sumSamples;
        m_sum.push_back({x, fourier::partialSum(m_series, x)});
    }
    for (int k = 1; k <= std::min(n, 8); k++) {
        std::vector<std::pair<double, double>> h;
        for (int i = 0; i <= 300; i++) {
            double x = a + (b - a) * i / 300;
            h.push_back({x, fourier::harmonic(m_series, k, x)});
        }
        m_harmonics.push_back(std::move(h));
    }
    return expr::Value::fail("no value");  // drawn, not used in formulas
}

} // namespace linalg::scene
