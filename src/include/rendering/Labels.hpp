#pragma once

#include <raylib.h>

namespace linalg {

// Draw a text label anchored at a 3D world position (call after EndMode3D()).
// Uses the UI label font on a translucent pill; nothing is drawn when the
// point is behind the camera. `offset` is in design units.
// `background` with alpha 0 uses the theme's label pill color; `large` uses a bigger font;
// `ring` (alpha > 0) outlines the pill (selected objects).
void drawWorldLabel(const Camera3D& camera, Vector3 position, const char* text, Color color,
                    Vector2 offset = {8.0f, -8.0f}, Color background = {0, 0, 0, 0}, bool large = false,
                    Color ring = {0, 0, 0, 0});

// The pieces of drawWorldLabel, for laying out several labels before drawing them:
// the screen point of a world position (false when behind the camera), the pill rectangle a
// label gets at that point, and drawing a label in a given pill.
bool labelScreenPoint(const Camera3D& camera, Vector3 position, Vector2& out);
Rectangle labelPill(Vector2 screen, const char* text, Vector2 offset, bool large);
void drawLabelPill(Rectangle pill, const char* text, Color color, Color background, bool large, Color ring);

} // namespace linalg
