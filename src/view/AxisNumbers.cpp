#include "view/AxisNumbers.hpp"
#include "gfx/Pure.hpp"
#include "math/GridMath.hpp"
#include "rendering/WorldMapping.hpp"
#include "ui/core/Ui.hpp"
#include <raymath.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace linalg::view {

void axisNumbers(const Camera3D& camera, const AxisNumbersSpec& spec, const gfx::TextMeasure& measure,
                 gfx::Overlay& out) {
    const bool complex = spec.space == scene::SpaceKind::Complex;
    const int axes = complex ? 2 : 3;
    const Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    const float w = ui::screenWidth(), h = ui::screenHeight();
    std::vector<Rectangle> placed;  // so numbers never overlap each other

    for (int axis = 0; axis < axes; axis++) {
        // World point at distance s along this axis (math x, y, z in 3D; Re and Im in the complex plane)
        auto at = [&](double s) {
            float f = static_cast<float>(s);
            if (complex) return axis == 0 ? Vector3{f, 0, 0} : Vector3{0, f, 0};
            return mathToWorld(axis == 0 ? f : 0.0f, axis == 1 ? f : 0.0f, axis == 2 ? f : 0.0f);
        };
        // Only the stretch of the axis near the view center (the rest is far away or off-screen)
        Vector3 c = spec.viewCenter;
        double along = complex ? (axis == 0 ? c.x : c.y) : (axis == 0 ? c.x : axis == 1 ? -c.z : c.y);
        // The z axis is drawn upward only, so it gets positive numbers only (x and y run along the floor grid)
        double lowest = (!complex && axis == 2) ? 0.0 : -static_cast<double>(spec.axisLength);
        double lo = std::max(lowest, along - spec.reach);
        double hi = std::min(static_cast<double>(spec.axisLength), along + spec.reach);
        long first = static_cast<long>(std::ceil(lo / spec.step - 1e-9));
        long last = static_cast<long>(std::floor(hi / spec.step + 1e-9));
        if (last - first > 400) continue;  // far too many: the step doesn't fit this zoom
        for (long k = first; k <= last; k++) {
            if (k == 0) continue;
            Vector3 p = at(k * spec.step);
            if (Vector3DotProduct(forward, Vector3Subtract(p, camera.position)) <= 0.0f) continue;  // behind
            Vector2 s = gfx::worldToScreen(p, camera, static_cast<int>(w), static_cast<int>(h));
            std::string text = grid::formatTick(k * spec.step, spec.step);
            float tw = measure.width(text.c_str(), gfx::TextStyle::Small);
            // Beside the axis: below it for horizontal axes, to the right for the vertical one
            bool vertical = complex ? axis == 1 : axis == 2;
            Rectangle r = vertical ? Rectangle{s.x + ui::px(6), s.y - ui::px(8), tw, ui::px(16)}
                                   : Rectangle{s.x - tw / 2, s.y + ui::px(4), tw, ui::px(16)};
            if (r.x < 0 || r.y < 0 || r.x + r.width > w || r.y + r.height > h) continue;
            Rectangle padded = {r.x - ui::px(3), r.y - ui::px(1), r.width + ui::px(6), r.height + ui::px(2)};
            bool overlaps = false;
            for (const Rectangle& q : placed) overlaps = overlaps || gfx::overlaps(padded, q);
            if (overlaps) continue;
            placed.push_back(padded);
            gfx::OverlayItem number;
            number.kind = gfx::OverlayItem::Kind::Text;
            number.rect = r;
            number.text = text;
            number.style = gfx::TextStyle::Small;
            number.color = spec.color;
            out.push_back(std::move(number));
        }
    }
}

} // namespace linalg::view
