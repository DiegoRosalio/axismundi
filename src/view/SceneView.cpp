#include "view/SceneView.hpp"
#include "math/Decompositions.hpp"
#include "rendering/Labels.hpp"
#include "rendering/VectorRenderer.hpp"
#include "rendering/WorldMapping.hpp"
#include "ui/core/Theme.hpp"
#include "view/ViewStyle.hpp"
#include <raymath.h>
#include <cmath>
#include <cstdio>

namespace linalg::view {

using scene::SceneObject;

namespace {

constexpr Vector2 kOffset = {8.0f, -8.0f};  // default label offset (design units)

Vector3D toVec(Vector3 v) { return {v.x, v.y, v.z}; }

bool isSelected(const DrawOptions& o, const SceneObject& obj) {
    return o.selection && o.selection->contains(obj.id());
}

Color withAlpha(Color c, unsigned char a) {
    c.a = a;
    return c;
}

// Arrow between two world points; selected arrows are thicker with a halo
void arrow(Vector3 from, Vector3 to, Color color, bool selected) {
    float k = activeStyle().lineScale;
    float radius = (selected ? 0.034f : 0.022f) * k;
    float head = (selected ? 0.24f : 0.2f) * std::sqrt(k);
    if (selected) DrawCylinderEx(from, to, 0.075f * k, 0.075f * k, 10, withAlpha(color, 55));
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

void basis(const Matrix4D& m, float alpha, bool selected) {
    const ViewStyle& t = activeStyle();
    Vector3D origin = m.transform(Vector3D::zero());
    Vector3 o = mathToWorld(origin);
    Vector3D axes[3] = {m.transformDirection({1, 0, 0}), m.transformDirection({0, 1, 0}), m.transformDirection({0, 0, 1})};
    Color colors[3] = {t.basisX, t.basisY, t.basisZ};
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

} // namespace

Color SceneView::colorOf(const SceneObject& o) {
    if (o.customColor) return {o.color.r, o.color.g, o.color.b, 255};
    const ViewStyle& s = activeStyle();
    const std::string& type = o.typeId();
    if (type == "vector") return paletteColor(s.vectors, o.paletteIndex);
    if (type == "eigen") return s.eigen;
    if (type == "function" || type == "curve" || type == "surface" || type == "implicit")
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

int SceneView::printStyleOf(scene::ObjectId id) const {
    auto it = m_printStyle.find(id);
    return it == m_printStyle.end() ? -1 : it->second;
}

void SceneView::draw3D(scene::Space& space, const DrawOptions& options) {
    m_print = options.print;
    m_printGray = options.printGrayShading;
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
    if (options.print) {
        drawEuclideanPrint(space, options);
        return;
    }

    // The untransformed basis, faint, once if any matrix is shown
    bool anyMatrix = false;
    for (const auto& o : space.objects())
        if (o->visible && o->typeId() == "matrix" && o->value().ok()) anyMatrix = true;
    if (anyMatrix) basis(Matrix4D::identity(), 0.35f, false);

    for (const Matrix4D& g : options.ghosts) basis(g, 0.22f, false);

    for (const auto& obj : space.objects()) {
        const SceneObject& o = *obj;
        if (!o.visible) continue;
        bool selected = isSelected(options, o);
        const expr::Value& v = o.value();
        const std::string& type = o.typeId();

        if (type == "vector" && v.type == expr::Type::Vector) {
            Vector3D tail = static_cast<const scene::VectorObject&>(o).tail();
            arrow(mathToWorld(tail), mathToWorld(tail + v.vector), colorOf(o), selected);
        } else if (type == "matrix" && v.type == expr::Type::Matrix) {
            basis(displayedMatrix(o, options), 1.0f, selected);
        } else if (type == "eigen" && v.type == expr::Type::Matrix) {
            EigenResult e = eigen3(v.matrix3());
            Color c = colorOf(o);
            for (const auto& ev : e.values) {
                if (!ev.isReal() || !ev.hasVector) continue;
                dashed(mathToWorld(ev.vector * -len), mathToWorld(ev.vector * len), withAlpha(c, selected ? 255 : 180),
                       0.25f);
                VectorRenderer::drawArrow(Vector3D::zero(), toVec(mathToWorld(ev.vector)), withAlpha(c, 210), 0.015f, 0.15f);
                DrawSphere(mathToWorld(ev.vector * ev.re), selected ? 0.09f : 0.07f, c);
            }
        } else if (dynamic_cast<const scene::PlotObject*>(&o)) {
            auto it = m_plots.find(o.id());
            if (it != m_plots.end() && it->second.renderer.error().empty()) it->second.renderer.draw();
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
        if (!o.visible || !o.value().isNumber()) continue;
        bool selected = isSelected(options, o);
        const expr::Value& v = o.value();
        float re = static_cast<float>(v.re), im = static_cast<float>(v.im);
        Vector3 tip = {re, im, 0};
        Color c = colorOf(o);

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
        if (!o.visible) continue;
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
        if (!o.visible || !o.value().isNumber()) continue;
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
    if (view.showAxes) {
        if (space.kind() == scene::SpaceKind::Euclidean3D) {
            drawWorldLabel(camera, mathToWorld(len, 0, 0), "x", ink, {8.0f, -8.0f}, paper);
            drawWorldLabel(camera, mathToWorld(0, len, 0), "y", ink, {8.0f, -8.0f}, paper);
            drawWorldLabel(camera, mathToWorld(0, 0, len), "z", ink, {8.0f, -8.0f}, paper);
        } else {
            drawWorldLabel(camera, {len, 0, 0}, "Re", ink, {8.0f, -8.0f}, paper);
            drawWorldLabel(camera, {0, len, 0}, "Im", ink, {8.0f, -8.0f}, paper);
        }
    }
    for (const auto& obj : space.objects()) {
        const SceneObject& o = *obj;
        if (!o.visible || !o.value().ok()) continue;
        int style = printStyleOf(o.id());
        Color c = style < 0 ? Color{170, 170, 170, 255} : grayOf(style);
        if (o.typeId() == "eigen") {
            EigenResult e = eigen3(o.value().matrix3());
            for (const auto& ev : e.values) {
                if (!ev.isReal() || !ev.hasVector) continue;
                char text[48];
                snprintf(text, sizeof(text), "λ = %.3g", ev.re);
                drawWorldLabel(camera, mathToWorld(ev.vector * ev.re), text, c, {8.0f, 14.0f}, paper);
            }
            continue;
        }
        Vector3 at;
        if (!anchorPoint(o, space.kind(), options, at)) continue;
        drawWorldLabel(camera, at, o.name().c_str(), c, {8.0f, -8.0f}, paper);
    }
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

    if (space.kind() == scene::SpaceKind::Euclidean3D) {
        if (view.showAxes) {
            drawWorldLabel(camera, mathToWorld(len, 0, 0), "x", t.axisX, kOffset, t.labelBg, t.labelLarge);
            drawWorldLabel(camera, mathToWorld(0, len, 0), "y", t.axisY, kOffset, t.labelBg, t.labelLarge);
            drawWorldLabel(camera, mathToWorld(0, 0, len), "z", t.axisZ, kOffset, t.labelBg, t.labelLarge);
        }
    } else if (view.showAxes) {
        drawWorldLabel(camera, {len, 0, 0}, "Re", t.axisX, kOffset, t.labelBg, t.labelLarge);
        drawWorldLabel(camera, {0, len, 0}, "Im", t.axisY, kOffset, t.labelBg, t.labelLarge);
    }

    for (const auto& obj : space.objects()) {
        const SceneObject& o = *obj;
        if (!o.visible || !o.value().ok()) continue;
        char text[96];
        Vector3 at;
        if (o.typeId() == "eigen") {
            EigenResult e = eigen3(o.value().matrix3());
            for (const auto& ev : e.values) {
                if (!ev.isReal() || !ev.hasVector) continue;
                snprintf(text, sizeof(text), "λ = %.2f", ev.re);
                drawWorldLabel(camera, mathToWorld(ev.vector * ev.re), text, colorOf(o), {8.0f, 14.0f}, t.labelBg, t.labelLarge);
            }
            continue;
        }
        if (!anchorPoint(o, space.kind(), options, at)) continue;
        if (o.typeId() == "vector") {
            snprintf(text, sizeof(text), "%s  %.2f", o.name().c_str(), o.value().vector.magnitude());
        } else {
            snprintf(text, sizeof(text), "%s", o.name().c_str());
        }
        Color c = o.typeId() == "matrix" ? t.text : colorOf(o);
        drawWorldLabel(camera, at, text, c, kOffset, t.labelBg, t.labelLarge);
    }
}

} // namespace linalg::view
