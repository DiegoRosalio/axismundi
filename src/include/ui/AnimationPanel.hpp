#pragma once

#include "animation/Animator.hpp"
#include "math/Matrix4D.hpp"
#include "ui/SceneIO.hpp"

namespace linalg {

// Bottom bar for the linear algebra mode: play/pause, stop, loop, timeline
// scrubber, speed, animation presets and keyframe capture.
class AnimationPanel {
public:
    // `left`/`right` are the free horizontal span between the side panels.
    // `inputMatrix` is captured by "+ Key".
    void draw(Animator& animator, const Matrix4D& inputMatrix, float left, float right);

    static constexpr float kHeight = 104.0f;

    static void saveState(const Animator& animator, SceneData& data);
    static void loadState(Animator& animator, const SceneData& data);
};

} // namespace linalg
