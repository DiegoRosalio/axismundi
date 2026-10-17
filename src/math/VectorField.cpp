#include "math/VectorField.hpp"
#include <algorithm>
#include <cmath>

namespace linalg::field {

bool Box::contains(const Vector3D& p, double slack) const {
    return p.x >= min.x - slack && p.x <= max.x + slack && p.y >= min.y - slack && p.y <= max.y + slack &&
           p.z >= min.z - slack && p.z <= max.z + slack;
}

FieldFunction::FieldFunction(const expr::Program& program, const expr::Resolver* resolver, double time, bool plane)
    : m_program(program), m_resolver(resolver), m_time(time), m_plane(plane) {}

bool FieldFunction::at(const Vector3D& p, Vector3D& out) const {
    expr::Slots s;
    s.x = p.x;
    s.y = p.y;
    s.z = m_plane ? 0.0 : p.z;
    s.t = m_time;
    s.hasX = s.hasY = s.hasZ = s.hasT = true;
    expr::Value v = m_program.eval(m_resolver, &s);
    if (!v.ok() || v.type != expr::Type::Vector) return false;
    out = v.vector;
    if (m_plane) out.z = 0.0f;
    return std::isfinite(out.x) && std::isfinite(out.y) && std::isfinite(out.z);
}

ArrowSet sampleArrows(const FieldFunction& f, const Box& box, int nx, int ny, int nz, bool sameLength) {
    ArrowSet set;
    nx = std::max(1, nx);
    ny = std::max(1, ny);
    nz = f.plane() ? 1 : std::max(1, nz);
    double dx = (box.max.x - box.min.x) / nx, dy = (box.max.y - box.min.y) / ny;
    double dz = f.plane() ? 0.0 : (box.max.z - box.min.z) / nz;
    set.cell = std::min(dx, dy);
    if (!f.plane()) set.cell = std::min(set.cell, dz);
    if (!(set.cell > 0.0)) set.cell = 1.0;

    std::vector<Arrow> raw;
    raw.reserve(static_cast<size_t>(nx) * ny * nz);
    double lo = 0.0, hi = 0.0;
    bool first = true;
    for (int k = 0; k < nz; k++)
        for (int j = 0; j < ny; j++)
            for (int i = 0; i < nx; i++) {
                Vector3D p(static_cast<float>(box.min.x + (i + 0.5) * dx), static_cast<float>(box.min.y + (j + 0.5) * dy),
                           f.plane() ? 0.0f : static_cast<float>(box.min.z + (k + 0.5) * dz));
                Vector3D v;
                if (!f.at(p, v)) continue;
                double m = v.magnitude();
                if (!(m > 1e-12)) continue;
                raw.push_back({p, v, m});
                lo = first ? m : std::min(lo, m);
                hi = first ? m : std::max(hi, m);
                first = false;
            }
    set.minMagnitude = lo;
    set.maxMagnitude = hi;
    // The longest arrow is 90% of a cell, so neighbours don't touch
    const double longest = 0.9 * set.cell;
    for (Arrow& a : raw) {
        double scale = sameLength ? 0.75 * longest / a.magnitude : longest / hi;
        a.vec = a.vec * static_cast<float>(scale);
    }
    set.arrows = std::move(raw);
    return set;
}

namespace {

// Unit direction of F, false where F is (nearly) zero or undefined
bool direction(const FieldFunction& f, const Vector3D& p, double sign, Vector3D& dir, double& speed) {
    Vector3D v;
    if (!f.at(p, v)) return false;
    speed = v.magnitude();
    if (!(speed > 1e-9)) return false;
    dir = v * static_cast<float>(sign / speed);
    return true;
}

// One direction of a flow line: points after the seed (not including it) and their flow times
void traceOneWay(const FieldFunction& f, const Vector3D& seed, const Box& box, double step, int maxSteps, double sign,
                 std::vector<Vector3D>& points, std::vector<double>& times, bool& closed) {
    Vector3D p = seed;
    double tau = 0.0;
    for (int n = 0; n < maxSteps; n++) {
        Vector3D k1, k2, k3, k4;
        double s1, s2, s3, s4;
        float h = static_cast<float>(step);
        if (!direction(f, p, sign, k1, s1)) return;
        if (!direction(f, p + k1 * (h / 2), sign, k2, s2)) return;
        if (!direction(f, p + k2 * (h / 2), sign, k3, s3)) return;
        if (!direction(f, p + k3 * h, sign, k4, s4)) return;
        Vector3D next = p + (k1 + k2 * 2.0f + k3 * 2.0f + k4) * (h / 6.0f);
        if (f.plane()) next.z = 0.0f;
        // Flow time: arc length over the speed (Simpson's rule on 1/|F|)
        double dt = step * (1.0 / s1 + 4.0 / s2 + 1.0 / s4) / 6.0;
        if (!box.contains(next)) {
            // Clip the last step at the box edge
            double lo = 0.0, hi = 1.0;
            for (int it = 0; it < 20; it++) {
                double mid = (lo + hi) / 2;
                (box.contains(p + (next - p) * static_cast<float>(mid)) ? lo : hi) = mid;
            }
            if (lo > 1e-6) {
                points.push_back(p + (next - p) * static_cast<float>(lo));
                times.push_back(tau + dt * lo);
            }
            return;
        }
        tau += dt;
        p = next;
        points.push_back(p);
        times.push_back(tau);
        // Back at the start after going round: an orbit
        if (n > 8 && (p - seed).magnitude() < 0.6 * step) {
            closed = true;
            return;
        }
    }
}

} // namespace

FlowLine traceFlowLine(const FieldFunction& f, const Vector3D& seed, const Box& box, double step, int maxSteps) {
    FlowLine line;
    Vector3D s = seed;
    if (f.plane()) s.z = 0.0f;
    std::vector<Vector3D> fwd, back;
    std::vector<double> fwdT, backT;
    bool closed = false;
    traceOneWay(f, s, box, step, maxSteps, 1.0, fwd, fwdT, closed);
    if (!closed) {
        bool closedBack = false;
        traceOneWay(f, s, box, step, maxSteps, -1.0, back, backT, closedBack);
    }
    // Upstream end first: the backward points reversed, then the seed, then forward
    double offset = backT.empty() ? 0.0 : backT.back();
    for (size_t i = back.size(); i-- > 0;) {
        line.points.push_back(back[i]);
        line.times.push_back(offset - backT[i]);
    }
    line.points.push_back(s);
    line.times.push_back(offset);
    for (size_t i = 0; i < fwd.size(); i++) {
        line.points.push_back(fwd[i]);
        line.times.push_back(offset + fwdT[i]);
    }
    if (closed) {
        // Close the loop exactly
        Vector3D v;
        double speed = f.at(s, v) ? v.magnitude() : 0.0;
        double gap = (line.points.back() - s).magnitude();
        line.points.push_back(s);
        line.times.push_back(line.times.back() + (speed > 1e-9 ? gap / speed : 0.0));
        line.closed = true;
    }
    if (line.points.size() < 2) line.points.clear(), line.times.clear();
    return line;
}

std::vector<Vector3D> flowSeeds(const Box& box, bool plane, int perAxis) {
    std::vector<Vector3D> seeds;
    perAxis = std::max(1, perAxis);
    int nz = plane ? 1 : perAxis;
    for (int k = 0; k < nz; k++)
        for (int j = 0; j < perAxis; j++)
            for (int i = 0; i < perAxis; i++) {
                float x = static_cast<float>(box.min.x + (i + 0.5) * (box.max.x - box.min.x) / perAxis);
                float y = static_cast<float>(box.min.y + (j + 0.5) * (box.max.y - box.min.y) / perAxis);
                float z = plane ? 0.0f : static_cast<float>(box.min.z + (k + 0.5) * (box.max.z - box.min.z) / nz);
                seeds.emplace_back(x, y, z);
            }
    return seeds;
}

Vector3D particleAt(const FlowLine& line, double tau) {
    if (line.points.empty()) return Vector3D::zero();
    if (tau <= line.times.front()) return line.points.front();
    if (tau >= line.times.back()) return line.points.back();
    auto it = std::upper_bound(line.times.begin(), line.times.end(), tau);
    size_t i = static_cast<size_t>(it - line.times.begin());
    double t0 = line.times[i - 1], t1 = line.times[i];
    float a = t1 > t0 ? static_cast<float>((tau - t0) / (t1 - t0)) : 0.0f;
    return line.points[i - 1] + (line.points[i] - line.points[i - 1]) * a;
}

double particleTime(const FlowLine& line, int k, int n, double t, double speed) {
    double T = line.duration();
    if (!(T > 0.0) || n <= 0) return 0.0;
    double tau = std::fmod(speed * t + static_cast<double>(k) * T / n, T);
    return tau < 0 ? tau + T : tau;
}

} // namespace linalg::field
