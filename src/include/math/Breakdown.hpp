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

// Plays a breakdown: the progress t runs from 0 to the number of steps, pausing briefly after each step. Both
// front ends' Breakdown panels drive one; while it is shown, the scene draws the matrix at current().
class BreakdownPlayer {
public:
    // Recompute when the matrix changes (keeps the progress if possible)
    void setMatrix(const Matrix4D& m);
    void update(float deltaTime);

    const TransformBreakdown& breakdown() const { return m_breakdown; }
    int stepCount() const { return static_cast<int>(m_breakdown.steps.size()); }
    Matrix4D current() const;  // the matrix at the current progress (eased)
    float progress() const { return m_t; }
    int completedSteps() const;
    bool playing() const { return m_playing; }
    bool finished() const { return m_t >= static_cast<float>(stepCount()) - 1e-3f; }

    void restart();     // from the identity, playing
    void play() { m_playing = true; }
    void togglePlay();  // pause, play, or start over at the end
    void seek(float t); // jump there and pause
    void previous();    // back to the start of the step
    void next();        // to the end of the next step

private:
    TransformBreakdown m_breakdown;
    Matrix4D m_matrix = Matrix4D::identity();
    bool m_hasMatrix = false;
    float m_t = 0.0f;  // 0..steps
    bool m_playing = false;
    float m_hold = 0.0f;  // pause left at a step boundary (seconds)
};

} // namespace linalg
