#include "rendering/Labels.hpp"
#include "gfx/Pure.hpp"
#include "ui/core/Ui.hpp"
#include <raymath.h>

namespace linalg {

bool labelScreenPoint(const Camera3D& camera, Vector3 position, Vector2& out) {
    Vector3 forward = Vector3Subtract(camera.target, camera.position);
    Vector3 toPoint = Vector3Subtract(position, camera.position);
    if (Vector3DotProduct(forward, toPoint) <= 0.0f) return false;
    out = gfx::worldToScreen(position, camera, static_cast<int>(ui::screenWidth()), static_cast<int>(ui::screenHeight()));
    return true;
}

Rectangle labelPill(Vector2 screen, const char* text, Vector2 offset, gfx::TextStyle style,
                    const gfx::TextMeasure& measure) {
    float size = measure.lineHeight(style);
    float width = measure.width(text, style);
    float padX = ui::px(6.0f), padY = ui::px(2.0f);
    float x = screen.x + ui::px(offset.x);
    float y = screen.y + ui::px(offset.y) - size / 2.0f;
    return {x - padX, y - padY, width + 2 * padX, size + 2 * padY};
}

} // namespace linalg
