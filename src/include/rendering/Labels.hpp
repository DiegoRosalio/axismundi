#pragma once

#include <raylib.h>

namespace linalg {

// Draw 2D text anchored at a 3D world position (call after EndMode3D()).
// Nothing is drawn when the point is behind the camera.
void drawWorldLabel(const Camera3D& camera, Vector3 position, const char* text,
                    int fontSize, Color color, Vector2 offset = {8.0f, -8.0f});

} // namespace linalg
