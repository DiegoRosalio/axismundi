#pragma once

// The one-line description of an object for the objects list ("slope 0.54 at x = 1", "y = sin(x)"), or its
// error. Shared by both front ends.

#include "math/Taylor.hpp"
#include "scene/SceneObject.hpp"
#include <string>

namespace linalg::scene {

std::string summaryOf(const SceneObject& object);

// "0.841 + 0.54(x − 1) − 0.421(x − 1)² + …": the first `maxTerms` nonzero terms of a Taylor polynomial
std::string polynomialText(const taylor::Series& series, int maxTerms);

} // namespace linalg::scene
