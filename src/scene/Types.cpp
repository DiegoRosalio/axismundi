#include "scene/Types.hpp"
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cstdio>

namespace linalg::scene {

// --- Parameter -------------------------------------------------------------------

ParameterObject::ParameterObject() : SceneObject("parameter") {
    setProp("mode", std::string("slider"));
    setProp("value", 1.0);
    setProp("min", 0.0);
    setProp("max", 6.2832);
    setProp("step", 0.0);
    setProp("anim.start", 0.0);
    Playback p;
    p.duration = 6.0f;
    setPlayback(p);
    addField("formula", "Value =", Expect::Number, "sin(t)");
}

ParameterObject::Mode ParameterObject::mode() const {
    std::string m = prop("mode", "slider");
    if (m == "animate") return Mode::Animate;
    if (m == "formula") return Mode::Formula;
    return Mode::Slider;
}

void ParameterObject::setMode(Mode m) {
    // Leaving a time-driven mode keeps the current value on the slider
    if (m == Mode::Slider && mode() != Mode::Slider) setProp("value", number());
    setProp("mode", std::string(m == Mode::Animate ? "animate" : m == Mode::Formula ? "formula" : "slider"));
}

Playback ParameterObject::playback() const {
    Playback p;
    p.load(props(), "anim.");
    return p;
}

void ParameterObject::setPlayback(const Playback& p) {
    SceneData d;
    p.save(d, "anim.");
    for (const auto& [k, v] : d) setProp(k, v);
}

double ParameterObject::animatedValue(double seconds) const {
    double lo = minimum(), hi = maximum();
    return lo + (hi - lo) * static_cast<double>(playback().valueAt(seconds - startDelay()));
}

double ParameterObject::number() const {
    switch (mode()) {
        case Mode::Formula: {
            const Field* f = field("formula");
            return (f && f->value.isRealNumber()) ? f->value.re : 0.0;
        }
        case Mode::Animate: return animatedValue(time());
        case Mode::Slider: break;
    }
    return propNumber("value", 0.0);
}

void ParameterObject::setNumber(double v) {
    double lo = minimum(), hi = maximum();
    if (lo < hi) v = std::clamp(v, lo, hi);
    double step = propNumber("step", 0.0);
    if (step > 0) v = lo + std::round((v - lo) / step) * step;
    setProp("value", v);
}

bool ParameterObject::fieldActive(const Field& f) const {
    return f.key == "formula" ? mode() == Mode::Formula : true;
}

expr::Value ParameterObject::computeValue() const {
    if (mode() == Mode::Formula) return field("formula")->value;
    return expr::Value::scalar(number());
}

// --- Vector -----------------------------------------------------------------------

VectorObject::VectorObject() : SceneObject("vector") {
    setProp("mode", std::string("components"));
    addField("x", "x", Expect::Number, "1");
    addField("y", "y", Expect::Number, "0");
    addField("z", "z", Expect::Number, "0");
    addField("value", "Formula", Expect::Vector, "[1, 0, 0]");
    addField("tail", "Starts at", Expect::Vector, "[0, 0, 0]");
}

void VectorObject::setByFormula(bool formula) {
    setProp("mode", std::string(formula ? "formula" : "components"));
}

bool VectorObject::fieldActive(const Field& f) const {
    if (f.key == "tail") return true;
    if (f.key == "value") return byFormula();
    return !byFormula();
}

Vector3D VectorObject::tail() const {
    const Field* f = field("tail");
    if (f && f->value.type == expr::Type::Vector) return f->value.vector;
    return Vector3D::zero();
}

expr::Value VectorObject::computeValue() const {
    if (byFormula()) return field("value")->value;
    return expr::Value::vec(Vector3D(static_cast<float>(field("x")->value.re), static_cast<float>(field("y")->value.re),
                                     static_cast<float>(field("z")->value.re)));
}

// --- Matrix -----------------------------------------------------------------------

std::string MatrixObject::cellKey(int row, int col) {
    return "m" + std::to_string(row) + std::to_string(col);
}

MatrixObject::MatrixObject() : SceneObject("matrix") {
    setProp("size", 3.0);
    setProp("mode", std::string("cells"));
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::string label = "row " + std::to_string(i + 1) + ", column " + std::to_string(j + 1);
            addField(cellKey(i, j), label, Expect::Number, i == j ? "1" : "0");
        }
    }
    addField("value", "Formula", Expect::Matrix, "eye(3)");
}

void MatrixObject::setSize(int n) {
    setProp("size", static_cast<double>(n == 4 ? 4 : 3));
}

void MatrixObject::setCells(const Matrix4D& m, int n) {
    setByFormula(false);
    setSize(n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            float v = m.m[i][j];
            if (std::fabs(v) < 1e-6f) v = 0.0f;  // no "-0" or float noise in the cells
            char buf[32];
            snprintf(buf, sizeof(buf), "%.6g", v);
            setField(cellKey(i, j), buf);
        }
}

void MatrixObject::applyPreset(const Matrix3D& p) {
    Matrix4D P = Matrix4D::identity();
    bool identity = true;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            P.m[i][j] = p.m[i][j];
            if (std::fabs(p.m[i][j] - (i == j ? 1.0f : 0.0f)) > 1e-9f) identity = false;
        }
    int n = size();
    const expr::Value& v = value();
    if (v.ok() && v.type == expr::Type::Matrix) n = v.size;
    if (identity) {
        setCells(Matrix4D::identity(), n);
        return;
    }
    Matrix4D current = Matrix4D::identity();
    if (v.ok() && v.type == expr::Type::Matrix) {
        for (int i = 0; i < v.size; i++)
            for (int j = 0; j < v.size; j++) current.m[i][j] = v.matrix.m[i][j];
    }
    setCells(P * current, n);
}

void MatrixObject::setByFormula(bool formula) {
    setProp("mode", std::string(formula ? "formula" : "cells"));
}

bool MatrixObject::fieldActive(const Field& f) const {
    if (f.key == "value") return byFormula();
    if (byFormula()) return false;
    int n = size();
    int row = f.key[1] - '0', col = f.key[2] - '0';
    return row < n && col < n;
}

expr::Value MatrixObject::computeValue() const {
    if (byFormula()) return field("value")->value;
    int n = size();
    Matrix4D m = Matrix4D::identity();
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) m.m[i][j] = static_cast<float>(field(cellKey(i, j))->value.re);
    if (n == 4) return expr::Value::mat4(m);
    Matrix3D m3;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) m3.m[i][j] = m.m[i][j];
    return expr::Value::mat3(m3);
}

// --- Eigenvectors -------------------------------------------------------------------

EigenObject::EigenObject() : SceneObject("eigen") {
    addField("of", "Matrix", Expect::Matrix, "M");
}

expr::Value EigenObject::computeValue() const {
    return field("of")->value;
}

// --- Plots --------------------------------------------------------------------------

PlotObject::PlotObject(Kind kind) : SceneObject(kind == Kind::Function ? "function"
                                                : kind == Kind::Curve  ? "curve"
                                                : kind == Kind::Surface ? "surface"
                                                                        : "implicit"),
                                    m_kind(kind) {
    setProp("density", densityRange(kind).defaultValue);
    setProp("heatmap", true);
    switch (kind) {
        case Kind::Function:
            addField("f", "y =", Expect::Plot, "sin(x)");
            addField("xmin", "x from", Expect::Number, "-6");
            addField("xmax", "x to", Expect::Number, "6");
            break;
        case Kind::Curve:
            // u is the curve's own variable; t stays the scene time
            addField("x", "x(u) =", Expect::Plot, "2cos(u)");
            addField("y", "y(u) =", Expect::Plot, "2sin(u)");
            addField("z", "z(u) =", Expect::Plot, "u/4");
            for (const char* k : {"x", "y", "z"}) markCurveVariable(k);
            addField("umin", "u from", Expect::Number, "0");
            addField("umax", "u to", Expect::Number, "4pi");
            break;
        case Kind::Surface:
            addField("f", "z =", Expect::Plot, "sin(sqrt(x^2 + y^2))");
            addField("xmin", "x from", Expect::Number, "-6");
            addField("xmax", "x to", Expect::Number, "6");
            addField("ymin", "y from", Expect::Number, "-6");
            addField("ymax", "y to", Expect::Number, "6");
            break;
        case Kind::Implicit:
            addField("f", "f(x, y) =", Expect::Plot, "x^2 + y^2 - 9");
            addField("xmin", "x from", Expect::Number, "-5");
            addField("xmax", "x to", Expect::Number, "5");
            addField("ymin", "y from", Expect::Number, "-5");
            addField("ymax", "y to", Expect::Number, "5");
            break;
    }
}

PlotObject::DensityRange PlotObject::densityRange(Kind kind) {
    switch (kind) {
        case Kind::Function: return {2, 200, 30, 20000};
        case Kind::Curve: return {5, 200, 40, 20000};
        case Kind::Surface: return {1, 40, 6, 400};
        case Kind::Implicit: return {2, 60, 10, 800};
    }
    return {1, 100, 10, 1000};
}

double PlotObject::density() const {
    DensityRange r = densityRange(m_kind);
    return std::clamp(propNumber("density", r.defaultValue), r.min, r.max);
}

void PlotObject::setDensity(double perUnit) {
    DensityRange r = densityRange(m_kind);
    setProp("density", std::clamp(perUnit, r.min, r.max));
}

PlotObject::Samples PlotObject::samples() const {
    DensityRange r = densityRange(m_kind);
    double d = density();
    auto count = [&](const char* lo, const char* hi, double a, double b, int minimum) {
        double span = rangeValue(hi, b) - rangeValue(lo, a);
        if (!(span > 0.0) || !std::isfinite(span)) span = 1.0;
        double n = std::ceil(span * d);
        return static_cast<int>(std::clamp(n, static_cast<double>(minimum), 1e7));
    };
    Samples s;
    switch (m_kind) {
        case Kind::Function: s.wantedX = count("xmin", "xmax", -5, 5, 8); break;
        case Kind::Curve: s.wantedX = count("umin", "umax", 0, 6.28, 8); break;
        case Kind::Surface:
        case Kind::Implicit:
            s.wantedX = count("xmin", "xmax", -5, 5, 2);
            s.wantedY = count("ymin", "ymax", -5, 5, 2);
            break;
    }
    s.nx = std::min(s.wantedX, r.cap);
    s.ny = std::min(s.wantedY, r.cap);
    s.capped = s.nx < s.wantedX || s.ny < s.wantedY;
    return s;
}

void PlotObject::convertOldResolution() {
    auto it = props().find("resolution");
    if (it == props().end()) return;
    double res = std::atof(it->second.c_str());
    if (res <= 0) res = 60;
    auto span = [&](const char* lo, const char* hi, double a, double b) {
        double v = rangeValue(hi, b) - rangeValue(lo, a);
        return v > 0 ? v : 1.0;
    };
    // The old sample counts for each kind, spread over the range
    double perUnit = 0;
    switch (m_kind) {
        case Kind::Function: perUnit = std::max(100.0, res * 6) / span("xmin", "xmax", -5, 5); break;
        case Kind::Curve: perUnit = std::max(200.0, res * 10) / span("umin", "umax", 0, 6.28); break;
        case Kind::Surface:
            perUnit = std::clamp(res, 4.0, 200.0) / std::max(span("xmin", "xmax", -5, 5), span("ymin", "ymax", -5, 5));
            break;
        case Kind::Implicit:
            perUnit = std::clamp(res * 2, 20.0, 300.0) / std::max(span("xmin", "xmax", -5, 5), span("ymin", "ymax", -5, 5));
            break;
    }
    setDensity(std::round(perUnit * 10) / 10);
    removeProp("resolution");
}

double PlotObject::rangeValue(const char* key, double fallback) const {
    const Field* f = field(key);
    if (!f || !f->value.isRealNumber()) return fallback;
    return f->value.re;
}

expr::Value PlotObject::computeValue() const {
    auto range = [&](const char* lo, const char* hi) -> std::string {
        if (!field(lo)) return "";
        if (!(rangeValue(lo, 0) < rangeValue(hi, 0))) return std::string(field(lo)->label) + " must be less than " + field(hi)->label;
        return "";
    };
    std::string err = range("xmin", "xmax");
    if (err.empty()) err = range("ymin", "ymax");
    if (err.empty()) err = range("umin", "umax");
    if (!err.empty()) return expr::Value::fail(err);
    return expr::Value::fail("no value");  // plots are drawn, not used in formulas
}

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

// --- Complex ------------------------------------------------------------------------

ComplexObject::ComplexObject() : SceneObject("complex") {
    addField("value", "Value", Expect::ComplexNum, "1 + i");
    setProp("circle", false);
    setProp("projections", false);
    setProp("helix", false);
}

expr::Value ComplexObject::computeValue() const {
    const expr::Value& v = field("value")->value;
    return expr::Value::complex(v.re, v.type == expr::Type::Complex ? v.im : 0.0);
}

ProductObject::ProductObject() : SceneObject("product") {
    addField("a", "First factor", Expect::ComplexNum, "z1");
    addField("b", "Second factor", Expect::ComplexNum, "z2");
    setProp("angles", true);
}

expr::Value ProductObject::computeValue() const {
    const expr::Value& a = field("a")->value;
    const expr::Value& b = field("b")->value;
    double ar = a.re, ai = a.type == expr::Type::Complex ? a.im : 0.0;
    double br = b.re, bi = b.type == expr::Type::Complex ? b.im : 0.0;
    return expr::Value::complex(ar * br - ai * bi, ar * bi + ai * br);
}

// --- Registry -----------------------------------------------------------------------

const std::vector<ObjectTypeInfo>& objectTypes() {
    static const std::vector<ObjectTypeInfo> types = {
        {"parameter", "Parameter", "Basics", "s", spaceBit(SpaceKind::Euclidean3D) | spaceBit(SpaceKind::Complex),
         "A named number: a slider, an animation or a formula of time t",
         [] { return std::make_unique<ParameterObject>(); }},
        {"vector", "Vector", "Basics", "v", spaceBit(SpaceKind::Euclidean3D),
         "An arrow given by components or by a formula such as a + b",
         [] { return std::make_unique<VectorObject>(); }},
        {"matrix", "Matrix", "Basics", "M", spaceBit(SpaceKind::Euclidean3D),
         "A 3×3 or 4×4 transformation, by cells or by a formula such as rotz(theta)",
         [] { return std::make_unique<MatrixObject>(); }},
        {"eigen", "Eigenvectors", "Linear algebra", "E", spaceBit(SpaceKind::Euclidean3D),
         "The directions a matrix only stretches",
         [] { return std::make_unique<EigenObject>(); }},
        {"quaternion", "Quaternion (rotation)", "Linear algebra", "q", spaceBit(SpaceKind::Euclidean3D),
         "A rotation in four numbers: an axis and an angle, e.g. qaxis([0, 0, 1], pi/2)",
         [] { return std::make_unique<QuaternionObject>(); }},
        {"function", "Function y = f(x)", "Plots", "f", spaceBit(SpaceKind::Euclidean3D),
         "The graph of a function of x",
         [] { return std::make_unique<PlotObject>(PlotObject::Kind::Function); }},
        {"curve", "Curve (x(u), y(u), z(u))", "Plots", "c", spaceBit(SpaceKind::Euclidean3D),
         "A path traced as u runs through its range (t is still the scene time)",
         [] { return std::make_unique<PlotObject>(PlotObject::Kind::Curve); }},
        {"surface", "Surface z = f(x, y)", "Plots", "S", spaceBit(SpaceKind::Euclidean3D),
         "A height above every point of the floor",
         [] { return std::make_unique<PlotObject>(PlotObject::Kind::Surface); }},
        {"implicit", "Implicit curve f(x, y) = 0", "Plots", "g", spaceBit(SpaceKind::Euclidean3D),
         "All points where f(x, y) = 0",
         [] { return std::make_unique<PlotObject>(PlotObject::Kind::Implicit); }},
        {"field", "Vector field F(x, y, z)", "Fields", "F", spaceBit(SpaceKind::Euclidean3D),
         "Arrows of F at every point, with flow lines and moving particles",
         [] { return std::make_unique<VectorFieldObject>(); }},
        {"doubleintegral", "Volume under z = f(x, y)", "Calculus", "V", spaceBit(SpaceKind::Euclidean3D),
         "A double integral: Riemann boxes next to the exact volume",
         [] { return std::make_unique<DoubleIntegralObject>(); }},
        {"lineintegral", "Work ∫ F·dr along a curve", "Calculus", "W", spaceBit(SpaceKind::Euclidean3D),
         "A line integral: how much a vector field pushes along a path",
         [] { return std::make_unique<LineIntegralObject>(); }},
        {"complex", "Complex number", "Basics", "z", spaceBit(SpaceKind::Complex),
         "A point of the complex plane, such as 2 + i or exp(i*theta)",
         [] { return std::make_unique<ComplexObject>(); }},
        {"product", "Product z₁·z₂", "Operations", "w", spaceBit(SpaceKind::Complex),
         "A product drawn as a rotation plus a scaling",
         [] { return std::make_unique<ProductObject>(); }},
    };
    return types;
}

const ObjectTypeInfo* findType(const std::string& id) {
    for (const auto& t : objectTypes())
        if (id == t.id) return &t;
    return nullptr;
}

std::unique_ptr<SceneObject> createObject(const std::string& id) {
    const ObjectTypeInfo* t = findType(id);
    return t ? t->create() : nullptr;
}

} // namespace linalg::scene
