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
    app.restartTime(false);
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
    list.push_back({"Quarter turn (quaternion)", C::Matrices,
                    "q = qaxis([0, 0, 1], π/2): a quarter turn about the z axis, stored in four numbers "
                    "(0.707 + 0.707k). The gray vector v is turned into q v q⁻¹ along the dashed circle.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "quaternion", "q",
                                      {{"value", "qaxis([0, 0, 1], pi/2)"}, {"v", "[1.5, 0, 1]"}},
                                      {{"act", "1"}, {"basis", "0"}});
                        app.select("q");
                        app.setCameraView({4.2f, 3.4f, 4.8f}, {0, 0.6f, 0});
                    }});
    list.push_back({"Order matters (quaternions)", C::Matrices,
                    "q1 turns 90° about z, q2 turns 90° about x. The vector a turned by q1·q2 (q2 first) ends up "
                    "somewhere else than by q2·q1: rotations don't commute.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "quaternion", "q1", {{"value", "qaxis([0, 0, 1], pi/2)"}},
                                      {{"basis", "0"}});
                        app.addObject(AppMode::Space3D, "quaternion", "q2", {{"value", "qaxis([1, 0, 0], pi/2)"}},
                                      {{"basis", "0"}});
                        app.addObject(AppMode::Space3D, "vector", "a", {{"x", "1.5"}, {"y", "0"}, {"z", "0"}});
                        app.addObject(AppMode::Space3D, "vector", "u", {{"value", "rotate(q1*q2, a)"}},
                                      {{"mode", "formula"}});
                        app.addObject(AppMode::Space3D, "vector", "w", {{"value", "rotate(q2*q1, a)"}},
                                      {{"mode", "formula"}});
                        app.select("u");
                        app.select("w", AppMode::Space3D, true);
                        app.setCameraView({4.2f, 3.4f, 4.8f}, {0, 0.6f, 0});
                    }});
    list.push_back({"Slerp between two rotations", C::Matrices,
                    "q = slerp(a, b, s) turns smoothly from rotation a (none) to rotation b as s plays from 0 to "
                    "1 and back: the shortest way, at constant speed.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "parameter", "s", {},
                                      {{"value", "0"}, {"min", "0"}, {"max", "1"}, {"mode", "animate"},
                                       {"anim.repeat", "2"}, {"anim.duration", "3"}});
                        app.addObject(AppMode::Space3D, "quaternion", "a", {{"value", "quat(1, 0, 0, 0)"}},
                                      {{"axis", "0"}, {"basis", "0"}});
                        app.addObject(AppMode::Space3D, "quaternion", "b", {{"value", "qaxis([1, 1, 0], 2.5)"}},
                                      {{"basis", "0"}});
                        app.addObject(AppMode::Space3D, "quaternion", "q", {{"value", "slerp(a, b, s)"}});
                        app.select("q");
                        app.setCameraView({4.2f, 3.4f, 4.8f}, {0, 0.6f, 0});
                        app.restartTime(true);
                    }});
    list.push_back({"Quaternion from a matrix", C::Matrices,
                    "M = rot([1, 1, 0], θ) is a rotation matrix and q = quat(M) the same rotation as a "
                    "quaternion. Drag θ: q's axis stays (1, 1, 0)/√2 and its angle follows θ.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "parameter", "theta", {},
                                      {{"value", "1.2"}, {"min", "0"}, {"max", "3.1416"}});
                        app.addObject(AppMode::Space3D, "matrix", "M", {{"value", "rot([1, 1, 0], theta)"}},
                                      {{"mode", "formula"}});
                        app.addObject(AppMode::Space3D, "quaternion", "q", {{"value", "quat(M)"}}, {{"basis", "0"}});
                        app.select("q");
                    }});
    list.push_back({"Animated rotation", C::Matrices,
                    "A parameter θ plays from 0 to 2π and drives M = rotz(θ): every object that uses M moves "
                    "with it.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "parameter", "theta", {},
                                      {{"value", "0"}, {"min", "0"}, {"max", "6.2832"}, {"mode", "animate"}});
                        app.addObject(AppMode::Space3D, "matrix", "M", {{"value", "rotz(theta)"}}, {{"mode", "formula"}});
                        app.addObject(AppMode::Space3D, "vector", "a", {{"x", "2"}, {"y", "0"}, {"z", "1"}});
                        app.addObject(AppMode::Space3D, "vector", "Ma", {{"value", "M*a"}}, {{"mode", "formula"}});
                        app.select("theta");
                        app.restartTime(true);
                    }});
    list.push_back({"Pendulum (formula of time)", C::Matrices,
                    "θ is a formula of time: θ = π/4·cos(2t). The arm r hangs from (0, 0, 4) and swings with it; "
                    "edit the formula to change the swing.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "parameter", "theta", {{"formula", "pi/4*cos(2t)"}},
                                      {{"mode", "formula"}});
                        app.addObject(AppMode::Space3D, "vector", "r",
                                      {{"value", "[3sin(theta), 0, -3cos(theta)]"}, {"tail", "[0, 0, 4]"}},
                                      {{"mode", "formula"}});
                        app.select("theta");
                        app.restartTime(true);
                    }});
    list.push_back({"Morph to a matrix (slider)", C::Matrices,
                    "N = lerp(I, A, s) blends the identity into A as you drag the static slider s from 0 to 1. "
                    "Switch s to Animate to let the scene clock do it.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "matrix", "A",
                                      {{"value", "[[1.5, 0.8, 0], [-0.4, 1, 0], [0, 0.5, 1.2]]"}}, {{"mode", "formula"}});
                        app.addObject(AppMode::Space3D, "parameter", "s", {},
                                      {{"value", "0.35"}, {"min", "0"}, {"max", "1"}});
                        app.addObject(AppMode::Space3D, "matrix", "N", {{"value", "lerp(eye(3), A, s)"}},
                                      {{"mode", "formula"}});
                        if (auto* a = app.object("A")) a->visible = false;
                        app.select("s");
                    }});
    list.push_back({"Stretch back and forth", C::Matrices,
                    "s runs from 0.5 to 2 and back (ping-pong) with an ease-in-out curve. M = diag(s, 1/s, 1) "
                    "stretches one way and squeezes the other, so areas stay the same: det M = 1.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "parameter", "s", {},
                                      {{"value", "0.5"}, {"min", "0.5"}, {"max", "2"}, {"mode", "animate"},
                                       {"anim.repeat", "2"}, {"anim.easing", "3"}, {"anim.duration", "3"}});
                        app.addObject(AppMode::Space3D, "matrix", "M", {{"value", "scale(s, 1/s, 1)"}},
                                      {{"mode", "formula"}});
                        app.addObject(AppMode::Space3D, "vector", "a", {{"x", "1"}, {"y", "1"}, {"z", "0"}});
                        app.addObject(AppMode::Space3D, "vector", "Ma", {{"value", "M*a"}}, {{"mode", "formula"}});
                        app.select("s");
                        app.restartTime(true);
                    }});
    list.push_back({"Spinning vector", C::Matrices,
                    "v = [2cos(t), 2sin(t), 1] uses the scene time t directly: no parameter needed. M = rotz(t/2) "
                    "turns at half the speed, and Ma = M·a follows it.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "vector", "v", {{"value", "[2cos(t), 2sin(t), 1]"}},
                                      {{"mode", "formula"}});
                        app.addObject(AppMode::Space3D, "matrix", "M", {{"value", "rotz(t/2)"}}, {{"mode", "formula"}});
                        app.addObject(AppMode::Space3D, "vector", "a", {{"x", "0"}, {"y", "-2"}, {"z", "0.5"}});
                        app.addObject(AppMode::Space3D, "vector", "Ma", {{"value", "M*a"}}, {{"mode", "formula"}});
                        app.select("v");
                        app.restartTime(true);
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
    list.push_back({"Epicycles draw a heart", C::Complex,
                    "Twelve spinning circles, each turning k times per lap, add up to a point that draws a heart: "
                    "the Fourier series of the curve. Add circles to sharpen the dip.",
                    [](App& app) {
                        fresh(app, AppMode::Complex);
                        app.addObject(AppMode::Complex, "epicycles", "E");
                        app.select("E");
                        app.restartTime(true);
                    }});
    list.push_back({"Epicycles draw a square", C::Complex,
                    "Corners are hard for circles: with 8 the square is round, with 40 the corners sharpen. Change "
                    "the number of circles while it draws.",
                    [](App& app) {
                        fresh(app, AppMode::Complex);
                        app.addObject(AppMode::Complex, "epicycles", "E",
                                      {{"z", "2(cos(u) + i*sin(u))/max(abs(cos(u)), abs(sin(u)))"}, {"N", "16"},
                                       {"speed", "0.15"}});
                        app.select("E");
                        app.restartTime(true);
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
                                  {{"x", "sin(u) + 2sin(2u)"}, {"y", "cos(u) - 2cos(2u)"}, {"z", "-sin(3u)"}, {"umin", "0"},
                                   {"umax", "2pi"}},
                                  {{"density", "80"}});
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
                                  {{"density", "40"}});
                    }});
    list.push_back({"Travelling wave", C::Plots,
                    "y = sin(2x − 3t): t is the scene time, so the wave moves right at speed 3/2 while the clock "
                    "plays. Any formula can use t.",
                    [](App& app) {
                        plotScene(app, "function", {{"f", "sin(2x - 3t)"}, {"xmin", "-8"}, {"xmax", "8"}},
                                  {{"density", "40"}});
                        app.restartTime(true);
                    }});
    list.push_back({"Spinning curve", C::Plots,
                    "A circle whose height ripples: z = sin(3u − 2t). u runs along the curve, t is the scene "
                    "time, so the ripple travels around.",
                    [](App& app) {
                        plotScene(app, "curve",
                                  {{"x", "3cos(u)"}, {"y", "3sin(u)"}, {"z", "sin(3u - 2t)"}, {"umin", "0"},
                                   {"umax", "2pi"}});
                        app.restartTime(true);
                    }});
    list.push_back({"Square wave from sines", C::Plots,
                    "A square wave rebuilt from sine waves: S = (4/π)(sin x + sin 3x/3 + sin 5x/5 + …). Drag n to "
                    "add terms; the spectrum in the inspector shows only odd k, falling like 1/k.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "parameter", "n", {},
                                      {{"value", "5"}, {"min", "1"}, {"max", "40"}, {"step", "1"}});
                        app.addObject(AppMode::Space3D, "fourierseries", "S",
                                      {{"f", "sign(x)"}, {"N", "n"}}, {{"harmonics", "1"}});
                        app.select("S");
                        app.setCameraView({0.0f, 9.0f, 6.0f}, {0, 0, 0});
                    }});
    list.push_back({"Gibbs overshoot", C::Plots,
                    "With 60 terms the sum hugs the sawtooth everywhere except at the jumps, where it overshoots by "
                    "about 9% however many terms you take (the Gibbs phenomenon).",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "fourierseries", "S", {{"f", "x"}, {"N", "60"}});
                        app.select("S");
                        app.setCameraView({0.0f, 9.0f, 6.0f}, {0, 0, 0});
                    }});
    list.push_back({"Secant becomes tangent", C::Plots,
                    "The gray secant joins x0 and x0 + h. As the clock shrinks h toward 0 it turns into the tangent "
                    "(black): the derivative is the limit of these slopes.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "tangent", "T",
                                      {{"f", "x^3/6 - x"}, {"x0", "1"}, {"h", "2.5(1 - mod(t, 5)/5)"},
                                       {"xmin", "-4"}, {"xmax", "4"}},
                                      {{"derivative", "0"}, {"triangle", "0"}});
                        app.select("T");
                        app.setCameraView({0.0f, 10.0f, 5.5f}, {0, 0, -1.0f});
                        app.restartTime(true);
                    }});
    list.push_back({"Slope as a function: f and f'", C::Plots,
                    "The point slides along f; the second curve is f', the slope at every x. Where f has a peak or "
                    "a valley the tangent is flat and f' crosses zero.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "tangent", "T",
                                      {{"f", "x^3/6 - x"}, {"x0", "3sin(t/2)"}, {"xmin", "-4"}, {"xmax", "4"}});
                        app.select("T");
                        app.setCameraView({0.0f, 10.0f, 5.5f}, {0, 0, -1.0f});
                        app.restartTime(true);
                    }});
    list.push_back({"Gradient points uphill", C::Plots,
                    "On a hill z = 3e^(−(x² + y²)/4) the gradient arrow on the floor always points to the top, "
                    "longest where the slope is steepest. The tangent plane touches the hill at the point.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "tangentplane", "G",
                                      {{"f", "3exp(-(x^2 + y^2)/4)"}, {"x0", "2cos(t/2)"}, {"y0", "2sin(t/2)"},
                                       {"xmin", "-4"}, {"xmax", "4"}, {"ymin", "-4"}, {"ymax", "4"}});
                        app.select("G");
                        app.setCameraView({6.5f, 6.5f, 8.0f}, {0, 1.0f, 0});
                        app.restartTime(true);
                    }});
    list.push_back({"Velocity on a helix", C::Plots,
                    "r' follows the helix at constant speed, so the acceleration r'' never speeds it up: it only "
                    "turns, pointing straight at the axis.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "velocity", "R",
                                      {{"x", "2cos(u)"}, {"y", "2sin(u)"}, {"z", "u/4"}, {"umin", "0"},
                                       {"umax", "4pi"}, {"u0", "mod(t, 4pi)"}});
                        app.select("R");
                        app.setCameraView({5.5f, 5.0f, 6.5f}, {0, 1.4f, 0});
                        app.restartTime(true);
                    }});
    list.push_back({"Speeding up vs turning", C::Plots,
                    "Around an ellipse the speed changes: the acceleration splits into a part along the motion "
                    "(speeding up or slowing down) and a part across it (turning).",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "velocity", "R", {{"u0", "mod(t/2, 2pi)"}}, {{"split", "1"}});
                        app.select("R");
                        app.setCameraView({0.0f, 9.0f, 5.0f}, {0, 0, 0});
                        app.restartTime(true);
                    }});
    list.push_back({"sin(x) term by term", C::Plots,
                    "Every second the Taylor polynomial of sin about 0 gains a term: x, x − x³/6, ... Each one fits "
                    "sin a little farther from the center. The faint curve is the previous degree.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "taylor", "p",
                                      {{"f", "sin(x)"}, {"a", "0"}, {"N", "min(floor(t), 15)"}, {"xmin", "-8"},
                                       {"xmax", "8"}});
                        app.select("p");
                        app.setCameraView({0.0f, 12.0f, 6.5f}, {0, 0, -1.0f});
                        app.restartTime(true);
                    }});
    list.push_back({"e^x around a moving center", C::Plots,
                    "A degree-3 Taylor polynomial of e^x whose center slides back and forth: it always fits best near "
                    "its center.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "taylor", "p",
                                      {{"f", "exp(x)"}, {"a", "1.5sin(t/2)"}, {"N", "3"}, {"xmin", "-4"}, {"xmax", "3"}});
                        app.select("p");
                        app.setCameraView({0.0f, 10.0f, 5.5f}, {0, 0, -1.5f});
                        app.restartTime(true);
                    }});
    list.push_back({"Where the series stops working", C::Plots,
                    "1/(1 + x²) is smooth everywhere on the line, yet its Taylor polynomials about 0 only fit for "
                    "|x| < 1: more terms make them worse beyond. The reason is off the line: 1 + x² = 0 at x = ±i.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "taylor", "p",
                                      {{"f", "1/(1 + x^2)"}, {"a", "0"}, {"N", "min(2floor(t), 20)"}, {"xmin", "-2.5"},
                                       {"xmax", "2.5"}});
                        app.select("p");
                        app.setCameraView({0.0f, 6.0f, 3.5f}, {0, 0, -0.5f});
                        app.restartTime(true);
                    }});
    list.push_back({"Newton finds √2", C::Plots,
                    "Newton's method on x² − 2 from x0 = 3: each step follows the tangent down to the x axis. The "
                    "steps land closer and closer to √2 ≈ 1.41421, the correct digits doubling each time.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "newton", "r",
                                      {{"f", "x^2 - 2"}, {"x0", "3"}, {"steps", "min(floor(t), 6)"}, {"xmin", "-1"},
                                       {"xmax", "3.2"}});
                        app.select("r");
                        app.setCameraView({1.2f, 13.0f, 2.0f}, {1.2f, 0, -3.2f});
                        app.restartTime(true);
                    }});
    list.push_back({"Newton overshoots", C::Plots,
                    "On atan(x) from x0 = 1.5 the tangents are too flat: each step lands farther away on the other "
                    "side, and the method runs off. Start nearer 0 and it works.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "newton", "r",
                                      {{"f", "atan(x)"}, {"x0", "1.5"}, {"steps", "min(floor(t), 3)"}, {"xmin", "-7"},
                                       {"xmax", "7"}});
                        app.select("r");
                        app.setCameraView({0.0f, 11.0f, 5.5f}, {0, 0, -0.5f});
                        app.restartTime(true);
                    }});
    list.push_back({"Frenet frame on a helix", C::Plots,
                    "T (red), N (green), B (blue) ride a helix as u0 follows the clock. A helix bends and twists "
                    "the same everywhere: κ and τ stay constant, and N always points at the axis.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "frame", "P", {{"u0", "mod(t, 4pi)"}});
                        app.select("P");
                        app.setCameraView({5.5f, 5.0f, 6.5f}, {0, 1.4f, 0});
                        app.restartTime(true);
                    }});
    list.push_back({"Curvature of an ellipse", C::Plots,
                    "The comb shows κ along the ellipse: the teeth are longest at the ends of the long axis, where "
                    "it turns sharply, and shortest at the flat sides. Drag u0.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "parameter", "s", {},
                                      {{"value", "0.5"}, {"min", "0"}, {"max", "6.2832"}});
                        app.addObject(AppMode::Space3D, "frame", "P",
                                      {{"x", "3cos(u)"}, {"y", "1.5sin(u)"}, {"z", "0"}, {"umin", "0"}, {"umax", "2pi"},
                                       {"u0", "s"}},
                                      {{"comb", "1"}});
                        app.select("P");
                        app.setCameraView({0.0f, 9.0f, 5.0f}, {0, 0, 0});
                    }});
    list.push_back({"Osculating circle on a parabola", C::Plots,
                    "The circle that hugs y = x²/2 best: radius 1 at the vertex, growing as the parabola "
                    "straightens out. u0 swings with the clock.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "frame", "P",
                                      {{"x", "u"}, {"y", "u^2/2"}, {"z", "0"}, {"umin", "-3"}, {"umax", "3"},
                                       {"u0", "2sin(t/2)"}});
                        app.select("P");
                        app.setCameraView({0.0f, 10.0f, 5.5f}, {0, 0, -1.5f});
                        app.restartTime(true);
                    }});
    list.push_back({"Volume under a paraboloid", C::Plots,
                    "V = ∫∫ (4 − x² − y²) dA over the square [−1, 1]²: the boxes under the surface add up to "
                    "almost the exact volume 40/3 ≈ 13.33.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "doubleintegral", "V",
                                      {{"f", "4 - x^2 - y^2"}, {"xmin", "-1"}, {"xmax", "1"}, {"ymin", "-1"},
                                       {"ymax", "1"}, {"n", "6"}});
                        app.select("V");
                        app.setCameraView({4.2f, 4.0f, 5.0f}, {0, 1.2f, 0});
                    }});
    list.push_back({"Riemann boxes refine", C::Plots,
                    "n = round(1 + 19s) boxes per side while s plays: the sum of the boxes closes in on the exact "
                    "volume under z = 1 + sin(x)cos(y).",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "parameter", "s", {},
                                      {{"value", "0"}, {"min", "0"}, {"max", "1"}, {"mode", "animate"},
                                       {"anim.repeat", "2"}, {"anim.duration", "6"}});
                        app.addObject(AppMode::Space3D, "doubleintegral", "V",
                                      {{"f", "1 + sin(x)cos(y)"}, {"xmin", "-3"}, {"xmax", "3"}, {"ymin", "-3"},
                                       {"ymax", "3"}, {"n", "round(1 + 19s)"}});
                        app.select("V");
                        app.restartTime(true);
                    }});
    list.push_back({"Work around a circle", C::Plots,
                    "The rotation field F = [−y, x, 0] pushes along the circle everywhere (all green): the work "
                    "around it is 2π·r² = 8π. The marker travels the path with the clock.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "lineintegral", "W", {{"upto", "mod(t/8, 1)"}});
                        app.addObject(AppMode::Space3D, "field", "F", {{"F", "[-y, x, 0]"}}, {{"density", "0.75"}});
                        app.select("W");
                        app.setCameraView({0.0f, 9.0f, 6.0f}, {0, 0, 0});
                        app.restartTime(true);
                    }});
    list.push_back({"Path independence", C::Plots,
                    "F = [y·z, x·z, x·y] is the gradient of x·y·z, so its work from (0,0,0) to (1,1,1) is 1 on "
                    "any path: the straight W1 and the wiggly W2 agree.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "lineintegral", "W1",
                                      {{"F", "[y*z, x*z, x*y]"}, {"x", "u"}, {"y", "u"}, {"z", "u"}, {"umin", "0"},
                                       {"umax", "1"}});
                        app.addObject(AppMode::Space3D, "lineintegral", "W2",
                                      {{"F", "[y*z, x*z, x*y]"}, {"x", "u"}, {"y", "u^2"}, {"z", "u + sin(pi*u)"},
                                       {"umin", "0"}, {"umax", "1"}});
                        app.select("W1");
                        app.select("W2", AppMode::Space3D, true);
                        app.setCameraView({2.6f, 2.4f, 3.2f}, {0.5f, 0.6f, -0.5f});
                    }});
    list.push_back({"Rotation field", C::Plots,
                    "F = [−y, x, 0] turns around the origin: arrows grow with the distance, and the flow lines "
                    "are circles.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "field", "F", {{"F", "[-y, x, 0]"}}, {{"lines", "1"}});
                        app.setCameraView({0.0f, 10.0f, 6.5f}, {0, 0, 0});
                    }});
    list.push_back({"Source and sink", C::Plots,
                    "A source at (−2, 0) and a sink at (2, 0): every flow line leaves the source and ends in the "
                    "sink, like an electric dipole.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "field", "F",
                                      {{"F", "([x + 2, y, 0]/((x + 2)^2 + y^2) - [x - 2, y, 0]/((x - 2)^2 + y^2))"}},
                                      {{"lines", "1"}, {"same_length", "1"}, {"seeds", "8"}});
                        app.setCameraView({0.0f, 10.0f, 6.5f}, {0, 0, 0});
                    }});
    list.push_back({"Flow with particles", C::Plots,
                    "A vortex that drains into the origin. Particles drift along the flow lines while the clock "
                    "plays: watch them spiral in.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "field", "F", {{"F", "[-y - x/4, x - y/4, 0]"}},
                                      {{"lines", "1"}, {"particles", "1"}, {"seeds", "5"}});
                        app.setCameraView({0.0f, 10.0f, 6.5f}, {0, 0, 0});
                        app.restartTime(true);
                    }});
    list.push_back({"3D swirl", C::Plots,
                    "F = [−y, x, 0.4] in a box: every flow line is a helix around the z axis.",
                    [](App& app) {
                        fresh(app, AppMode::Space3D);
                        app.addObject(AppMode::Space3D, "field", "F", {{"F", "[-y, x, 0.4]"}},
                                      {{"domain", "volume"}, {"lines", "1"}, {"seeds", "3"}, {"density", "0.75"}});
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
