// The Fourier series of f(x) on the floor: f, the partial sum, the separate harmonics
#include "view/ObjectViews.hpp"
#include "gfx/Canvas3D.hpp"
#include "math/Decompositions.hpp"
#include "rendering/VectorRenderer.hpp"
#include "rendering/WorldMapping.hpp"
#include "ui/core/Theme.hpp"
#include "view/SceneViewDraw.hpp"
#include "view/ViewStyle.hpp"
#include <raymath.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace linalg::view {

using scene::SceneObject;
using namespace draw;

namespace {

void drawFourierSeries(const scene::FourierSeriesObject& o, Color c, bool fade, bool print, Color ink) {
    const ViewStyle& st = activeStyle();
    // On the floor like y = f(x), a hair above it
    auto at = [](double x, double y) { return mathToWorld(static_cast<float>(x), static_cast<float>(y), 0.006f); };
    auto polyline = [&](const std::vector<std::pair<double, double>>& pts, Color col, float radius) {
        for (size_t i = 1; i < pts.size(); i++) {
            const auto &a = pts[i - 1], &b = pts[i];
            if (!std::isfinite(a.second) || !std::isfinite(b.second) || std::fabs(a.second) > 100 || std::fabs(b.second) > 100)
                continue;
            if (radius <= 0) gfx::line(at(a.first, a.second), at(b.first, b.second), col);
            else gfx::cylinder(at(a.first, a.second), at(b.first, b.second), radius, radius, 5, col);
        }
    };
    if (o.showHarmonics()) {
        Color h = print ? Color{170, 170, 170, 255} : withAlpha(fade ? faded(st.muted) : st.muted, 150);
        for (const auto& harm : o.harmonics()) polyline(harm, h, 0.0f);
    }
    if (o.showOriginal()) polyline(o.original(), print ? Color{120, 120, 120, 255} : (fade ? faded(st.muted) : st.muted),
                                   print ? 0.0f : 0.012f * st.lineScale);
    polyline(o.sum(), print ? ink : (fade ? faded(c) : c), print ? 0.0f : 0.025f * st.lineScale);
}

} // namespace

void registerFourierViews(ObjectViewTable& t) {
    ObjectView v;
    v.palette = Palette::Plots;
    v.draw = [](const SceneObject& o, const DrawContext& c) {
        if (!o.error().empty()) return;
        drawFourierSeries(static_cast<const scene::FourierSeriesObject&>(o), c.color, c.dim, c.print,
                          c.print ? c.color : BLACK);
    };
    t["fourierseries"] = v;
}

} // namespace linalg::view
