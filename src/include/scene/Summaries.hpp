#pragma once

// The one-line description of an object for the objects list ("slope 0.54 at x = 1", "y = sin(x)"), or its
// error. Shared by both front ends.

#include "scene/SceneObject.hpp"
#include <string>

namespace linalg::scene {

std::string summaryOf(const SceneObject& object);

} // namespace linalg::scene
