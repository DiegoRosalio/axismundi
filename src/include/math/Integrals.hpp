#pragma once

#include <functional>
#include <vector>

namespace linalg::integrals {

// Numerical integrals for the calculus objects (no raylib): Riemann boxes under z = f(x, y), a precise
// double integral, and line integrals of a vector field along a curve.

struct D3 {
    double x = 0, y = 0, z = 0;
};

// z = f(x, y); false where f is undefined (the box is then left out)
using Surface = std::function<bool(double x, double y, double& z)>;
// F(p); false where undefined
using Field = std::function<bool(const D3& p, D3& out)>;
// r(u); false where undefined
using Path = std::function<bool(double u, D3& out)>;

struct Rect {
    double x0 = -1, x1 = 1, y0 = -1, y1 = 1;
};

enum class Rule { Left, Middle, Right };  // where each box samples f (lower-left corner, center, upper-right)

struct Box {
    double x0, x1, y0, y1;  // the cell on the floor
    double height;          // f at the sample point (negative: below the floor)
};

struct RiemannSum {
    std::vector<Box> boxes;
    double sum = 0.0;     // Σ height · area
    int skipped = 0;      // cells where f was undefined
};

RiemannSum riemann2D(const Surface& f, const Rect& r, int nx, int ny, Rule rule);

// Composite Simpson's rule on an n × n grid (n made even): the "exact" value to compare with.
// ok is false when f is undefined somewhere on the grid.
double simpson2D(const Surface& f, const Rect& r, int n, bool& ok);

// ∫ F · dr along r(u) for u from a to a + upto·(b − a) (upto in [0, 1]), by Simpson on F(r(u)) · r′(u)
// with r′ by central differences. ok is false when F or r is undefined on the way.
double lineIntegral(const Field& F, const Path& r, double a, double b, double upto, int n, bool& ok);

// r′(u) by central differences (step scaled to the parameter range)
bool derivative(const Path& r, double u, double h, D3& out);

} // namespace linalg::integrals
