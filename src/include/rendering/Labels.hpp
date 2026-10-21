#pragma once

// Placing a label for a 3D point on screen: the point's screen position (false when behind the camera) and the
// pill a label of that text gets there. SceneView lays several out together (text/LabelLayout) and puts them
// in the overlay (gfx/Overlay.hpp) that the front end paints.

#include "gfx/Overlay.hpp"
#include <raylib.h>

namespace linalg {

bool labelScreenPoint(const Camera3D& camera, Vector3 position, Vector2& out);
// `offset` is in design units (ui::px)
Rectangle labelPill(Vector2 screen, const char* text, Vector2 offset, gfx::TextStyle style,
                    const gfx::TextMeasure& measure);

} // namespace linalg
