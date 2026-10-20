#include "rendering/Labels.hpp"
#include "ui/core/Theme.hpp"
#include "ui/core/Ui.hpp"
#include <raymath.h>

namespace linalg {

namespace {

ui::FontRole roleOf(bool large) { return large ? ui::FontRole::Title : ui::FontRole::Label3D; }

} // namespace

bool labelScreenPoint(const Camera3D& camera, Vector3 position, Vector2& out) {
    Vector3 forward = Vector3Subtract(camera.target, camera.position);
    Vector3 toPoint = Vector3Subtract(position, camera.position);
    if (Vector3DotProduct(forward, toPoint) <= 0.0f) return false;
    out = GetWorldToScreenEx(position, camera, static_cast<int>(ui::screenWidth()), static_cast<int>(ui::screenHeight()));
    return true;
}

Rectangle labelPill(Vector2 screen, const char* text, Vector2 offset, bool large) {
    float size = ui::fontSize(roleOf(large));
    float width = ui::textWidth(text, roleOf(large));
    float padX = ui::px(6.0f), padY = ui::px(2.0f);
    float x = screen.x + ui::px(offset.x);
    float y = screen.y + ui::px(offset.y) - size / 2.0f;
    return {x - padX, y - padY, width + 2 * padX, size + 2 * padY};
}

void drawLabelPill(Rectangle pill, const char* text, Color color, Color background, bool large, Color ring) {
    ui::fillRounded(pill, pill.height / 2, background.a ? background : ui::theme().labelBg);
    if (ring.a) ui::strokeRounded(pill, pill.height / 2, ui::px(2.0f), ring);
    ui::text(text, pill.x + ui::px(6.0f), pill.y + ui::px(2.0f), roleOf(large), color);
}

void drawWorldLabel(const Camera3D& camera, Vector3 position, const char* text, Color color, Vector2 offset,
                    Color background, bool large, Color ring) {
    Vector2 screen;
    if (!labelScreenPoint(camera, position, screen)) return;
    drawLabelPill(labelPill(screen, text, offset, large), text, color, background, large, ring);
}

} // namespace linalg
