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
    void setMatrix(const Matrix4D& m) { m_player.setMatrix(m); }
    void update(float deltaTime) { m_player.update(deltaTime); }
    float drawContent(Rectangle area);

    const BreakdownPlayer& player() const { return m_player; }
    const TransformBreakdown& breakdown() const { return m_player.breakdown(); }
    Matrix4D current() const { return m_player.current(); }  // matrix at the current progress (eased)
    float progress() const { return m_player.progress(); }
    int completedSteps() const { return m_player.completedSteps(); }
    void restart() { m_player.restart(); }
    void play() { m_player.play(); }

private:
    BreakdownPlayer m_player;  // the playback (shared with the GTK app)
};

} // namespace linalg
