#include "math/CurveGeometry.hpp"
#include <cmath>

namespace linalg::geometry {

D3 add(const D3& a, const D3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
D3 sub(const D3& a, const D3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
D3 scale(const D3& a, double k) { return {a.x * k, a.y * k, a.z * k}; }
double dot(const D3& a, const D3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
D3 cross(const D3& a, const D3& b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
double length(const D3& a) { return std::sqrt(dot(a, a)); }

bool frenet(const Path& r, double u, double h, Frame& out) {
    D3 m2, m1, c, p1, p2;
    if (!r(u - 2 * h, m2) || !r(u - h, m1) || !r(u, c) || !r(u + h, p1) || !r(u + 2 * h, p2)) return false;
    // Central differences: r' and r'' to O(h²), r''' to O(h²)
    D3 d1 = scale(sub(p1, m1), 1.0 / (2 * h));
    D3 d2 = scale(add(sub(p1, scale(c, 2.0)), m1), 1.0 / (h * h));
    D3 d3 = scale(add(sub(sub(p2, scale(p1, 2.0)), m2), scale(m1, 2.0)), 1.0 / (2 * h * h * h));
    double speed = length(d1);
    if (!(speed > 1e-12) || !std::isfinite(speed)) return false;
    out.p = c;
    out.speed = speed;
    out.T = scale(d1, 1.0 / speed);
    D3 c12 = cross(d1, d2);
    double n12 = length(c12);
    out.kappa = n12 / (speed * speed * speed);
    // Straight (relative to how fast the curve is traced): no defined normal
    out.hasNormal = out.kappa * speed > 1e-6 && n12 > 1e-12;
    if (out.hasNormal) {
        out.B = scale(c12, 1.0 / n12);
        out.N = cross(out.B, out.T);
        out.tau = dot(c12, d3) / (n12 * n12);
    } else {
        // Any frame at right angles to T
        D3 e = std::fabs(out.T.x) < 0.9 ? D3{1, 0, 0} : D3{0, 1, 0};
        out.N = cross(out.T, e);
        out.N = scale(out.N, 1.0 / length(out.N));
        out.B = cross(out.T, out.N);
        out.kappa = 0.0;
        out.tau = 0.0;
    }
    return std::isfinite(out.kappa) && std::isfinite(out.tau);
}

} // namespace linalg::geometry
