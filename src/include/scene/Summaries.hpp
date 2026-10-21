#pragma once

// The one-line description of an object for the objects list ("slope 0.54 at x = 1", "y = sin(x)"), or its
// error. Shared by both front ends.

#include "math/Taylor.hpp"
#include "scene/SceneObject.hpp"
#include "scene/Space.hpp"
#include <string>

namespace linalg::scene {

std::string summaryOf(const SceneObject& object);

// The formula editors' live result for `text` typed into one of the object's fields, evaluated as the object
// would (t = its time; plots and fields at a sample point): "= (1, 0, 0)   (vector)", a plot or field sample, or
// the problem. `ok` is false for problems (and for plot samples, which are only an illustration).
std::string fieldPreview(const Space& space, const SceneObject& object, const Field& field, const std::string& text,
                         bool& ok);

// "0.841 + 0.54(x − 1) − 0.421(x − 1)² + …": the first `maxTerms` nonzero terms of a Taylor polynomial
std::string polynomialText(const taylor::Series& series, int maxTerms);

} // namespace linalg::scene
