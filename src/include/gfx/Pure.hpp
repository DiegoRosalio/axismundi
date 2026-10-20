#pragma once

// Small value helpers that raylib has as library functions, written out here (the same arithmetic, so results
// are bit-identical) so that view/ and rendering/ need nothing from the library: colors, rectangles and the
// world → screen projection. Header-only; raymath is header-only too.

#include <raylib.h>
#include <raymath.h>

namespace linalg::gfx {

// raylib's Fade / ColorAlpha
inline Color fade(Color color, float alpha) {
    if (alpha < 0.0f) alpha = 0.0f;
    else if (alpha > 1.0f) alpha = 1.0f;
    color.a = static_cast<unsigned char>(255.0f * alpha);
    return color;
}

// raylib's ColorToInt: 0xRRGGBBAA (as int)
inline int colorToInt(Color c) {
    return static_cast<int>((static_cast<unsigned>(c.r) << 24) | (static_cast<unsigned>(c.g) << 16) |
                            (static_cast<unsigned>(c.b) << 8) | static_cast<unsigned>(c.a));
}

// raylib's CheckCollisionRecs
inline bool overlaps(Rectangle a, Rectangle b) {
    return (a.x < (b.x + b.width) && (a.x + a.width) > b.x) && (a.y < (b.y + b.height) && (a.y + a.height) > b.y);
}

// Near and far planes of the 3D view (rlgl's defaults; the projection in RaylibCanvas uses the same)
constexpr double kNearPlane = 0.01;
constexpr double kFarPlane = 1000.0;

// raylib's GetWorldToScreenEx: where a world point lands on a width × height screen
inline Vector2 worldToScreen(Vector3 position, const Camera3D& camera, int width, int height) {
    Matrix proj = MatrixIdentity();
    if (camera.projection == CAMERA_PERSPECTIVE) {
        proj = MatrixPerspective(camera.fovy * DEG2RAD, (static_cast<double>(width) / static_cast<double>(height)),
                                 kNearPlane, kFarPlane);
    } else if (camera.projection == CAMERA_ORTHOGRAPHIC) {
        double aspect = static_cast<double>(width) / static_cast<double>(height);
        double top = camera.fovy / 2.0, right = top * aspect;
        proj = MatrixOrtho(-right, right, -top, top, kNearPlane, kFarPlane);
    }
    Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);
    Quaternion p = {position.x, position.y, position.z, 1.0f};
    p = QuaternionTransform(p, view);
    p = QuaternionTransform(p, proj);
    Vector3 ndc = {p.x / p.w, -p.y / p.w, p.z / p.w};
    return {(ndc.x + 1.0f) / 2.0f * static_cast<float>(width), (ndc.y + 1.0f) / 2.0f * static_cast<float>(height)};
}

} // namespace linalg::gfx
