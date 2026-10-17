#include "view/SceneView.hpp"
#include "math/Decompositions.hpp"
#include "rendering/Labels.hpp"
#include "rendering/VectorRenderer.hpp"
#include "rendering/WorldMapping.hpp"
#include "ui/core/Theme.hpp"
#include "view/ViewStyle.hpp"
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
                DrawLineEx(it.screen, near, ui::px(1.0f), Fade(it.color, 0.6f));
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

Vector3D toVec(Vector3 v) { return {v.x, v.y, v.z}; }

bool isSelected(const DrawOptions& o, const SceneObject& obj) {
    return o.selection && o.selection->contains(obj.id());
}

Color withAlpha(Color c, unsigned char a) {
    c.a = a;
    return c;
}

// Faded toward the view's background (unselected objects while something is selected).
// Blending the color, not the alpha, keeps the depth order right.
Color faded(Color c) {
    const Color bg = activeStyle().background;
    const float k = 0.62f;
    auto mix = [&](unsigned char a, unsigned char b) { return static_cast<unsigned char>(a + (b - a) * k); };
    return {mix(c.r, bg.r), mix(c.g, bg.g), mix(c.b, bg.b), c.a};
}

// Arrow between two world points; selected arrows are thicker with a halo
void arrow(Vector3 from, Vector3 to, Color color, bool selected) {
    float k = activeStyle().lineScale;
    float radius = (selected ? 0.034f : 0.022f) * k;
    float head = (selected ? 0.24f : 0.2f) * std::sqrt(k);
    if (selected) {
        DrawCylinderEx(from, to, 0.085f * k, 0.085f * k, 12, withAlpha(color, 75));
        DrawSphere(to, 0.16f * std::sqrt(k), withAlpha(color, 60));
    }
    VectorRenderer::drawArrow(toVec(from), toVec(to), color, radius, head);
}

void dashed(Vector3 a, Vector3 b, Color color, float dash = 0.12f) {
    float len = Vector3Distance(a, b);
    if (len < 1e-6f) return;
    for (float s = 0.0f; s < len; s += 2.0f * dash) {
        float e = std::fmin(s + dash, len);
        DrawLine3D(Vector3Lerp(a, b, s / len), Vector3Lerp(a, b, e / len), color);
    }
}

// Arc in the complex plane (world XY), radius r, from angle a0 to a1
void arcXY(float r, float a0, float a1, Color color) {
    int segments = static_cast<int>(std::fabs(a1 - a0) / 0.05f) + 2;
    for (int i = 0; i < segments; i++) {
        float t0 = a0 + (a1 - a0) * i / segments, t1 = a0 + (a1 - a0) * (i + 1) / segments;
        DrawCylinderEx({r * std::cos(t0), r * std::sin(t0), 0}, {r * std::cos(t1), r * std::sin(t1), 0}, 0.012f, 0.012f, 4,
                       color);
    }
}

// --- Print preview helpers (black and white, like the LaTeX export) ---------------

Color grayOf(int styleIndex) {
    if (styleIndex < 0) return {205, 205, 205, 255};  // not exported: faint
    unsigned char v = static_cast<unsigned char>(255.0f * latex::styleFor(styleIndex).grayLevel);
    return {v, v, v, 255};
}

int dashOf(int styleIndex) {
    return styleIndex < 0 ? 0 : latex::styleFor(styleIndex).dashKind;
}

// A line with the export's dash pattern, drawn as thin cylinders
void printLine(Vector3 a, Vector3 b, Color color, int dash, float radius) {
    float len = Vector3Distance(a, b);
    if (len < 1e-6f) return;
    if (dash == 0) {
        DrawCylinderEx(a, b, radius, radius, 6, color);
        return;
    }
    float on1, off1, on2, off2;
    switch (dash) {
        case 1: on1 = 0.16f, off1 = 0.1f, on2 = 0.16f, off2 = 0.1f; break;
        case 2: on1 = 0.035f, off1 = 0.07f, on2 = 0.035f, off2 = 0.07f; break;
        case 3: on1 = 0.16f, off1 = 0.07f, on2 = 0.035f, off2 = 0.07f; break;
        default: on1 = 0.16f, off1 = 0.22f, on2 = 0.16f, off2 = 0.22f; break;
    }
    float pos = 0.0f;
    int k = 0;
    while (pos < len) {
        float on = (k % 2 == 0) ? on1 : on2, off = (k % 2 == 0) ? off1 : off2;
        float e = std::fmin(pos + on, len);
        DrawCylinderEx(Vector3Lerp(a, b, pos / len), Vector3Lerp(a, b, e / len), radius, radius, 6, color);
        pos += on + off;
        k++;
    }
}

void printArrow(Vector3 from, Vector3 to, Color color, int dash, bool selected) {
    float len = Vector3Distance(from, to);
    if (len < 1e-6f) return;
    float head = std::fmin(0.22f, len * 0.4f);
    Vector3 base = Vector3Lerp(from, to, (len - head) / len);
    if (selected) DrawCylinderEx(from, to, 0.06f, 0.06f, 10, Color{0, 0, 0, 30});  // soft gray halo on paper
    printLine(from, base, color, dash, 0.016f);
    DrawCylinderEx(base, to, 0.065f, 0.0f, 12, color);
}

Color paletteColor(const std::vector<Color>& palette, int index) {
    return palette[static_cast<size_t>(index) % palette.size()];
}

void basis(const Matrix4D& m, float alpha, bool selected, bool fade = false) {
    const ViewStyle& t = activeStyle();
    Vector3D origin = m.transform(Vector3D::zero());
    Vector3 o = mathToWorld(origin);
    Vector3D axes[3] = {m.transformDirection({1, 0, 0}), m.transformDirection({0, 1, 0}), m.transformDirection({0, 0, 1})};
    Color colors[3] = {t.basisX, t.basisY, t.basisZ};
    if (fade)
        for (Color& c : colors) c = faded(c);
    float radius = (selected ? 0.04f : 0.03f) * t.lineScale;
    for (int i = 0; i < 3; i++) {
        Vector3 tip = mathToWorld(origin + axes[i]);
        Color c = withAlpha(colors[i], static_cast<unsigned char>(255 * alpha));
        if (selected && alpha > 0.9f)
            DrawCylinderEx(o, tip, 0.08f * t.lineScale, 0.08f * t.lineScale, 8, withAlpha(colors[i], 50));
        DrawCylinderEx(o, tip, radius, radius, 8, c);
        DrawSphere(tip, selected ? 0.11f : 0.09f, c);
    }
    DrawSphere(o, 0.06f, withAlpha(t.text, static_cast<unsigned char>(255 * alpha)));
}

// --- Quaternions: axis and angle, rotated basis, a vector before and after ----------------

constexpr float kQuatAxisLength = 2.0f;  // the axis arrow (the label sits at its tip)

Matrix4D embed3(const Matrix3D& m) {
    Matrix4D r = Matrix4D::identity();
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) r.m[i][j] = m.m[i][j];
    return r;
}

// A unit vector at right angles to the axis (where the angle arc starts)
Vector3D perpendicular(const Vector3D& a) {
    Vector3D e = std::fabs(a.x) < 0.9f ? Vector3D(1, 0, 0) : Vector3D(0, 1, 0);
    return a.cross(e).normalized();
}

// Points of an arc around `axis` (through `center`), from `start` (relative to center) over `angle`
std::vector<Vector3D> arcPoints(const Vector3D& center, const Vector3D& axis, const Vector3D& start, double angle) {
    std::vector<Vector3D> pts;
    int n = std::max(2, static_cast<int>(std::fabs(angle) / 0.06) + 2);
    Vector3D side = axis.cross(start);
    for (int i = 0; i <= n; i++) {
        float a = static_cast<float>(angle * i / n);
        pts.push_back(center + start * std::cos(a) + side * std::sin(a));
    }
    return pts;
}

void tubeAlong(const std::vector<Vector3D>& pts, float radius, Color color) {
    for (size_t i = 1; i < pts.size(); i++)
        DrawCylinderEx(mathToWorld(pts[i - 1]), mathToWorld(pts[i]), radius, radius, 6, color);
}

// An arc with an arrowhead at its end (the direction of turning)
void angleArc(const std::vector<Vector3D>& pts, float radius, Color color) {
    if (pts.size() < 2) return;
    tubeAlong(pts, radius, color);
    Vector3D end = pts.back(), before = pts[pts.size() - 2];
    Vector3D d = end - before;
    if (d.magnitude() < 1e-6f) return;
    d = d.normalized();
    float head = 0.14f * std::sqrt(activeStyle().lineScale);
    DrawCylinderEx(mathToWorld(end - d * (head * 0.3f)), mathToWorld(end + d * (head * 0.7f)), head * 0.38f, 0.0f, 10,
                   color);
}

} // namespace

Color SceneView::colorOf(const SceneObject& o) {
    if (o.customColor) return {o.color.r, o.color.g, o.color.b, 255};
    const ViewStyle& s = activeStyle();
    const std::string& type = o.typeId();
    if (type == "vector" || type == "quaternion") return paletteColor(s.vectors, o.paletteIndex);
    if (type == "eigen") return s.eigen;
    if (type == "function" || type == "curve" || type == "surface" || type == "implicit" || type == "field" ||
        type == "doubleintegral" || type == "lineintegral")
        return paletteColor(s.plots, o.paletteIndex);
    if (type == "complex") return paletteColor(s.complexes, o.paletteIndex);
    if (type == "product") return s.product;
    return ui::theme().accent;  // matrix, parameter (list dots; matrices draw their basis colors)
}

Matrix4D SceneView::displayedMatrix(const SceneObject& o, const DrawOptions& options) {
    if (o.id() == options.overrideMatrix) return options.overrideValue;
    return o.value().matrix4();
}

bool SceneView::anchorPoint(const SceneObject& o, scene::SpaceKind kind, const DrawOptions& options, Vector3& out) {
    const expr::Value& v = o.value();
    if (kind == scene::SpaceKind::Euclidean3D) {
        if (o.typeId() == "vector" && v.type == expr::Type::Vector) {
            Vector3D tail = static_cast<const scene::VectorObject&>(o).tail();
            out = mathToWorld(tail + v.vector);
            return true;
        }
        if (o.typeId() == "matrix" && v.type == expr::Type::Matrix) {
            Matrix4D m = displayedMatrix(o, options);
            out = mathToWorld(m.transform(Vector3D(1, 0, 0)));
            return true;
        }
        if (o.typeId() == "doubleintegral") {
            auto& d = static_cast<const scene::DoubleIntegralObject&>(o);
            integrals::Rect r = d.rect();
            double top = 0;
            for (const auto& b : d.riemann().boxes) top = std::max(top, b.height);
            out = mathToWorld(static_cast<float>((r.x0 + r.x1) / 2), static_cast<float>((r.y0 + r.y1) / 2),
                              static_cast<float>(top) + 0.3f);
            return true;
        }
        if (o.typeId() == "lineintegral") {
            integrals::D3 m = static_cast<const scene::LineIntegralObject&>(o).marker();
            out = mathToWorld(static_cast<float>(m.x), static_cast<float>(m.y), static_cast<float>(m.z));
            return true;
        }
        if (o.typeId() == "quaternion") {
            expr::Rotation r;
            // The label and the picking handle sit at the axis tip, so only when the axis is drawn
            if (!static_cast<const scene::QuaternionObject&>(o).showAxis() || !expr::rotationOf(v, r) || !r.hasAxis)
                return false;
            out = mathToWorld(r.axis * kQuatAxisLength);
            return true;
        }
        return false;
    }
    if ((o.typeId() == "complex" || o.typeId() == "product") && v.isNumber()) {
        out = {static_cast<float>(v.re), static_cast<float>(v.im), 0.0f};
        return true;
    }
    return false;
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
        unsigned colorKey = static_cast<unsigned>(ColorToInt(color));
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
            int style = printStyleOf(p->id());
            spec.print = true;
            spec.heatmap = false;
            spec.smooth = false;  // flat cells with a black mesh, like the export
            spec.grayShading = m_printGray;
            spec.color = grayOf(style);
            spec.dash = dashOf(style);
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
        unsigned colorKey = static_cast<unsigned>(ColorToInt(color));
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
        float r = 0.02f * t.lineScale;
        DrawCylinderEx({0, 0, 0}, mathToWorld(len, 0, 0), r, r, 8, t.axisX);
        DrawCylinderEx({0, 0, 0}, mathToWorld(0, len, 0), r, r, 8, t.axisY);
        DrawCylinderEx({0, 0, 0}, mathToWorld(0, 0, len), r, r, 8, t.axisZ);
    }

    updatePlots(space);
    updateFields(space);
    if (options.print) {
        drawEuclideanPrint(space, options);
        return;
    }

    // The untransformed basis, faint, once if any matrix is shown
    bool anyMatrix = false;
    for (const auto& o : space.objects())
        if (shown(options, *o) && o->value().ok() &&
            (o->typeId() == "matrix" ||
             (o->typeId() == "quaternion" && static_cast<const scene::QuaternionObject&>(*o).showBasis())))
            anyMatrix = true;
    if (anyMatrix) basis(Matrix4D::identity(), 0.35f, false);

    for (const Matrix4D& g : options.ghosts) basis(g, 0.22f, false);

    for (const auto& obj : space.objects()) {
        const SceneObject& o = *obj;
        if (!shown(options, o)) continue;
        bool selected = isSelected(options, o);
        const expr::Value& v = o.value();
        const std::string& type = o.typeId();

        if (type == "vector" && v.type == expr::Type::Vector) {
            Vector3D tail = static_cast<const scene::VectorObject&>(o).tail();
            Color c = dimmed(options, o) ? faded(colorOf(o)) : colorOf(o);
            arrow(mathToWorld(tail), mathToWorld(tail + v.vector), c, selected);
        } else if (type == "matrix" && v.type == expr::Type::Matrix) {
            basis(displayedMatrix(o, options), 1.0f, selected, dimmed(options, o));
        } else if (type == "doubleintegral" && o.error().empty()) {
            bool dim = dimmed(options, o);
            drawDoubleIntegral(static_cast<const scene::DoubleIntegralObject&>(o), dim ? faded(colorOf(o)) : colorOf(o),
                               dim, false, BLACK);
        } else if (type == "lineintegral" && o.error().empty()) {
            bool dim = dimmed(options, o);
            drawLineIntegral(static_cast<const scene::LineIntegralObject&>(o), dim ? faded(colorOf(o)) : colorOf(o),
                             dim, false, BLACK, selected);
        } else if (type == "quaternion" && v.type == expr::Type::Quaternion) {
            drawQuaternion(static_cast<const scene::QuaternionObject&>(o), selected,
                           dimmed(options, o) ? faded(colorOf(o)) : colorOf(o), dimmed(options, o), false, -1);
        } else if (type == "eigen" && v.type == expr::Type::Matrix) {
            EigenResult e = eigen3(v.matrix3());
            Color c = dimmed(options, o) ? faded(colorOf(o)) : colorOf(o);
            for (const auto& ev : e.values) {
                if (!ev.isReal() || !ev.hasVector) continue;
                dashed(mathToWorld(ev.vector * -len), mathToWorld(ev.vector * len), withAlpha(c, selected ? 255 : 180),
                       0.25f);
                VectorRenderer::drawArrow(Vector3D::zero(), toVec(mathToWorld(ev.vector)), withAlpha(c, 210), 0.015f, 0.15f);
                DrawSphere(mathToWorld(ev.vector * ev.re), selected ? 0.09f : 0.07f, c);
            }
        } else if (dynamic_cast<const scene::PlotObject*>(&o)) {
            auto it = m_plots.find(o.id());
            if (it != m_plots.end() && it->second.renderer.error().empty())
                it->second.renderer.draw(dimmed(options, o) ? 0.3f : 1.0f);
        } else if (type == "field" && o.error().empty()) {
            auto it = m_fields.find(o.id());
            if (it != m_fields.end() && it->second.renderer.error().empty())
                it->second.renderer.draw(o.time(), dimmed(options, o) ? 0.3f : 1.0f);
        }
    }
}

void SceneView::drawComplex(scene::Space& space, const DrawOptions& options) {
    const ViewStyle& t = activeStyle();
    const ViewSettings& view = *options.view;
    float extent = view.axisLength;
    if (options.print) {
        drawComplexPrint(space, options);
        return;
    }

    if (view.showAxes) {
        float r = 0.015f * t.lineScale;
        DrawCylinderEx({-extent, 0, 0}, {extent, 0, 0}, r, r, 6, t.axisX);
        DrawCylinderEx({0, -extent, 0}, {0, extent, 0}, r, r, 6, t.axisY);
        for (int i = -static_cast<int>(extent); i <= static_cast<int>(extent); i++) {
            if (i == 0) continue;
            float f = static_cast<float>(i);
            DrawLine3D({f, -0.08f, 0}, {f, 0.08f, 0}, t.axisX);
            DrawLine3D({-0.08f, f, 0}, {0.08f, f, 0}, t.axisY);
        }
    }
    // Unit circle
    for (int i = 0; i < 96; i++) {
        float a0 = 2.0f * PI * i / 96, a1 = 2.0f * PI * (i + 1) / 96;
        DrawLine3D({std::cos(a0), std::sin(a0), 0}, {std::cos(a1), std::sin(a1), 0}, t.faint);
    }

    for (const auto& obj : space.objects()) {
        const SceneObject& o = *obj;
        if (!shown(options, o) || !o.value().isNumber()) continue;
        bool selected = isSelected(options, o);
        const expr::Value& v = o.value();
        float re = static_cast<float>(v.re), im = static_cast<float>(v.im);
        Vector3 tip = {re, im, 0};
        Color c = dimmed(options, o) ? faded(colorOf(o)) : colorOf(o);

        if (o.typeId() == "product") {
            const expr::Value& a = o.field("a")->value;
            const expr::Value& b = o.field("b")->value;
            if (o.propBool("angles", true) && a.isNumber() && b.isNumber()) {
                float ar = static_cast<float>(a.re), ai = static_cast<float>(a.type == expr::Type::Complex ? a.im : 0);
                float br = static_cast<float>(b.re), bi = static_cast<float>(b.type == expr::Type::Complex ? b.im : 0);
                float argA = std::atan2(ai, ar), argB = std::atan2(bi, br);
                float lenB = std::hypot(br, bi), lenA = std::hypot(ar, ai);
                // Rotation step: a turned by arg b; scaling step: stretched by |b|
                Vector3 rotated = {lenA * std::cos(argA + argB), lenA * std::sin(argA + argB), 0};
                if (lenB > 1e-6f) {
                    VectorRenderer::drawArrow(Vector3D::zero(), toVec(rotated), withAlpha(c, 90), 0.015f, 0.15f);
                    dashed(rotated, tip, c, 0.08f);
                }
                arcXY(0.35f, 0.0f, argA, paletteColor({t.z1}, 0));
                arcXY(0.5f, argA, argA + argB, t.z2);
            }
            arrow({0, 0, 0}, tip, c, selected);
            continue;
        }

        // Complex number, with optional Euler-style extras
        float r = std::hypot(re, im);
        float theta = std::atan2(im, re);
        if (theta < 0 && o.propBool("helix")) theta += 2.0f * PI;
        if (o.propBool("projections")) {
            dashed(tip, {re, 0, 0}, t.muted, 0.08f);
            dashed(tip, {0, im, 0}, t.muted, 0.08f);
            DrawCylinderEx({0, 0, 0}, {re, 0, 0}, 0.035f, 0.035f, 6, t.axisX);
            DrawCylinderEx({0, 0, 0}, {0, im, 0}, 0.035f, 0.035f, 6, t.axisY);
        }
        if (o.propBool("circle") && std::fabs(r - 1.0f) > 1e-3f) {
            for (int i = 0; i < 96; i++) {
                float a0 = 2.0f * PI * i / 96, a1 = 2.0f * PI * (i + 1) / 96;
                DrawLine3D({r * std::cos(a0), r * std::sin(a0), 0}, {r * std::cos(a1), r * std::sin(a1), 0}, withAlpha(c, 120));
            }
        }
        if (o.propBool("helix")) {
            // r·e^(it) for t in [0, θ], rising along Z: its shadow is the circle
            const float pitch = 0.3f, step = 0.05f;
            Vector3 prev = {r, 0, 0};
            for (float s = step; s <= theta + 1e-4f; s += step) {
                Vector3 next = {r * std::cos(s), r * std::sin(s), s * pitch};
                DrawLine3D(prev, next, t.helix);
                prev = next;
            }
            Vector3 top = {re, im, theta * pitch};
            DrawSphere(top, 0.04f, t.helix);
            dashed(top, tip, t.helix, 0.08f);
            arcXY(0.2f, 0.0f, theta, c);
        }
        arrow({0, 0, 0}, tip, c, selected);
    }
}

void SceneView::drawQuaternion(const scene::QuaternionObject& o, bool selected, Color c, bool fade, bool print,
                               int printStyle) {
    expr::Rotation r;
    if (!expr::rotationOf(o.value(), r)) return;
    const ViewStyle& st = activeStyle();
    const float k = print ? 0.8f : st.lineScale;
    const Color muted = print ? Color{150, 150, 150, 255} : (fade ? faded(st.muted) : st.muted);
    if (o.showBasis()) {
        if (print) {
            for (int axisIndex = 0; axisIndex < 3; axisIndex++) {
                Vector3D e(axisIndex == 0 ? 1.0f : 0.0f, axisIndex == 1 ? 1.0f : 0.0f, axisIndex == 2 ? 1.0f : 0.0f);
                printArrow({0, 0, 0}, mathToWorld(r.matrix.transform(e)), c, dashOf(printStyle), selected);
            }
        } else {
            basis(embed3(r.matrix), 1.0f, selected, fade);
        }
    }
    if (o.showAxis() && r.hasAxis) {
        // The axis: a dashed line through the origin, an arrow along +axis
        Vector3D a = r.axis;
        if (print) {
            printLine(mathToWorld(a * -2.6f), mathToWorld(a * 2.6f), c, 1, 0.008f);
            printArrow({0, 0, 0}, mathToWorld(a * kQuatAxisLength), c, 0, selected);
        } else {
            dashed(mathToWorld(a * -2.6f), mathToWorld(a * 2.6f), withAlpha(c, 200), 0.1f);
            if (selected) DrawCylinderEx({0, 0, 0}, mathToWorld(a * kQuatAxisLength), 0.06f * k, 0.06f * k, 10, withAlpha(c, 60));
            VectorRenderer::drawArrow(Vector3D::zero(), toVec(mathToWorld(a * kQuatAxisLength)), c, 0.02f * k, 0.17f);
        }
        // The angle: an arc around the axis, from a direction at right angles to the axis
        Vector3D u = perpendicular(a) * 1.0f;
        auto pts = arcPoints(Vector3D::zero(), a, u, r.angle);
        Color arc = c;
        if (print) {
            for (size_t n = 1; n < pts.size(); n++) printLine(mathToWorld(pts[n - 1]), mathToWorld(pts[n]), arc, 0, 0.01f);
        } else {
            angleArc(pts, 0.018f * k, arc);
            DrawLine3D({0, 0, 0}, mathToWorld(pts.front()), withAlpha(arc, 140));
            DrawLine3D({0, 0, 0}, mathToWorld(pts.back()), withAlpha(arc, 140));
        }
    }
    if (o.actsOnVector()) {
        const scene::Field* vf = o.field("v");
        if (vf && vf->value.type == expr::Type::Vector) {
            Vector3D v = vf->value.vector, w = r.matrix.transform(v);
            // v (before), q v q⁻¹ (after), and the circle its tip follows around the axis
            Vector3D along = r.axis * r.axis.dot(v);
            auto path = arcPoints(along, r.axis, v - along, r.angle);
            if (print) {
                printArrow({0, 0, 0}, mathToWorld(v), muted, 2, false);
                printArrow({0, 0, 0}, mathToWorld(w), c, 0, false);
                for (size_t n = 1; n < path.size(); n++) printLine(mathToWorld(path[n - 1]), mathToWorld(path[n]), muted, 2, 0.006f);
            } else {
                arrow({0, 0, 0}, mathToWorld(v), muted, false);
                arrow({0, 0, 0}, mathToWorld(w), c, false);
                for (size_t n = 1; n < path.size(); n += 2) DrawLine3D(mathToWorld(path[n - 1]), mathToWorld(path[n]), c);
            }
        }
    }
}

namespace {

Vector3 w3(const integrals::D3& p) {
    return mathToWorld(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z));
}

Color mixColor(Color a, Color b, float k) {
    k = std::clamp(k, 0.0f, 1.0f);
    auto m = [&](unsigned char x, unsigned char y) { return static_cast<unsigned char>(x + (y - x) * k); };
    return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a)};
}

} // namespace

void SceneView::drawDoubleIntegral(const scene::DoubleIntegralObject& o, Color c, bool fade, bool print, Color ink) {
    const ViewStyle& st = activeStyle();
    if (o.showBoxes()) {
        Color up = print ? Color{215, 215, 215, 255} : withAlpha(c, 120);
        Color down = print ? Color{170, 170, 170, 255} : withAlpha(fade ? faded(st.axisX) : st.axisX, 120);
        Color edge = print ? ink : withAlpha(fade ? faded(st.text) : st.text, 90);
        for (const auto& b : o.riemann().boxes) {
            float h = static_cast<float>(b.height);
            Vector3 center = mathToWorld(static_cast<float>((b.x0 + b.x1) / 2), static_cast<float>((b.y0 + b.y1) / 2),
                                         h / 2);
            Vector3 size = {static_cast<float>(b.x1 - b.x0), std::fabs(h), static_cast<float>(b.y1 - b.y0)};
            DrawCubeV(center, size, h >= 0 ? up : down);
            DrawCubeWiresV(center, size, edge);
        }
    }
    if (o.showSurface()) {
        Color line = print ? ink : (fade ? faded(c) : c);
        for (const auto& l : o.surfaceLines())
            for (size_t i = 1; i < l.size(); i++) DrawLine3D(w3(l[i - 1]), w3(l[i]), line);
    }
}

void SceneView::drawLineIntegral(const scene::LineIntegralObject& o, Color c, bool fade, bool print, Color ink,
                                 bool selected) {
    const ViewStyle& st = activeStyle();
    const auto& path = o.path();
    if (path.size() < 2) return;
    float k = print ? 0.8f : st.lineScale;
    // The path colored by F · T: green where F helps the motion, red where it opposes it
    Color helps = print ? Color{40, 40, 40, 255} : (fade ? faded(st.axisY) : st.axisY);
    Color opposes = print ? Color{160, 160, 160, 255} : (fade ? faded(st.axisX) : st.axisX);
    Color neutral = print ? Color{110, 110, 110, 255} : (fade ? faded(st.muted) : st.muted);
    double scale = o.maxAlong() > 1e-12 ? 1.0 / o.maxAlong() : 0.0;
    size_t done = static_cast<size_t>(std::lround(o.upto() * (path.size() - 1)));
    for (size_t i = 1; i < path.size(); i++) {
        double a = (path[i - 1].along + path[i].along) / 2 * scale;
        Color col = a >= 0 ? mixColor(neutral, helps, static_cast<float>(a)) : mixColor(neutral, opposes, static_cast<float>(-a));
        float r = (i <= done ? 0.035f : 0.018f) * k;  // the part travelled so far is thicker
        DrawCylinderEx(w3(path[i - 1].p), w3(path[i].p), r, r, 6, col);
    }
    if (selected && !print) {
        for (size_t i = 1; i < path.size(); i++)
            DrawCylinderEx(w3(path[i - 1].p), w3(path[i].p), 0.07f * k, 0.07f * k, 8, withAlpha(c, 45));
    }
    // F along the path (the longest arrow 0.7 units)
    double longest = 0;
    for (const auto& a : o.arrows()) longest = std::max(longest, std::sqrt(a.f.x * a.f.x + a.f.y * a.f.y + a.f.z * a.f.z));
    if (longest > 1e-12) {
        double s = 0.7 / longest;
        Color arrowColor = print ? ink : (fade ? faded(c) : c);
        for (const auto& a : o.arrows()) {
            integrals::D3 tip = {a.p.x + a.f.x * s, a.p.y + a.f.y * s, a.p.z + a.f.z * s};
            VectorRenderer::drawArrow(toVec(w3(a.p)), toVec(w3(tip)), arrowColor, 0.012f * k, 0.12f);
        }
    }
    DrawSphere(w3(o.marker()), 0.08f * std::sqrt(k), print ? ink : (fade ? faded(c) : c));
}

void SceneView::drawEuclideanPrint(scene::Space& space, const DrawOptions& options) {
    const ViewSettings& view = *options.view;
    float len = view.axisLength;
    const Color ink = {20, 20, 20, 255};
    if (view.showAxes) {
        for (int k = 0; k < 3; k++) {
            Vector3 tip = mathToWorld(k == 0 ? len : 0, k == 1 ? len : 0, k == 2 ? len : 0);
            Vector3 back = mathToWorld(k == 0 ? -len : 0, k == 1 ? -len : 0, k == 2 ? -len : 0);
            DrawCylinderEx(back, tip, 0.008f, 0.008f, 6, ink);
            Vector3 base = Vector3Lerp(back, tip, 1.0f - 0.18f / (2 * len));
            DrawCylinderEx(base, tip, 0.04f, 0.0f, 10, ink);
        }
    }
    const Color faintGray = {150, 150, 150, 255}, cubeGray = {185, 185, 185, 255};
    for (const auto& obj : space.objects()) {
        const SceneObject& o = *obj;
        if (!shown(options, o)) continue;
        int style = printStyleOf(o.id());
        Color c = grayOf(style);
        int dash = dashOf(style);
        bool selected = isSelected(options, o);
        const expr::Value& v = o.value();
        const std::string& type = o.typeId();
        if (type == "vector" && v.type == expr::Type::Vector) {
            Vector3D tail = static_cast<const scene::VectorObject&>(o).tail();
            printArrow(mathToWorld(tail), mathToWorld(tail + v.vector), c, dash, selected);
        } else if (type == "matrix" && v.type == expr::Type::Matrix) {
            // Like the export: original basis dotted, image of the unit cube in light gray, moved basis
            Matrix4D m = displayedMatrix(o, options);
            Vector3D origin = m.transform(Vector3D::zero());
            Vector3D cols[3] = {m.transformDirection({1, 0, 0}), m.transformDirection({0, 1, 0}),
                                m.transformDirection({0, 0, 1})};
            for (int k = 0; k < 3; k++)
                printLine({0, 0, 0}, mathToWorld(k == 0 ? 1 : 0, k == 1 ? 1 : 0, k == 2 ? 1 : 0), faintGray, 2, 0.008f);
            auto corner = [&](int a, int b, int cc) { return mathToWorld(origin + cols[0] * a + cols[1] * b + cols[2] * cc); };
            for (int a = 0; a < 2; a++)
                for (int b = 0; b < 2; b++)
                    for (int cc = 0; cc < 2; cc++) {
                        if (a == 0) DrawLine3D(corner(a, b, cc), corner(1, b, cc), cubeGray);
                        if (b == 0) DrawLine3D(corner(a, b, cc), corner(a, 1, cc), cubeGray);
                        if (cc == 0) DrawLine3D(corner(a, b, cc), corner(a, b, 1), cubeGray);
                    }
            for (int k = 0; k < 3; k++) printArrow(mathToWorld(origin), mathToWorld(origin + cols[k]), c, dash, selected);
        } else if (type == "doubleintegral" && o.error().empty()) {
            drawDoubleIntegral(static_cast<const scene::DoubleIntegralObject&>(o), c, false, true, c);
        } else if (type == "lineintegral" && o.error().empty()) {
            drawLineIntegral(static_cast<const scene::LineIntegralObject&>(o), c, false, true, c, false);
        } else if (type == "quaternion" && v.type == expr::Type::Quaternion) {
            drawQuaternion(static_cast<const scene::QuaternionObject&>(o), selected, c, false, true, style);
        } else if (type == "eigen" && v.type == expr::Type::Matrix) {
            EigenResult e = eigen3(v.matrix3());
            for (const auto& ev : e.values) {
                if (!ev.isReal() || !ev.hasVector) continue;
                float l = std::fmax(2.0f, std::fabs(ev.re) * 1.3f);
                printLine(mathToWorld(ev.vector * -l), mathToWorld(ev.vector * l), c, 1, 0.008f);
                DrawSphere(mathToWorld(ev.vector * ev.re), 0.05f, c);
            }
        } else if (dynamic_cast<const scene::PlotObject*>(&o)) {
            auto it = m_plots.find(o.id());
            if (it != m_plots.end() && it->second.renderer.error().empty()) it->second.renderer.draw();
        } else if (type == "field" && o.error().empty()) {
            auto it = m_fields.find(o.id());
            if (it != m_fields.end() && it->second.renderer.error().empty()) it->second.renderer.draw(o.time());
        }
    }
}

void SceneView::drawComplexPrint(scene::Space& space, const DrawOptions& options) {
    const ViewSettings& view = *options.view;
    float extent = view.axisLength;
    const Color ink = {20, 20, 20, 255}, light = {160, 160, 160, 255};
    if (view.showAxes) {
        DrawCylinderEx({-extent, 0, 0}, {extent, 0, 0}, 0.008f, 0.008f, 6, ink);
        DrawCylinderEx({0, -extent, 0}, {0, extent, 0}, 0.008f, 0.008f, 6, ink);
        DrawCylinderEx({extent - 0.18f, 0, 0}, {extent, 0, 0}, 0.04f, 0.0f, 10, ink);
        DrawCylinderEx({0, extent - 0.18f, 0}, {0, extent, 0}, 0.04f, 0.0f, 10, ink);
    }
    for (int i = 0; i < 96; i++) {
        float a0 = 2.0f * PI * i / 96, a1 = 2.0f * PI * (i + 1) / 96;
        DrawLine3D({std::cos(a0), std::sin(a0), 0}, {std::cos(a1), std::sin(a1), 0}, light);
    }
    for (const auto& obj : space.objects()) {
        const SceneObject& o = *obj;
        if (!shown(options, o) || !o.value().isNumber()) continue;
        int style = printStyleOf(o.id());
        Color c = grayOf(style);
        const expr::Value& v = o.value();
        float re = static_cast<float>(v.re), im = static_cast<float>(v.type == expr::Type::Complex ? v.im : 0.0);
        if (o.typeId() == "product" && o.propBool("angles", true)) {
            const expr::Value& a = o.field("a")->value;
            const expr::Value& b = o.field("b")->value;
            if (a.isNumber() && b.isNumber()) {
                float argA = std::atan2(static_cast<float>(a.type == expr::Type::Complex ? a.im : 0), static_cast<float>(a.re));
                float argB = std::atan2(static_cast<float>(b.type == expr::Type::Complex ? b.im : 0), static_cast<float>(b.re));
                arcXY(0.35f, 0.0f, argA, Color{110, 110, 110, 255});
                arcXY(0.5f, argA, argA + argB, Color{110, 110, 110, 255});
            }
        }
        float r = std::hypot(re, im);
        if (o.propBool("circle") && std::fabs(r - 1.0f) > 1e-3f) {
            for (int i = 0; i < 96; i += 2) {
                float a0 = 2.0f * PI * i / 96, a1 = 2.0f * PI * (i + 1) / 96;
                DrawLine3D({r * std::cos(a0), r * std::sin(a0), 0}, {r * std::cos(a1), r * std::sin(a1), 0}, c);
            }
        }
        if (o.propBool("projections")) {
            printLine({re, 0, 0}, {re, im, 0}, c, 1, 0.006f);
            printLine({0, im, 0}, {re, im, 0}, c, 1, 0.006f);
        }
        printArrow({0, 0, 0}, {re, im, 0}, c, dashOf(style), isSelected(options, o));
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
        if (o.typeId() == "eigen") {
            EigenResult e = eigen3(o.value().matrix3());
            for (const auto& ev : e.values) {
                if (!ev.isReal() || !ev.hasVector) continue;
                char text[48];
                snprintf(text, sizeof(text), "λ = %.3g", ev.re);
                labels.add(mathToWorld(ev.vector * ev.re), text, c, {8.0f, 14.0f}, paper);
            }
            continue;
        }
        Vector3 at;
        if (!anchorPoint(o, space.kind(), options, at)) continue;
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
        char text[96];
        Vector3 at;
        if (o.typeId() == "eigen") {
            EigenResult e = eigen3(o.value().matrix3());
            for (const auto& ev : e.values) {
                if (!ev.isReal() || !ev.hasVector) continue;
                snprintf(text, sizeof(text), "λ = %.2f", ev.re);
                labels.add(mathToWorld(ev.vector * ev.re), text, shade(colorOf(o)), {8.0f, 14.0f}, t.labelBg, t.labelLarge, ring);
            }
            continue;
        }
        if (!anchorPoint(o, space.kind(), options, at)) continue;
        if (o.typeId() == "vector") {
            snprintf(text, sizeof(text), "%s  %.2f", o.name().c_str(), o.value().vector.magnitude());
        } else if (o.typeId() == "doubleintegral") {
            snprintf(text, sizeof(text), "%s = %.3f", o.name().c_str(), o.value().re);
        } else if (o.typeId() == "lineintegral") {
            snprintf(text, sizeof(text), "%s = %.3f", o.name().c_str(), o.value().re);
        } else if (o.typeId() == "quaternion") {
            expr::Rotation r;
            expr::rotationOf(o.value(), r);
            auto c3 = [](float v) { return std::fabs(v) < 5e-3f ? 0.0f : v; };
            if (r.hasAxis)
                snprintf(text, sizeof(text), "%s  %.0f° about (%.2g, %.2g, %.2g)", o.name().c_str(),
                         r.angle * 180.0 / 3.14159265358979, c3(r.axis.x), c3(r.axis.y), c3(r.axis.z));
            else
                snprintf(text, sizeof(text), "%s  no rotation", o.name().c_str());
        } else {
            snprintf(text, sizeof(text), "%s", o.name().c_str());
        }
        Color c = o.typeId() == "matrix" ? t.text : colorOf(o);
        labels.add(at, text, shade(c), kOffset, t.labelBg, t.labelLarge, ring);
    }
    labels.draw();
}

} // namespace linalg::view
