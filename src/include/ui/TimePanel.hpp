#pragma once

#include "scene/Scene.hpp"
#include <raylib.h>

namespace linalg {

// The scene clock: play / pause / restart, a scrubber, speed, loop and length,
// and the list of parameters in the current space that follow the clock.
class TimePanel {
public:
    // Returns the content height (pixels)
    float drawContent(Rectangle area, scene::Scene& scene, scene::SpaceKind kind);

    // Play or pause; at the end of a non-looping timeline, play starts again from 0
    static void togglePlay(scene::Scene& scene);
};

} // namespace linalg
