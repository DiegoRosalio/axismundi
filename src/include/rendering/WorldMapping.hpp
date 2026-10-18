#pragma once

#include "math/Integrals.hpp"
#include "math/Vector3D.hpp"
#include <raylib.h>

namespace linalg {

// The 3D space uses the math convention: z is up and the floor is the xy-plane.
// raylib's world is y-up, so math (x, y, z) is drawn at world (x, z, -y),
// which keeps the axes right-handed.
inline Vector3 mathToWorld(float x, float y, float z) { return {x, z, -y}; }
inline Vector3 mathToWorld(const Vector3D& v) { return {v.x, v.z, -v.y}; }
inline Vector3 mathToWorld(const integrals::D3& p) { return mathToWorld(integrals::toVector(p)); }
inline Vector3D worldToMath(Vector3 w) { return {w.x, -w.z, w.y}; }

} // namespace linalg
