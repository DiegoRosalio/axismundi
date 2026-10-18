#pragma once

#include "scene/SceneObject.hpp"
#include <raylib.h>

// Numbers along the axes (…, −2, −1, 1, 2, …) at a step that follows the zoom (math/GridMath tickStep).
// Drawn after the 3D scene and before the object labels, so labels stay on top.
namespace linalg::view {

struct AxisNumbersSpec {
    scene::SpaceKind space = scene::SpaceKind::Euclidean3D;
    double step = 1.0;          // distance between numbers
    float axisLength = 5.0f;    // numbers stay within ±axisLength (where the axes are drawn)
    Vector3 viewCenter{0, 0, 0};  // world point the view is about
    float reach = 20.0f;        // and within this distance of it (the grid's reach)
    Color color{128, 128, 128, 255};
};

void drawAxisNumbers(const Camera3D& camera, const AxisNumbersSpec& spec);

} // namespace linalg::view
