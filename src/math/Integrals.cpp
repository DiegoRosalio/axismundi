#include "math/Integrals.hpp"
#include <algorithm>
#include <cmath>

namespace linalg::integrals {

RiemannSum riemann2D(const Surface& f, const Rect& r, int nx, int ny, Rule rule) {
    RiemannSum out;
    nx = std::max(1, nx);
    ny = std::max(1, ny);
    double dx = (r.x1 - r.x0) / nx, dy = (r.y1 - r.y0) / ny;
    double fx = rule == Rule::Left ? 0.0 : rule == Rule::Middle ? 0.5 : 1.0;
    out.boxes.reserve(static_cast<size_t>(nx) * ny);
    for (int j = 0; j < ny; j++)
        for (int i = 0; i < nx; i++) {
            double x0 = r.x0 + i * dx, y0 = r.y0 + j * dy;
            double z = 0;
            if (!f(x0 + fx * dx, y0 + fx * dy, z) || !std::isfinite(z)) {
                out.skipped++;
                continue;
            }
            out.boxes.push_back({x0, x0 + dx, y0, y0 + dy, z});
            out.sum += z * dx * dy;
        }
    return out;
}

double simpson2D(const Surface& f, const Rect& r, int n, bool& ok) {
    ok = true;
    n = std::max(2, n + (n % 2));
    double hx = (r.x1 - r.x0) / n, hy = (r.y1 - r.y0) / n;
    auto w = [n](int i) { return (i == 0 || i == n) ? 1.0 : (i % 2 ? 4.0 : 2.0); };
    double total = 0;
    for (int j = 0; j <= n; j++)
        for (int i = 0; i <= n; i++) {
            double z = 0;
            if (!f(r.x0 + i * hx, r.y0 + j * hy, z) || !std::isfinite(z)) {
                ok = false;
                return 0.0;
            }
            total += w(i) * w(j) * z;
        }
    return total * hx * hy / 9.0;
}

bool derivative(const Path& r, double u, double h, D3& out) {
    D3 a, b;
    if (!r(u - h, a) || !r(u + h, b)) return false;
    out = {(b.x - a.x) / (2 * h), (b.y - a.y) / (2 * h), (b.z - a.z) / (2 * h)};
    return std::isfinite(out.x) && std::isfinite(out.y) && std::isfinite(out.z);
}

double lineIntegral(const Field& F, const Path& r, double a, double b, double upto, int n, bool& ok) {
    ok = true;
    upto = std::clamp(upto, 0.0, 1.0);
    double end = a + upto * (b - a);
    if (std::fabs(end - a) < 1e-15) return 0.0;
    n = std::max(2, n + (n % 2));
    double h = (end - a) / n;
    double dh = std::max(1e-7, std::fabs(b - a) * 1e-5);  // derivative step
    double total = 0;
    for (int i = 0; i <= n; i++) {
        double u = a + i * h;
        D3 p, v, d;
        if (!r(u, p) || !F(p, v) || !derivative(r, u, dh, d)) {
            ok = false;
            return 0.0;
        }
        double g = v.x * d.x + v.y * d.y + v.z * d.z;
        double w = (i == 0 || i == n) ? 1.0 : (i % 2 ? 4.0 : 2.0);
        total += w * g;
    }
    return total * h / 3.0;
}

} // namespace linalg::integrals
