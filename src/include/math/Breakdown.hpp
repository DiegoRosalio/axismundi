#pragma once

#include "math/Matrix4D.hpp"
#include "math/Quaternion.hpp"
#include "math/Vector3D.hpp"
#include <string>
#include <vector>

namespace linalg {

// One elementary step of a transformation
struct BreakdownStep {
    enum class Kind { Rotation, Scale, Translation };
    Kind kind;
    std::string title;        // "Rotate", "Stretch", "Move"
    std::string description;  // "45.0° about the z-axis", "x by 2, flip z", ...
    Quaternion rotation;      // Rotation
    Vector3D scale{1, 1, 1};  // Scale (signed: negative = flip, 0 = squash)
    Vector3D translation;     // Translation

    Matrix4D matrix() const;               // this step alone
    Matrix4D partial(float fraction) const; // the step done `fraction` of the way
};

// M = T · U · Σ · Vᵀ applied right to left: rotate (Vᵀ), stretch along the
// axes (Σ), rotate (U), move (T). U and V are made proper rotations, so a
// mirror shows up as a negative stretch. Steps that do nothing are skipped,
// and equal stretches merge the two rotations into one.
struct TransformBreakdown {
    std::vector<BreakdownStep> steps;

    // Matrix after progress t in [0, steps.size()]: full steps below floor(t)
    // and the current step done (t - floor(t)) of the way
    Matrix4D at(float t) const;
    // Product of the first k steps
    Matrix4D after(int k) const;
};

TransformBreakdown breakdown(const Matrix4D& m);

} // namespace linalg
