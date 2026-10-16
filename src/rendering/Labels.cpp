#include "rendering/Labels.hpp"
#include <raymath.h>

namespace linalg {

void drawWorldLabel(const Camera3D& camera, Vector3 position, const char* text,
                    int fontSize, Color color, Vector2 offset) {
    Vector3 forward = Vector3Subtract(camera.target, camera.position);
    Vector3 toPoint = Vector3Subtract(position, camera.position);
    if (Vector3DotProduct(forward, toPoint) <= 0.0f) return;

    Vector2 screen = GetWorldToScreen(position, camera);
    int x = static_cast<int>(screen.x + offset.x);
    int y = static_cast<int>(screen.y + offset.y - fontSize / 2.0f);

    // Light backdrop keeps labels readable over the grid and other geometry
    int width = MeasureText(text, fontSize);
    DrawRectangle(x - 3, y - 2, width + 6, fontSize + 4, Fade(RAYWHITE, 0.75f));
    DrawText(text, x, y, fontSize, color);
}

} // namespace linalg
