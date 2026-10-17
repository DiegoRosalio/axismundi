#include "rendering/Labels.hpp"
#include "ui/core/Theme.hpp"
#include "ui/core/Ui.hpp"
#include <raymath.h>

namespace linalg {

void drawWorldLabel(const Camera3D& camera, Vector3 position, const char* text, Color color, Vector2 offset,
                    Color background, bool large, Color ring) {
    Vector3 forward = Vector3Subtract(camera.target, camera.position);
    Vector3 toPoint = Vector3Subtract(position, camera.position);
    if (Vector3DotProduct(forward, toPoint) <= 0.0f) return;

    const ui::FontRole role = large ? ui::FontRole::Title : ui::FontRole::Label3D;
    Vector2 screen = GetWorldToScreen(position, camera);
    float size = ui::fontSize(role);
    float width = ui::textWidth(text, role);
    float padX = ui::px(6.0f), padY = ui::px(2.0f);
    float x = screen.x + ui::px(offset.x);
    float y = screen.y + ui::px(offset.y) - size / 2.0f;

    Rectangle pill = {x - padX, y - padY, width + 2 * padX, size + 2 * padY};
    ui::fillRounded(pill, pill.height / 2, background.a ? background : ui::theme().labelBg);
    if (ring.a) ui::strokeRounded(pill, pill.height / 2, ui::px(2.0f), ring);
    ui::text(text, x, y, role, color);
}

} // namespace linalg
