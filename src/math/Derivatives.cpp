#include "math/Derivatives.hpp"
#include <algorithm>
#include <cmath>

namespace linalg::derivatives {

double stepFor(double scale) { return std::max(1e-6, 1e-3 * std::fabs(scale)); }

bool derivative1(const Function& f, double x, double h, double& out) {
    double m2, m1, p1, p2;
    if (!f(x - 2 * h, m2) || !f(x - h, m1) || !f(x + h, p1) || !f(x + 2 * h, p2)) return false;
    out = (m2 - 8 * m1 + 8 * p1 - p2) / (12 * h);
    return std::isfinite(out);
}

bool derivative2(const Function& f, double x, double h, double& out) {
    double m1, c, p1;
    if (!f(x - h, m1) || !f(x, c) || !f(x + h, p1)) return false;
    out = (p1 - 2 * c + m1) / (h * h);
    return std::isfinite(out);
}

bool partials(const Surface& f, double x, double y, double h, double& fx, double& fy) {
    Function alongX = [&](double s, double& z) { return f(s, y, z); };
    Function alongY = [&](double s, double& z) { return f(x, s, z); };
    return derivative1(alongX, x, h, fx) && derivative1(alongY, y, h, fy);
}

bool curveDerivatives(const Path& r, double u, double h, D3& velocity, D3& acceleration) {
    D3 m, c, p;
    if (!r(u - h, m) || !r(u, c) || !r(u + h, p)) return false;
    velocity = {(p.x - m.x) / (2 * h), (p.y - m.y) / (2 * h), (p.z - m.z) / (2 * h)};
    acceleration = {(p.x - 2 * c.x + m.x) / (h * h), (p.y - 2 * c.y + m.y) / (h * h), (p.z - 2 * c.z + m.z) / (h * h)};
    for (double v : {velocity.x, velocity.y, velocity.z, acceleration.x, acceleration.y, acceleration.z})
        if (!std::isfinite(v)) return false;
    return true;
}

} // namespace linalg::derivatives
