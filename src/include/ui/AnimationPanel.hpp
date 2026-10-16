#pragma once

#include "animation/Animator.hpp"
#include "math/Matrix4D.hpp"
#include "ui/SceneIO.hpp"
#include <raylib.h>

namespace linalg {

// Bottom bar for the linear algebra mode: play/pause, stop, loop, timeline
// scrubber, speed, animation presets and keyframe capture.
class AnimationPanel {
public:
    // `left`/`right` (pixels) are the free horizontal span between the side
    // panels; the card is centered there. `inputMatrix` is captured by "+ Key".
    void draw(Animator& animator, const Matrix4D& inputMatrix, float left, float right);
    static Rectangle bounds(float left, float right);

    static constexpr float kWidthDesign = 660.0f;
    static constexpr float kHeightDesign = 92.0f;

    static void saveState(const Animator& animator, SceneData& data);
    static void loadState(Animator& animator, const SceneData& data);
};

} // namespace linalg
