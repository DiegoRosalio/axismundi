#include "edu/Examples.hpp"
#include "app/App.hpp"
#include "math/Quaternion.hpp"
#include <cmath>

namespace linalg {

namespace {

constexpr float kPi = 3.14159265f;

void matrices(App& app, const Matrix3D& m) {
    app.setMode(AppMode::LinearAlgebra);
    app.stopAnimation();
    app.setMatrix3(m);
    app.setVectors(Vector3D(1, 0, 0), Vector3D(0, 1, 0));
    app.resetCamera();
}

void complex(App& app, Complex z1, Complex z2, bool product, bool euler, float theta = 1.0f) {
    app.setMode(AppMode::Complex);
    ComplexScene s;
    s.z1 = z1;
    s.z2 = z2;
    s.showProduct = product;
    s.showEuler = euler;
    s.theta = theta;
    app.setComplexScene(s);
    app.resetCamera();
}

void plot(App& app, PlotSpec s) {
    app.setMode(AppMode::Plotting);
    app.setPlotSpec(s);
    app.resetCamera();
}

PlotSpec surface(const char* f, float range = 6.0f) {
    PlotSpec s;
    s.type = PlotType::Surface;
    s.f = f;
    s.xMin = s.yMin = -range;
    s.xMax = s.yMax = range;
    s.resolution = 60;
    return s;
}

std::vector<Example> build() {
    using C = ExampleCategory;
    std::vector<Example> list;

    // --- Matrices ------------------------------------------------------------
    list.push_back({"Rotation about an axis", C::Matrices,
                    "60° about the diagonal (1, 1, 1). Lengths and angles are kept (det = 1); the axis is the "
                    "only eigenvector, with λ = 1.",
                    [](App& app) {
                        matrices(app, Quaternion::fromAxisAngle(Vector3D(1, 1, 1), kPi / 3).toMatrix());
                    }});
    list.push_back({"Non-uniform scale", C::Matrices,
                    "Stretches x by 2 and squeezes y to half. The axes are eigenvectors with λ = 2, 0.5 and 1.",
                    [](App& app) { matrices(app, Matrix3D({{{{2, 0, 0}}, {{0, 0.5f, 0}}, {{0, 0, 1}}}})); }});
    list.push_back({"Shear", C::Matrices,
                    "Slides layers past each other. Volume is kept (det = 1) but only the x-direction stays "
                    "on its line: a defective matrix.",
                    [](App& app) { matrices(app, Matrix3D({{{{1, 1, 0}}, {{0, 1, 0}}, {{0, 0, 1}}}})); }});
    list.push_back({"Projection onto a plane", C::Matrices,
                    "Flattens everything onto the xy-plane. det = 0, so it cannot be undone; the z-axis "
                    "collapses to zero (λ = 0).",
                    [](App& app) { matrices(app, Matrix3D({{{{1, 0, 0}}, {{0, 1, 0}}, {{0, 0, 0}}}})); }});
    list.push_back({"Mirror across x = y", C::Matrices,
                    "Swaps x and y. det = −1: orientation flips, like a reflection in a mirror.",
                    [](App& app) { matrices(app, Matrix3D({{{{0, 1, 0}}, {{1, 0, 0}}, {{0, 0, 1}}}})); }});
    list.push_back({"Symmetric matrix", C::Matrices,
                    "A symmetric matrix has perpendicular eigenvectors: here along (1, 1, 0), (1, −1, 0) "
                    "and the z-axis.",
                    [](App& app) { matrices(app, Matrix3D({{{{2, 1, 0}}, {{1, 2, 0}}, {{0, 0, 3}}}})); }});
    list.push_back({"Quarter turn", C::Matrices,
                    "90° about z. No direction in the xy-plane stays put, so two eigenvalues are complex (±i).",
                    [](App& app) { matrices(app, Matrix3D::rotation(kPi / 2)); }});
    list.push_back({"General matrix, broken down", C::Matrices,
                    "A matrix with no obvious structure. The breakdown panel shows it as rotate, stretch, "
                    "rotate.",
                    [](App& app) {
                        matrices(app, Matrix3D({{{{1.5f, 0.6f, 0}}, {{0.2f, 1, 0.3f}}, {{0, -0.4f, 0.8f}}}}));
                        if (auto* p = app.panel("breakdown")) p->setVisible(true);
                        app.breakdownPanel().restart();
                    }});
    list.push_back({"Move and turn (4×4)", C::Matrices,
                    "Homogeneous coordinates: the fourth column moves the origin to (2, 0, 1) while the "
                    "3×3 part turns 45° about y.",
                    [](App& app) {
                        app.setMode(AppMode::LinearAlgebra);
                        app.stopAnimation();
                        app.setMatrix4(Matrix4D::translation(2, 0, 1) * Matrix4D::rotationY(kPi / 4));
                        app.resetCamera();
                    }});

    // --- Complex -------------------------------------------------------------
    list.push_back({"Multiply by i", C::Complex,
                    "Multiplying by i turns any number 90° counter-clockwise and keeps its length.",
                    [](App& app) { complex(app, Complex(2, 1), Complex(0, 1), true, false); }});
    list.push_back({"Powers of a root of unity", C::Complex,
                    "z = e^(2πi/5). Each multiplication by z adds 72°, so z₁z₂ = z² is two fifths of the "
                    "way round.",
                    [](App& app) {
                        Complex z = Complex::expi(2 * kPi / 5);
                        complex(app, z, z, true, false);
                    }});
    list.push_back({"Number times its conjugate", C::Complex,
                    "The angles cancel, so the product (2 + 1.5i)(2 − 1.5i) = |z|² = 6.25 is real.",
                    [](App& app) { complex(app, Complex(2, 1.5f), Complex(2, -1.5f), true, false); }});
    list.push_back({"Euler's identity", C::Complex,
                    "Half way round the unit circle: e^(iπ) = −1, so e^(iπ) + 1 = 0.",
                    [](App& app) { complex(app, Complex(1, 0), Complex(1, 0), false, true, kPi); }});
    list.push_back({"Rotate and scale", C::Complex,
                    "z₂ = 1.5 e^(i·30°) turns z₁ by 30° and stretches it 1.5 times.",
                    [](App& app) {
                        complex(app, Complex(1.5f, 0.5f), Complex::fromPolar(1.5f, kPi / 6), true, false);
                    }});

    // --- Plots ---------------------------------------------------------------
    list.push_back({"Saddle", C::Plots,
                    "z = (x² − y²)/5 curves up along x and down along y: a saddle point at the origin.",
                    [](App& app) { plot(app, surface("(x^2 - y^2)/5")); }});
    list.push_back({"Ripple", C::Plots, "z = sin(√(x² + y²)): circular waves spreading from the origin.",
                    [](App& app) { plot(app, surface("sin(sqrt(x^2 + y^2))", 8.0f)); }});
    list.push_back({"Gaussian bell", C::Plots,
                    "z = 3e^(−(x² + y²)/4), the bell curve in two variables.",
                    [](App& app) { plot(app, surface("3exp(-(x^2 + y^2)/4)")); }});
    list.push_back({"Trefoil knot", C::Plots, "A closed curve that cannot be untied without cutting it.",
                    [](App& app) {
                        PlotSpec s;
                        s.type = PlotType::Parametric;
                        s.px = "sin(t) + 2sin(2t)";
                        s.py = "cos(t) - 2cos(2t)";
                        s.pz = "-sin(3t)";
                        s.tMin = 0;
                        s.tMax = 2 * kPi;
                        s.resolution = 80;
                        plot(app, s);
                    }});
    list.push_back({"Lemniscate", C::Plots, "(x² + y²)² = 8(x² − y²): a figure eight found as an implicit curve.",
                    [](App& app) {
                        PlotSpec s;
                        s.type = PlotType::Implicit;
                        s.f = "(x^2 + y^2)^2 - 8(x^2 - y^2)";
                        s.xMin = s.yMin = -4;
                        s.xMax = s.yMax = 4;
                        s.resolution = 60;
                        plot(app, s);
                    }});
    list.push_back({"Damped wave", C::Plots, "y = 3e^(−x²/10) cos(3x): a wave inside a shrinking envelope.",
                    [](App& app) {
                        PlotSpec s;
                        s.type = PlotType::Function2D;
                        s.f = "3exp(-x^2/10) cos(3x)";
                        s.xMin = -8;
                        s.xMax = 8;
                        s.resolution = 80;
                        plot(app, s);
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
