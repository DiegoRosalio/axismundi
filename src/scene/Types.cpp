#include "scene/Types.hpp"
#include <algorithm>
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
    setProp("playing", false);
    setProp("clock", 0.0);
    setProp("period", 0.0);  // formula mode: wrap the clock after this many seconds (0 = never)
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
    setProp("mode", std::string(m == Mode::Animate ? "animate" : m == Mode::Formula ? "formula" : "slider"));
}

Playback ParameterObject::playback() const {
    Playback p;
    p.load(props(), "anim.");
    p.phase = static_cast<float>(propNumber("anim.phase", 0.0));
    p.direction = propNumber("anim.direction", 1.0) < 0 ? -1 : 1;
    p.playing = playing();
    return p;
}

void ParameterObject::setPlayback(const Playback& p) {
    SceneData d;
    p.save(d, "anim.");
    for (const auto& [k, v] : d) setProp(k, v);
    setProp("anim.phase", static_cast<double>(p.phase));
    setProp("anim.direction", static_cast<double>(p.direction));
}

double ParameterObject::number() const {
    if (mode() == Mode::Formula) {
        const Field* f = field("formula");
        return (f && f->value.isRealNumber()) ? f->value.re : 0.0;
    }
    return propNumber("value", 0.0);
}

void ParameterObject::setNumber(double v) {
    double lo = minimum(), hi = maximum();
    if (lo < hi) v = std::clamp(v, lo, hi);
    double step = propNumber("step", 0.0);
    if (step > 0) v = lo + std::round((v - lo) / step) * step;
    setProp("value", v);
    if (mode() == Mode::Animate && hi > lo) {
        // Jump the animation to the matching point (inverse only for linear progress)
        Playback p = playback();
        p.phase = static_cast<float>((v - lo) / (hi - lo));
        setPlayback(p);
    }
}

void ParameterObject::restart() {
    setProp("clock", 0.0);
    Playback p = playback();
    p.phase = 0.0f;
    p.direction = 1;
    setPlayback(p);
    if (mode() == Mode::Animate) setProp("value", minimum());
    setPlaying(true);
}

void ParameterObject::advance(double dt) {
    if (!playing()) return;
    if (mode() == Mode::Formula) {
        double t = clock() + dt;
        double period = propNumber("period", 0.0);
        if (period > 0.0) t = std::fmod(t, period);
        setProp("clock", t);
        return;
    }
    if (mode() == Mode::Animate) {
        Playback p = playback();
        p.advance(static_cast<float>(dt));
        setPlayback(p);
        setPlaying(p.playing);
        double lo = minimum(), hi = maximum();
        setProp("value", lo + (hi - lo) * static_cast<double>(p.value()));
        return;
    }
    setPlaying(false);  // a plain slider does not play
}

bool ParameterObject::fieldActive(const Field& f) const {
    return f.key == "formula" ? mode() == Mode::Formula : true;
}

bool ParameterObject::provideSlots(expr::Slots& slots) const {
    slots.t = clock();
    slots.hasT = true;
    return true;
}

expr::Value ParameterObject::computeValue() const {
    if (mode() == Mode::Formula) return field("formula")->value;
    return expr::Value::scalar(propNumber("value", 0.0));
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
    setProp("resolution", 60.0);
    setProp("heatmap", true);
    switch (kind) {
        case Kind::Function:
            addField("f", "y =", Expect::Plot, "sin(x)");
            addField("xmin", "x from", Expect::Number, "-6");
            addField("xmax", "x to", Expect::Number, "6");
            break;
        case Kind::Curve:
            addField("x", "x(t) =", Expect::Plot, "2cos(t)");
            addField("y", "y(t) =", Expect::Plot, "2sin(t)");
            addField("z", "z(t) =", Expect::Plot, "t/4");
            addField("tmin", "t from", Expect::Number, "0");
            addField("tmax", "t to", Expect::Number, "4pi");
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
    if (err.empty()) err = range("tmin", "tmax");
    if (!err.empty()) return expr::Value::fail(err);
    return expr::Value::fail("no value");  // plots are drawn, not used in formulas
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
        {"function", "Function y = f(x)", "Plots", "f", spaceBit(SpaceKind::Euclidean3D),
         "The graph of a function of x",
         [] { return std::make_unique<PlotObject>(PlotObject::Kind::Function); }},
        {"curve", "Curve (x(t), y(t), z(t))", "Plots", "c", spaceBit(SpaceKind::Euclidean3D),
         "A path traced as t runs through its range",
         [] { return std::make_unique<PlotObject>(PlotObject::Kind::Curve); }},
        {"surface", "Surface z = f(x, y)", "Plots", "S", spaceBit(SpaceKind::Euclidean3D),
         "A height above every point of the floor",
         [] { return std::make_unique<PlotObject>(PlotObject::Kind::Surface); }},
        {"implicit", "Implicit curve f(x, y) = 0", "Plots", "g", spaceBit(SpaceKind::Euclidean3D),
         "All points where f(x, y) = 0",
         [] { return std::make_unique<PlotObject>(PlotObject::Kind::Implicit); }},
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
