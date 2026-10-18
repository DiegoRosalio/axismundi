#pragma once

#include "math/Integrals.hpp"

namespace linalg::geometry {

// Differential geometry of a space curve r(u) (no raylib): the Frenet frame, curvature and torsion,
// from derivatives taken by central differences.
using integrals::D3;
using integrals::Path;

struct Frame {
    D3 p;            // r(u)
    D3 T, N, B;      // unit tangent, normal (towards the center of curvature), binormal = T × N
    double kappa = 0.0;   // curvature: 1 / radius of the osculating circle
    double tau = 0.0;     // torsion: how fast the curve leaves its osculating plane
    double speed = 0.0;   // |r'(u)|
    bool hasNormal = false;  // false on straight pieces (κ ≈ 0): N and B are then arbitrary
};

// h is the difference step (choose about 1e-3 of the parameter range). False where r is undefined
// near u or r'(u) = 0.
bool frenet(const Path& r, double u, double h, Frame& out);

D3 add(const D3& a, const D3& b);
D3 sub(const D3& a, const D3& b);
D3 scale(const D3& a, double k);
double dot(const D3& a, const D3& b);
D3 cross(const D3& a, const D3& b);
double length(const D3& a);

} // namespace linalg::geometry
