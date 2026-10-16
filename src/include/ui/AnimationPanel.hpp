#pragma once

#include "animation/Animator.hpp"
#include "math/Matrix4D.hpp"
#include "ui/SceneIO.hpp"
#include <raylib.h>

namespace linalg {

// Content of the "Animation" panel: play/pause, stop, loop, timeline
// scrubber, speed, animation presets and keyframe capture.
class AnimationPanel {
public:
    // Draw the controls into `area`; `inputMatrix` is captured by "Key".
    // Returns the content height in pixels.
    float drawContent(Rectangle area, Animator& animator, const Matrix4D& inputMatrix);

    static constexpr float kWidthDesign = 660.0f;

    static void saveState(const Animator& animator, SceneData& data);
    static void loadState(Animator& animator, const SceneData& data);
};

} // namespace linalg
