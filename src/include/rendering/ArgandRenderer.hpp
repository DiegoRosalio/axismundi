#pragma once

#include "math/Complex.hpp"
#include <raylib.h>

namespace linalg {

// What the complex mode shows
struct ComplexScene {
    Complex z1{1.0f, 1.0f};
    Complex z2{1.0f, 0.5f};
    bool showProduct = true;
    bool showEuler = true;
    float theta = 1.0f;  // radians, for e^(iθ)
};

// Argand diagram on the world XY plane (real part → X, imaginary part → Y).
// Shows z1, z2 and z1·z2 with angle arcs (multiplication = rotation +
// scaling), and Euler's formula as a point on the unit circle with its
// cos/sin projections and the helix e^(it) rising along Z.
class ArgandRenderer {
public:
    void draw(const ComplexScene& scene, float extent, bool showAxes) const;
    void drawLabels(const Camera3D& camera, const ComplexScene& scene, float extent, bool showAxes) const;
};

} // namespace linalg
