#pragma once

#include "math/Breakdown.hpp"
#include <raylib.h>

namespace linalg {

// Content of the "Transformation breakdown" panel: the matrix split into
// rotate / stretch / rotate / move steps, with playback. While the panel is
// open the scene shows the matrix part-way through the steps.
class BreakdownPanel {
public:
    // Recompute when the input matrix changes (keeps the progress if possible)
    void setMatrix(const Matrix4D& m);
    void update(float deltaTime);
    float drawContent(Rectangle area);

    const TransformBreakdown& breakdown() const { return m_breakdown; }
    Matrix4D current() const;      // matrix at the current progress (eased)
    float progress() const { return m_t; }
    int completedSteps() const;
    void restart();
    void play() { m_playing = true; }

private:
    TransformBreakdown m_breakdown;
    Matrix4D m_matrix = Matrix4D::identity();
    bool m_hasMatrix = false;
    float m_t = 0.0f;        // 0..steps
    bool m_playing = false;
    float m_hold = 0.0f;     // pause left at a step boundary (seconds)
};

} // namespace linalg
