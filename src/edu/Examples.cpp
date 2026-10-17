#include "edu/Examples.hpp"
#include "app/App.hpp"
#include "math/Quaternion.hpp"
#include "ui/Widgets.hpp"
#include <cmath>
#include <cstdio>

namespace linalg {

namespace {

constexpr float kPi = 3.14159265f;
using Fields = std::map<std::string, std::string>;

// Empty the space, show it, and hide panels that belong to another example
void fresh(App& app, AppMode mode) {
    app.setMode(mode);
    app.stopAnimation();
    app.clearSpace(mode);
    if (auto* p = app.panel("breakdown")) p->setVisible(false);
    app.resetCamera();
}

std::string num(float v) {
    char buf[32];
    formatNumber(buf, sizeof(buf), v);
    return buf;
}

Fields cells(const Matrix3D& m) {
    Fields f;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) f[scene::MatrixObject::cellKey(i, j)] = num(m.m[i][j]);
    return f;
}

// A matrix M acting on a and b, with the images M*a, M*b and its eigenvectors
void matrixScene(App& app, const Matrix3D& m) {
    fresh(app, AppMode::Space3D);
    app.addObject(AppMode::Space3D, "vector", "a", {{"x", "1"}, {"y", "0"}, {"z", "0"}});
    app.addObject(AppMode::Space3D, "vector", "b", {{"x", "0"}, {"y", "1"}, {"z", "0"}});
    app.addObject(AppMode::Space3D, "vector", "Ma", {{"value", "M*a"}}, {{"mode", "formula"}});
    app.addObject(AppMode::Space3D, "vector", "Mb", {{"value", "M*b"}}, {{"mode", "formula"}});
    app.addObject(AppMode::Space3D, "eigen", "E", {{"of", "M"}});
    app.addObject(AppMode::Space3D, "matrix", "M", cells(m));  // last: it stays selected
}

void complexScene(App& app) {
    fresh(app, AppMode::Complex);
}

void plotScene(App& app, const std::string& type, const Fields& fields, const Fields& props = {}) {
    fresh(app, AppMode::Space3D);
    app.addObject(AppMode::Space3D, type, "f", fields, props);
}

std::vector<Example> build() {
    using C = ExampleCategory;
    std::vector<Example> list;

    // --- Matrices ------------------------------------------------------------
    list.push_back({"Rotation about an axis", C::Matrices,
                    "60° about the diagonal (1, 1, 1). Lengths and angles are kept (det = 1); the axis is the "
                    "only eigenvector, with λ = 1.",
                    [](App& app) {
                        matrixScene(app, Quaternion::fromAxisAngle(Vector3D(1, 1, 1), kPi / 3).toMatrix());
                    }});
    list.push_back({"Animated rotation", C::Matrices,
                    "A parameter θ plays from 0 to 2π and drives M = rotz(θ): every object that uses M moves "
                    "with it.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "parameter", "theta", {},
                                      {{"value", "0"}, {"min", "0"}, {"max", "6.2832"}, {"playing", "1"}});
                        app.addObject(AppMode::Space3D, "matrix", "M", {{"value", "rotz(theta)"}}, {{"mode", "formula"}});
                        app.addObject(AppMode::Space3D, "vector", "a", {{"x", "2"}, {"y", "0"}, {"z", "1"}});
                        app.addObject(AppMode::Space3D, "vector", "Ma", {{"value", "M*a"}}, {{"mode", "formula"}});
                        app.select("theta");
                    }});
    list.push_back({"Non-uniform scale", C::Matrices,
                    "Stretches x by 2 and squeezes y to half. The axes are eigenvectors with λ = 2, 0.5 and 1.",
                    [](App& app) { matrixScene(app, Matrix3D({{{{2, 0, 0}}, {{0, 0.5f, 0}}, {{0, 0, 1}}}})); }});
    list.push_back({"Shear", C::Matrices,
                    "Slides layers past each other. Volume is kept (det = 1) but only the x-direction stays "
                    "on its line: a defective matrix.",
                    [](App& app) { matrixScene(app, Matrix3D({{{{1, 1, 0}}, {{0, 1, 0}}, {{0, 0, 1}}}})); }});
    list.push_back({"Projection onto a plane", C::Matrices,
                    "Flattens everything onto the xy-plane. det = 0, so it cannot be undone; the z-axis "
                    "collapses to zero (λ = 0).",
                    [](App& app) { matrixScene(app, Matrix3D({{{{1, 0, 0}}, {{0, 1, 0}}, {{0, 0, 0}}}})); }});
    list.push_back({"Mirror across x = y", C::Matrices,
                    "Swaps x and y. det = −1: orientation flips, like a reflection in a mirror.",
                    [](App& app) { matrixScene(app, Matrix3D({{{{0, 1, 0}}, {{1, 0, 0}}, {{0, 0, 1}}}})); }});
    list.push_back({"Symmetric matrix", C::Matrices,
                    "A symmetric matrix has perpendicular eigenvectors: here along (1, 1, 0), (1, −1, 0) "
                    "and the z-axis.",
                    [](App& app) { matrixScene(app, Matrix3D({{{{2, 1, 0}}, {{1, 2, 0}}, {{0, 0, 3}}}})); }});
    list.push_back({"General matrix, broken down", C::Matrices,
                    "A matrix with no obvious structure. The breakdown panel shows it as rotate, stretch, "
                    "rotate.",
                    [](App& app) {
                        matrixScene(app, Matrix3D({{{{1.5f, 0.6f, 0}}, {{0.2f, 1, 0.3f}}, {{0, -0.4f, 0.8f}}}}));
                        if (auto* p = app.panel("breakdown")) p->setVisible(true);
                        app.breakdownPanel().restart();
                    }});
    list.push_back({"Move and turn (4×4)", C::Matrices,
                    "Homogeneous coordinates: translate(2, 0, 1) moves the origin while roty turns 45° about y.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "vector", "a", {{"x", "1"}, {"y", "0"}, {"z", "0"}});
                        app.addObject(AppMode::Space3D, "vector", "Ma", {{"value", "M*a"}}, {{"mode", "formula"}});
                        app.addObject(AppMode::Space3D, "matrix", "M", {{"value", "translate(2, 0, 1) * roty(pi/4)"}},
                                      {{"mode", "formula"}});
                    }});
    list.push_back({"Projection of a vector", C::Matrices,
                    "p = (a·b / b·b) b is the shadow of a on b; r = a − p is the part perpendicular to b.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "vector", "a", {{"x", "1"}, {"y", "2"}, {"z", "2"}});
                        app.addObject(AppMode::Space3D, "vector", "b", {{"x", "3"}, {"y", "0"}, {"z", "0"}});
                        app.addObject(AppMode::Space3D, "vector", "p", {{"value", "dot(a, b)/dot(b, b) * b"}},
                                      {{"mode", "formula"}});
                        app.addObject(AppMode::Space3D, "vector", "r", {{"value", "a - p"}, {"tail", "p"}},
                                      {{"mode", "formula"}});
                        app.select("a");
                        app.select("b", AppMode::Space3D, true);
                    }});

    // --- Complex -------------------------------------------------------------
    list.push_back({"Multiply by i", C::Complex,
                    "Multiplying by i turns any number 90° counter-clockwise and keeps its length.",
                    [](App& app) {
                        complexScene(app);
                        app.addObject(AppMode::Complex, "complex", "z1", {{"value", "2 + i"}});
                        app.addObject(AppMode::Complex, "complex", "z2", {{"value", "i"}});
                        app.addObject(AppMode::Complex, "product", "w", {{"a", "z1"}, {"b", "z2"}});
                    }});
    list.push_back({"Powers of a root of unity", C::Complex,
                    "z = e^(2πi/5) and its powers z², …, z⁵: each multiplication adds 72°, and z⁵ comes back to 1.",
                    [](App& app) {
                        complexScene(app);
                        app.addObject(AppMode::Complex, "complex", "z", {{"value", "exp(2pi i/5)"}}, {{"circle", "1"}});
                        for (int k = 2; k <= 5; k++) {
                            app.addObject(AppMode::Complex, "complex", "z" + std::to_string(k),
                                          {{"value", "z^" + std::to_string(k)}});
                        }
                        app.select("z");
                    }});
    list.push_back({"Number times its conjugate", C::Complex,
                    "The angles cancel, so (2 + 1.5i)(2 − 1.5i) = |z|² = 6.25 is real.",
                    [](App& app) {
                        complexScene(app);
                        app.addObject(AppMode::Complex, "complex", "z", {{"value", "2 + 1.5i"}});
                        app.addObject(AppMode::Complex, "complex", "zbar", {{"value", "conj(z)"}});
                        app.addObject(AppMode::Complex, "product", "w", {{"a", "z"}, {"b", "zbar"}});
                    }});
    list.push_back({"Euler's identity", C::Complex,
                    "Half way round the unit circle: e^(iπ) = −1, so e^(iπ) + 1 = 0. Drag θ or press Play.",
                    [](App& app) {
                        complexScene(app);
                        app.addObject(AppMode::Complex, "parameter", "theta", {}, {{"value", "3.14159"}, {"max", "6.2832"}});
                        app.addObject(AppMode::Complex, "complex", "u", {{"value", "exp(i*theta)"}},
                                      {{"projections", "1"}, {"helix", "1"}});
                        app.select("theta");
                    }});
    list.push_back({"Rotate and scale", C::Complex,
                    "z₂ = 1.5 e^(i·30°) turns z₁ by 30° and stretches it 1.5 times.",
                    [](App& app) {
                        complexScene(app);
                        app.addObject(AppMode::Complex, "complex", "z1", {{"value", "1.5 + 0.5i"}});
                        app.addObject(AppMode::Complex, "complex", "z2", {{"value", "polar(1.5, pi/6)"}});
                        app.addObject(AppMode::Complex, "product", "w", {{"a", "z1"}, {"b", "z2"}});
                    }});

    // --- Plots ---------------------------------------------------------------
    list.push_back({"Saddle", C::Plots,
                    "z = (x² − y²)/5 curves up along x and down along y: a saddle point at the origin.",
                    [](App& app) { plotScene(app, "surface", {{"f", "(x^2 - y^2)/5"}}); }});
    list.push_back({"Ripple with a slider", C::Plots,
                    "z = sin(k·√(x² + y²)): change the parameter k to make the waves tighter.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "parameter", "k", {}, {{"value", "1"}, {"min", "0.2"}, {"max", "3"}});
                        app.addObject(AppMode::Space3D, "surface", "f",
                                      {{"f", "sin(k*sqrt(x^2 + y^2))"}, {"xmin", "-8"}, {"xmax", "8"}, {"ymin", "-8"},
                                       {"ymax", "8"}});
                        app.select("k");
                    }});
    list.push_back({"Gaussian bell", C::Plots, "z = 3e^(−(x² + y²)/4), the bell curve in two variables.",
                    [](App& app) { plotScene(app, "surface", {{"f", "3exp(-(x^2 + y^2)/4)"}}); }});
    list.push_back({"Trefoil knot", C::Plots, "A closed curve that cannot be untied without cutting it.",
                    [](App& app) {
                        plotScene(app, "curve",
                                  {{"x", "sin(t) + 2sin(2t)"}, {"y", "cos(t) - 2cos(2t)"}, {"z", "-sin(3t)"}, {"tmin", "0"},
                                   {"tmax", "2pi"}},
                                  {{"resolution", "80"}});
                    }});
    list.push_back({"Lemniscate", C::Plots, "(x² + y²)² = 8(x² − y²): a figure eight found as an implicit curve.",
                    [](App& app) {
                        plotScene(app, "implicit",
                                  {{"f", "(x^2 + y^2)^2 - 8(x^2 - y^2)"}, {"xmin", "-4"}, {"xmax", "4"}, {"ymin", "-4"},
                                   {"ymax", "4"}});
                    }});
    list.push_back({"Damped wave", C::Plots, "y = 3e^(−x²/10) cos(3x): a wave inside a shrinking envelope.",
                    [](App& app) {
                        plotScene(app, "function", {{"f", "3exp(-x^2/10) cos(3x)"}, {"xmin", "-8"}, {"xmax", "8"}},
                                  {{"resolution", "80"}});
                    }});
    return list;
}

} // namespace

const std::vector<Example>& examples() {
    static const std::vector<Example> list = build();
    return list;
}

const char* categoryName(ExampleCategory category) {
    switch (category) {
        case ExampleCategory::Matrices: return "Matrices";
        case ExampleCategory::Complex:  return "Complex";
        case ExampleCategory::Plots:    return "Plots";
    }
    return "";
}

} // namespace linalg
