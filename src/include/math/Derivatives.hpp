#pragma once

#include "math/Integrals.hpp"
#include <functional>

namespace linalg::derivatives {

// Numerical derivatives for the tangent, tangent-plane and velocity objects (no raylib).
// Every function returns false where f is undefined near the point or the result is not finite.
using integrals::D3;
using integrals::Path;
using integrals::Surface;
// y = f(x); false where undefined
using Function = std::function<bool(double x, double& y)>;

// A step that suits a range of about `scale` units (1e-3 of it, at least 1e-6)
double stepFor(double scale);

// f′(x) by the five-point stencil (error ~ h⁴)
bool derivative1(const Function& f, double x, double h, double& out);
// f″(x) by central differences (error ~ h²)
bool derivative2(const Function& f, double x, double h, double& out);
// ∂f/∂x and ∂f/∂y of z = f(x, y), five-point stencils
bool partials(const Surface& f, double x, double y, double h, double& fx, double& fy);
// r′(u) and r″(u) of a curve, central differences
bool curveDerivatives(const Path& r, double u, double h, D3& velocity, D3& acceleration);

} // namespace linalg::derivatives
