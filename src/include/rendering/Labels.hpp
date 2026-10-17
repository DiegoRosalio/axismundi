#pragma once

#include <raylib.h>

namespace linalg {

// Draw a text label anchored at a 3D world position (call after EndMode3D()).
// Uses the UI label font on a translucent pill; nothing is drawn when the
// point is behind the camera. `offset` is in design units.
// `background` with alpha 0 uses the theme's label pill color; `large` uses a bigger font.
void drawWorldLabel(const Camera3D& camera, Vector3 position, const char* text, Color color,
                    Vector2 offset = {8.0f, -8.0f}, Color background = {0, 0, 0, 0}, bool large = false);

} // namespace linalg
