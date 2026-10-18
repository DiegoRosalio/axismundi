#pragma once

#include "math/Derivatives.hpp"
#include <complex>
#include <functional>
#include <string>
#include <vector>

// Taylor polynomials and Newton's method (no raylib, unit-tested).
namespace linalg::taylor {

using cplx = std::complex<double>;
// f at a complex point: false when f can't take a complex argument (floor, sign, mod...)
using ComplexFunction = std::function<bool(cplx z, cplx& out)>;
// f at a real point: false where undefined
using RealFunction = derivatives::Function;

// T(x) = Σ c_k (x − a)^k, c_k = f⁽ᵏ⁾(a) / k!
struct Series {
    double center = 0.0;
    std::vector<double> coeffs;  // c_0 ... c_degree
    bool complexMethod = true;   // false: finite differences (degree at most 4)
    double radius = 0.0;         // the circle used by the complex method
    int requested = 0;           // the degree asked for (coeffs may be shorter)
    std::string note;            // why the degree is lower than requested, or how it was computed
};

constexpr int kMaxDegree = 20;

// The coefficients up to degree n (≤ kMaxDegree). First by the Cauchy integral formula: f sampled on a small
// circle around a in the complex plane (accurate to high degree for formulas made of sin, exp, powers...). The
// circle shrinks until two radii agree, so a singularity close to a (1/(1+x²) has them at ±i) isn't inside it.
// If f can't take complex values or isn't analytic (abs), central finite differences give degrees 0..4.
// False when f is undefined at a.
bool coefficients(const ComplexFunction& fc, const RealFunction& f, double a, int n, Series& out);

// The partial sum with the first terms up to degree n (n < 0: all)
double evaluate(const Series& s, double x, int n = -1);

// --- Newton's method: x_{k+1} = x_k − f(x_k) / f'(x_k) ----------------------------------------

struct NewtonStep {
    double x;      // x_k
    double fx;     // f(x_k)
    double slope;  // f'(x_k)
};
enum class NewtonStop {
    Steps,        // did the steps asked for
    Converged,    // x stopped changing
    FlatTangent,  // f'(x_k) = 0: the tangent never meets the x axis
    Undefined,    // f or f' undefined at x_k
    Diverging,    // x ran off (|x| > 1e6)
};
struct NewtonRun {
    std::vector<NewtonStep> steps;  // x_0 ... x_m (the last one has no step after it)
    NewtonStop stop = NewtonStop::Steps;
};
NewtonRun newton(const RealFunction& f, double x0, int maxSteps);

} // namespace linalg::taylor
