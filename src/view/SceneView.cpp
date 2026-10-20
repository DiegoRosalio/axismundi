#include "view/SceneView.hpp"
#include "gfx/Canvas3D.hpp"
#include "gfx/Pure.hpp"
#include "math/Decompositions.hpp"
#include "rendering/Labels.hpp"
#include "rendering/VectorRenderer.hpp"
#include "rendering/WorldMapping.hpp"
#include "ui/core/Theme.hpp"
#include "view/ViewStyle.hpp"
#include "view/SceneViewDraw.hpp"
#include "view/ObjectViews.hpp"
#include "text/LabelLayout.hpp"
#include "ui/core/Ui.hpp"
#include <algorithm>
#include <string>
#include <vector>
#include <raymath.h>
#include <cmath>
#include <cstdio>

namespace linalg::view {

using scene::SceneObject;
using namespace draw;

namespace {

constexpr Vector2 kOffset = {8.0f, -8.0f};  // default label offset (design units)

// Labels of one frame: collected, laid out so they don't overlap (selected first, axes last),
// then drawn
class LabelBatch {
public:
    explicit LabelBatch(const Camera3D& camera) : m_camera(camera) {}

    // priority: -1 = automatic (2 when ringed, i.e. selected; 1 otherwise); axes pass 0
    void add(Vector3 position, const char* text, Color color, Vector2 offset, Color background, bool large = false,
             Color ring = {0, 0, 0, 0}, int priority = -1) {
        Vector2 screen;
        if (!labelScreenPoint(m_camera, position, screen)) return;
        Item item{screen, text, color, background, ring, offset, large, priority < 0 ? (ring.a ? 2 : 1) : priority};
        m_items.push_back(std::move(item));
    }

    void draw() const {
        std::vector<text::LabelRequest> requests;
        std::vector<Rectangle> pills;
        for (const Item& it : m_items) {
            Rectangle pill = labelPill(it.screen, it.text.c_str(), it.offset, it.large);
            pills.push_back(pill);
            requests.push_back({it.screen.x, it.screen.y, pill.width, pill.height, pill.x - it.screen.x,
                                pill.y - it.screen.y, it.priority});
        }
        auto boxes = text::layoutLabels(requests, ui::px(2.0f));
        for (size_t i = 0; i < m_items.size(); i++) {
            const Item& it = m_items[i];
            Rectangle pill = {boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h};
            // A label moved away from its point gets a thin leader line back to it
            if (std::fabs(pill.x - pills[i].x) > 0.5f || std::fabs(pill.y - pills[i].y) > 0.5f) {
                Vector2 near = {std::clamp(it.screen.x, pill.x, pill.x + pill.width),
                                std::clamp(it.screen.y, pill.y, pill.y + pill.height)};
                DrawLineEx(it.screen, near, ui::px(1.0f), gfx::fade(it.color, 0.6f));
            }
            drawLabelPill(pill, it.text.c_str(), it.color, it.background, it.large, it.ring);
        }
    }

private:
    struct Item {
        Vector2 screen;
        std::string text;
        Color color, background, ring;
        Vector2 offset;
        bool large;
        int priority;
    };
    const Camera3D& m_camera;
    std::vector<Item> m_items;
};


} // namespace

Color SceneView::colorOf(const SceneObject& o) {
    if (o.customColor) return {o.color.r, o.color.g, o.color.b, 255};
    const ViewStyle& s = activeStyle();
    const ObjectView* ov = objectView(o.typeId());
    switch (ov ? ov->palette : Palette::Accent) {
        case Palette::Vectors: return paletteColor(s.vectors, o.paletteIndex);
        case Palette::Plots: return paletteColor(s.plots, o.paletteIndex);
        case Palette::Complexes: return paletteColor(s.complexes, o.paletteIndex);
        case Palette::Eigen: return s.eigen;
        case Palette::Product: return s.product;
        case Palette::Accent: break;
    }
    return ui::theme().accent;  // matrix, parameter (list dots; matrices draw their basis colors)
}

Matrix4D SceneView::displayedMatrix(const SceneObject& o, const DrawOptions& options) {
    if (o.id() == options.overrideMatrix) return options.overrideValue;
    return o.value().matrix4();
}

bool SceneView::anchorPoint(const SceneObject& o, const DrawOptions& options, Vector3& out) {
    const ObjectView* ov = objectView(o.typeId());
    return ov && ov->anchor && ov->anchor(o, options, out);
}

const PlotRenderer* SceneView::plot(scene::ObjectId id) const {
    auto it = m_plots.find(id);
    return it == m_plots.end() ? nullptr : &it->second.renderer;
}

void SceneView::updatePlots(scene::Space& space) {
    // Drop caches of deleted objects
    for (auto it = m_plots.begin(); it != m_plots.end();) {
        if (!space.find(it->first)) it = m_plots.erase(it);
        else ++it;
    }
    for (const auto& obj : space.objects()) {
        auto* p = dynamic_cast<scene::PlotObject*>(obj.get());
        if (!p) continue;
        PlotCache& cache = m_plots[p->id()];
        Color color = colorOf(*p);
        unsigned colorKey = static_cast<unsigned>(gfx::colorToInt(color));
        int dark = static_cast<int>(activeStyle().id) * 2 + (ui::theme().dark ? 1 : 0);  // style + theme
        int printKey = m_print ? printStyleOf(p->id()) * 2 + (m_printGray ? 1 : 0) : -2;
        if (cache.builtAt == p->changedAt() && cache.builtAt != 0 && cache.dark == dark && cache.color == colorKey &&
            cache.smooth == m_smoothSurfaces && cache.printKey == printKey)
            continue;

        PlotSpec spec;
        static const PlotType kinds[4] = {PlotType::Function2D, PlotType::Parametric, PlotType::Surface, PlotType::Implicit};
        spec.type = kinds[static_cast<int>(p->kind())];
        auto text = [&](const char* key) { return p->field(key) ? p->field(key)->text : std::string(); };
        spec.f = text("f");
        spec.px = text("x");
        spec.py = text("y");
        spec.pz = text("z");
        spec.xMin = static_cast<float>(p->rangeValue("xmin", -5));
        spec.xMax = static_cast<float>(p->rangeValue("xmax", 5));
        spec.yMin = static_cast<float>(p->rangeValue("ymin", -5));
        spec.yMax = static_cast<float>(p->rangeValue("ymax", 5));
        spec.uMin = static_cast<float>(p->rangeValue("umin", 0));
        spec.uMax = static_cast<float>(p->rangeValue("umax", 6.28f));
        spec.time = p->time();
        scene::PlotObject::Samples samples = p->samples();
        spec.samplesX = samples.nx;
        spec.samplesY = samples.ny;
        spec.heatmap = p->heatmap();
        spec.smooth = m_smoothSurfaces;
        spec.color = (p->customColor || p->paletteIndex > 0) ? color : Color{0, 0, 0, 0};
        const ViewStyle& style = activeStyle();
        spec.curveDefault = style.curve;
        spec.surfaceDefault = style.surface;
        spec.wire = style.wire;
        spec.tubeScale = style.lineScale;
        spec.heatSoften = style.heatSoften;
        spec.heatSoftenTo = style.text;
        if (m_print) {
            int printStyle = printStyleOf(p->id());
            spec.print = true;
            spec.heatmap = false;
            spec.smooth = false;  // flat cells with a black mesh, like the export
            spec.grayShading = m_printGray;
            spec.color = grayOf(printStyle);
            spec.dash = dashOf(printStyle);
        }
        cache.renderer.rebuild(spec, &space);
        cache.builtAt = p->changedAt();
        cache.dark = dark;
        cache.color = colorKey;
        cache.smooth = m_smoothSurfaces;
        cache.printKey = printKey;
    }
}

bool SceneView::shown(const DrawOptions& options, const SceneObject& o) {
    if (!o.visible) return false;
    if (!options.solo || !options.selection || options.selection->empty()) return true;
    return options.selection->contains(o.id());
}

bool SceneView::dimmed(const DrawOptions& options, const SceneObject& o) const {
    return m_dim && !isSelected(options, o);
}

const FieldRenderer* SceneView::fieldRenderer(scene::ObjectId id) const {
    auto it = m_fields.find(id);
    return it == m_fields.end() ? nullptr : &it->second.renderer;
}

void SceneView::updateFields(scene::Space& space) {
    for (auto it = m_fields.begin(); it != m_fields.end();) {
        if (!space.find(it->first)) it = m_fields.erase(it);
        else ++it;
    }
    const ViewStyle& style = activeStyle();
    for (const auto& obj : space.objects()) {
        auto* f = dynamic_cast<scene::VectorFieldObject*>(obj.get());
        if (!f) continue;
        FieldCache& cache = m_fields[f->id()];
        Color color = colorOf(*f);
        unsigned colorKey = static_cast<unsigned>(gfx::colorToInt(color));
        int printStyle = m_print ? printStyleOf(f->id()) : -2;
        int styleKey = ((static_cast<int>(style.id) * 2 + (ui::theme().dark ? 1 : 0)) * 64) + printStyle + 2;
        if (cache.builtAt == f->changedAt() && cache.builtAt != 0 && cache.styleKey == styleKey &&
            cache.color == colorKey)
            continue;

        FieldSpec spec;
        spec.formula = f->field("F") ? f->field("F")->text : std::string();
        spec.plane = !f->volume();
        auto range = [&](const char* key, double fallback) { return static_cast<float>(f->rangeValue(key, fallback)); };
        spec.box.min = {range("xmin", -4), range("ymin", -4), spec.plane ? 0.0f : range("zmin", -2)};
        spec.box.max = {range("xmax", 4), range("ymax", 4), spec.plane ? 0.0f : range("zmax", 2)};
        scene::VectorFieldObject::Counts counts = f->counts();
        spec.nx = counts.nx;
        spec.ny = counts.ny;
        spec.nz = counts.nz;
        spec.sameLength = f->sameLength();
        spec.heatmap = f->heatmap();
        spec.color = color;
        spec.lineColor = Color{color.r, color.g, color.b, 230};
        if (spec.heatmap) spec.lineColor = style.text;  // heat-colored arrows: neutral flow lines
        spec.particleColor = style.text;
        spec.lines = f->lines();
        spec.seeds = f->seeds();
        spec.particles = f->particles();
        spec.speed = f->speed();
        spec.time = f->time();
        spec.lineScale = style.lineScale;
        spec.heatSoften = style.heatSoften;
        spec.heatSoftenTo = style.text;
        if (m_print) {
            // Paper: grays from the export's style table, no heat map
            Color g = grayOf(printStyle);
            spec.heatmap = false;
            spec.color = g;
            spec.lineColor = g;
            spec.particleColor = Color{20, 20, 20, 255};
            spec.lineScale = 0.8f;
        }
        cache.renderer.rebuild(spec, &space);
        cache.builtAt = f->changedAt();
        cache.styleKey = styleKey;
        cache.color = colorKey;
    }
}

int SceneView::printStyleOf(scene::ObjectId id) const {
    auto it = m_printStyle.find(id);
    return it == m_printStyle.end() ? -1 : it->second;
}

void SceneView::draw3D(scene::Space& space, const DrawOptions& options) {
    VectorRenderer::setScreenScale(options.cameraPosition, options.worldPerPixel);
    m_print = options.print;
    m_printGray = options.printGrayShading;
    m_dim = options.dimUnselected && !m_print && options.selection && !options.selection->empty();
    m_printStyle.clear();
    if (m_print && options.selection) {
        auto list = latex::exportedObjects(space, *options.selection, options.printScope);
        for (size_t k = 0; k < list.size(); k++) m_printStyle[list[k]->id()] = static_cast<int>(k);
    }
    if (space.kind() == scene::SpaceKind::Euclidean3D) drawEuclidean(space, options);
    else drawComplex(space, options);
}

void SceneView::drawEuclidean(scene::Space& space, const DrawOptions& options) {
    const ViewStyle& t = activeStyle();
    const ViewSettings& view = *options.view;
    float len = view.axisLength;

    if (view.showAxes && !options.print) {
        // About 2-3 px wide on screen whatever the zoom
        float r = std::clamp(0.02f * t.lineScale, VectorRenderer::pixelsAt({0, 0, 0}, 0.75f),
                             VectorRenderer::pixelsAt({0, 0, 0}, 2.0f));
        gfx::cylinder({0, 0, 0}, mathToWorld(len, 0, 0), r, r, 8, t.axisX);
        gfx::cylinder({0, 0, 0}, mathToWorld(0, len, 0), r, r, 8, t.axisY);
        gfx::cylinder({0, 0, 0}, mathToWorld(0, 0, len), r, r, 8, t.axisZ);
    }

    updatePlots(space);
    updateFields(space);
    if (options.print) {
        // Paper: thin black axes with arrowheads, like the export
        const Color ink = {20, 20, 20, 255};
        if (view.showAxes) {
            for (int k = 0; k < 3; k++) {
                Vector3 tip = mathToWorld(k == 0 ? len : 0, k == 1 ? len : 0, k == 2 ? len : 0);
                Vector3 back = mathToWorld(k == 0 ? -len : 0, k == 1 ? -len : 0, k == 2 ? -len : 0);
                gfx::cylinder(back, tip, 0.008f, 0.008f, 6, ink);
                Vector3 base = Vector3Lerp(back, tip, 1.0f - 0.18f / (2 * len));
                gfx::cylinder(base, tip, 0.04f, 0.0f, 10, ink);
            }
        }
    } else {
        // The untransformed basis, faint, once if any matrix is shown
        bool anyMatrix = false;
        for (const auto& o : space.objects())
            if (shown(options, *o) && o->value().ok() &&
                (o->typeId() == "matrix" ||
                 (o->typeId() == "quaternion" && static_cast<const scene::QuaternionObject&>(*o).showBasis())))
                anyMatrix = true;
        if (anyMatrix) basis(Matrix4D::identity(), 0.35f, false);
        for (const Matrix4D& g : options.ghosts) basis(g, 0.22f, false);
    }
    drawObjects(space, options, false);
}

void SceneView::drawComplex(scene::Space& space, const DrawOptions& options) {
    const ViewStyle& t = activeStyle();
    const ViewSettings& view = *options.view;
    float extent = view.axisLength;
    if (options.print) {
        const Color ink = {20, 20, 20, 255}, light = {160, 160, 160, 255};
        if (view.showAxes) {
            gfx::cylinder({-extent, 0, 0}, {extent, 0, 0}, 0.008f, 0.008f, 6, ink);
            gfx::cylinder({0, -extent, 0}, {0, extent, 0}, 0.008f, 0.008f, 6, ink);
            gfx::cylinder({extent - 0.18f, 0, 0}, {extent, 0, 0}, 0.04f, 0.0f, 10, ink);
            gfx::cylinder({0, extent - 0.18f, 0}, {0, extent, 0}, 0.04f, 0.0f, 10, ink);
        }
        for (int i = 0; i < 96; i++) {
            float a0 = 2.0f * PI * i / 96, a1 = 2.0f * PI * (i + 1) / 96;
            gfx::line({std::cos(a0), std::sin(a0), 0}, {std::cos(a1), std::sin(a1), 0}, light);
        }
    } else {
        if (view.showAxes) {
            float r = std::clamp(0.015f * t.lineScale, VectorRenderer::pixelsAt({0, 0, 0}, 0.75f),
                                 VectorRenderer::pixelsAt({0, 0, 0}, 2.0f));
            gfx::cylinder({-extent, 0, 0}, {extent, 0, 0}, r, r, 6, t.axisX);
            gfx::cylinder({0, -extent, 0}, {0, extent, 0}, r, r, 6, t.axisY);
            // Tick marks at the step the axis numbers use (at most 400 per axis)
            double step = std::max(options.tickStep, extent / 200.0);
            float tick = std::min(0.08f, static_cast<float>(step) * 0.3f);
            for (int i = -static_cast<int>(extent / step); i <= static_cast<int>(extent / step); i++) {
                if (i == 0) continue;
                float f = static_cast<float>(i * step);
                gfx::line({f, -tick, 0}, {f, tick, 0}, t.axisX);
                gfx::line({-tick, f, 0}, {tick, f, 0}, t.axisY);
            }
        }
        // Unit circle
        for (int i = 0; i < 96; i++) {
            float a0 = 2.0f * PI * i / 96, a1 = 2.0f * PI * (i + 1) / 96;
            gfx::line({std::cos(a0), std::sin(a0), 0}, {std::cos(a1), std::sin(a1), 0}, t.faint);
        }
    }
    drawObjects(space, options, true);
}

void SceneView::drawObjects(const scene::Space& space, const DrawOptions& options, bool numbersOnly) const {
    for (const auto& obj : space.objects()) {
        const SceneObject& o = *obj;
        if (!shown(options, o)) continue;
        if (numbersOnly && !o.value().isNumber()) continue;  // the complex plane draws numbers only
        const ObjectView* ov = objectView(o.typeId());
        if (!ov || !ov->draw) continue;
        DrawContext ctx{*this, options};
        ctx.selected = isSelected(options, o);
        ctx.print = options.print;
        if (ctx.print) {
            ctx.printStyle = printStyleOf(o.id());
            ctx.color = grayOf(ctx.printStyle);
        } else {
            ctx.dim = dimmed(options, o);
            ctx.color = ctx.dim ? faded(colorOf(o)) : colorOf(o);
        }
        ov->draw(o, ctx);
    }
}

void SceneView::drawLabelsPrint(scene::Space& space, const DrawOptions& options, const Camera3D& camera) {
    // Paper labels: names only (as in the export), dark text on a white pill
    const ViewSettings& view = *options.view;
    float len = view.axisLength;
    const Color ink = {20, 20, 20, 255}, paper = {255, 255, 255, 220};
    LabelBatch labels(camera);
    if (view.showAxes) {
        if (space.kind() == scene::SpaceKind::Euclidean3D) {
            labels.add(mathToWorld(len, 0, 0), "x", ink, {8.0f, -8.0f}, paper, false, {0, 0, 0, 0}, 0);
            labels.add(mathToWorld(0, len, 0), "y", ink, {8.0f, -8.0f}, paper, false, {0, 0, 0, 0}, 0);
            labels.add(mathToWorld(0, 0, len), "z", ink, {8.0f, -8.0f}, paper, false, {0, 0, 0, 0}, 0);
        } else {
            labels.add({len, 0, 0}, "Re", ink, {8.0f, -8.0f}, paper, false, {0, 0, 0, 0}, 0);
            labels.add({0, len, 0}, "Im", ink, {8.0f, -8.0f}, paper, false, {0, 0, 0, 0}, 0);
        }
    }
    for (const auto& obj : space.objects()) {
        const SceneObject& o = *obj;
        if (!shown(options, o) || !o.value().ok()) continue;
        int style = printStyleOf(o.id());
        Color c = style < 0 ? Color{170, 170, 170, 255} : grayOf(style);
        const ObjectView* ov = objectView(o.typeId());
        if (ov && ov->labels) {
            std::vector<LabelSpot> spots;
            ov->labels(o, true, spots);
            for (const auto& spot : spots) labels.add(spot.at, spot.text.c_str(), c, spot.offset, paper);
            continue;
        }
        Vector3 at;
        if (!anchorPoint(o, options, at)) continue;
        labels.add(at, o.name().c_str(), c, {8.0f, -8.0f}, paper);
    }
    labels.draw();
}

void SceneView::drawLabels(scene::Space& space, const DrawOptions& options, const Camera3D& camera) {
    const ViewSettings& view = *options.view;
    const ViewStyle& t = activeStyle();
    if (!view.showLabels) return;
    float len = view.axisLength;
    if (options.print) {
        drawLabelsPrint(space, options, camera);
        return;
    }
    LabelBatch labels(camera);

    if (space.kind() == scene::SpaceKind::Euclidean3D) {
        if (view.showAxes) {
            labels.add(mathToWorld(len, 0, 0), "x", t.axisX, kOffset, t.labelBg, t.labelLarge, {0, 0, 0, 0}, 0);
            labels.add(mathToWorld(0, len, 0), "y", t.axisY, kOffset, t.labelBg, t.labelLarge, {0, 0, 0, 0}, 0);
            labels.add(mathToWorld(0, 0, len), "z", t.axisZ, kOffset, t.labelBg, t.labelLarge, {0, 0, 0, 0}, 0);
        }
    } else if (view.showAxes) {
        labels.add({len, 0, 0}, "Re", t.axisX, kOffset, t.labelBg, t.labelLarge, {0, 0, 0, 0}, 0);
        labels.add({0, len, 0}, "Im", t.axisY, kOffset, t.labelBg, t.labelLarge, {0, 0, 0, 0}, 0);
    }

    for (const auto& obj : space.objects()) {
        const SceneObject& o = *obj;
        if (!shown(options, o) || !o.value().ok()) continue;
        const bool dim = dimmed(options, o);
        const Color ring = isSelected(options, o) ? ui::theme().accent : Color{0, 0, 0, 0};
        auto shade = [&](Color c) { return dim ? faded(c) : c; };
        const ObjectView* ov = objectView(o.typeId());
        if (ov && ov->labels) {
            std::vector<LabelSpot> spots;
            ov->labels(o, false, spots);
            for (const auto& spot : spots)
                labels.add(spot.at, spot.text.c_str(), shade(colorOf(o)), spot.offset, t.labelBg, t.labelLarge, ring);
            continue;
        }
        Vector3 at;
        if (!anchorPoint(o, options, at)) continue;
        std::string text = ov && ov->label ? ov->label(o) : o.name();
        Color c = ov && ov->plainLabel ? t.text : colorOf(o);
        labels.add(at, text.c_str(), shade(c), kOffset, t.labelBg, t.labelLarge, ring);
    }
    labels.draw();
}

} // namespace linalg::view
