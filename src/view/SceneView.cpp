#include "view/SceneView.hpp"
#include "math/Decompositions.hpp"
#include "rendering/Labels.hpp"
#include "rendering/VectorRenderer.hpp"
#include "rendering/WorldMapping.hpp"
#include "ui/core/Theme.hpp"
#include <raymath.h>
#include <cmath>
#include <cstdio>

namespace linalg::view {

using scene::SceneObject;

namespace {

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
    float radius = selected ? 0.034f : 0.022f;
    float head = selected ? 0.24f : 0.2f;
    if (selected) DrawCylinderEx(from, to, 0.075f, 0.075f, 10, withAlpha(color, 55));
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

Color paletteColor(const std::vector<Color>& palette, int index) {
    return palette[static_cast<size_t>(index) % palette.size()];
}

void basis(const Matrix4D& m, float alpha, bool selected) {
    const ui::Theme& t = ui::theme();
    Vector3D origin = m.transform(Vector3D::zero());
    Vector3 o = mathToWorld(origin);
    Vector3D axes[3] = {m.transformDirection({1, 0, 0}), m.transformDirection({0, 1, 0}), m.transformDirection({0, 0, 1})};
    Color colors[3] = {t.basisX, t.basisY, t.basisZ};
    float radius = selected ? 0.04f : 0.03f;
    for (int i = 0; i < 3; i++) {
        Vector3 tip = mathToWorld(origin + axes[i]);
        Color c = withAlpha(colors[i], static_cast<unsigned char>(255 * alpha));
        if (selected && alpha > 0.9f) DrawCylinderEx(o, tip, 0.08f, 0.08f, 8, withAlpha(colors[i], 50));
        DrawCylinderEx(o, tip, radius, radius, 8, c);
        DrawSphere(tip, selected ? 0.11f : 0.09f, c);
    }
    DrawSphere(o, 0.06f, withAlpha(t.text, static_cast<unsigned char>(255 * alpha)));
}

} // namespace

Color SceneView::colorOf(const SceneObject& o) {
    if (o.customColor) return {o.color.r, o.color.g, o.color.b, 255};
    const ui::Theme& t = ui::theme();
    const std::string& type = o.typeId();
    if (type == "vector") {
        return paletteColor({t.vecA, t.vecB, t.vecSum, t.vecCross, t.vecMA, t.vecMB, t.eigen, t.warning}, o.paletteIndex);
    }
    if (type == "eigen") return t.eigen;
    if (type == "function" || type == "curve" || type == "surface" || type == "implicit") {
        return paletteColor({t.curve, t.success, t.z2, t.warning, t.vecSum}, o.paletteIndex);
    }
    if (type == "complex") return paletteColor({t.z1, t.z2, t.euler, t.helix, t.vecSum, t.success}, o.paletteIndex);
    if (type == "product") return t.zProduct;
    return t.accent;  // matrix, parameter
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
        bool dark = ui::theme().dark;
        if (cache.builtAt == p->changedAt() && cache.builtAt != 0 && cache.dark == dark && cache.color == colorKey) continue;

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
        spec.tMin = static_cast<float>(p->rangeValue("tmin", 0));
        spec.tMax = static_cast<float>(p->rangeValue("tmax", 6.28f));
        scene::PlotObject::Samples samples = p->samples();
        spec.samplesX = samples.nx;
        spec.samplesY = samples.ny;
        spec.heatmap = p->heatmap();
        spec.color = (p->customColor || p->paletteIndex > 0) ? color : Color{0, 0, 0, 0};
        cache.renderer.rebuild(spec, &space);
        cache.builtAt = p->changedAt();
        cache.dark = dark;
        cache.color = colorKey;
    }
}

void SceneView::draw3D(scene::Space& space, const DrawOptions& options) {
    if (space.kind() == scene::SpaceKind::Euclidean3D) drawEuclidean(space, options);
    else drawComplex(space, options);
}

void SceneView::drawEuclidean(scene::Space& space, const DrawOptions& options) {
    const ui::Theme& t = ui::theme();
    const ViewSettings& view = *options.view;
    float len = view.axisLength;

    if (view.showAxes) {
        DrawCylinderEx({0, 0, 0}, mathToWorld(len, 0, 0), 0.02f, 0.02f, 8, t.axisX);
        DrawCylinderEx({0, 0, 0}, mathToWorld(0, len, 0), 0.02f, 0.02f, 8, t.axisY);
        DrawCylinderEx({0, 0, 0}, mathToWorld(0, 0, len), 0.02f, 0.02f, 8, t.axisZ);
    }

    updatePlots(space);

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
    const ui::Theme& t = ui::theme();
    const ViewSettings& view = *options.view;
    float extent = view.axisLength;

    if (view.showAxes) {
        DrawCylinderEx({-extent, 0, 0}, {extent, 0, 0}, 0.015f, 0.015f, 6, t.axisX);
        DrawCylinderEx({0, -extent, 0}, {0, extent, 0}, 0.015f, 0.015f, 6, t.axisY);
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
        DrawLine3D({std::cos(a0), std::sin(a0), 0}, {std::cos(a1), std::sin(a1), 0}, t.textFaint);
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
            dashed(tip, {re, 0, 0}, t.textMuted, 0.08f);
            dashed(tip, {0, im, 0}, t.textMuted, 0.08f);
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

void SceneView::drawLabels(scene::Space& space, const DrawOptions& options, const Camera3D& camera) {
    const ViewSettings& view = *options.view;
    const ui::Theme& t = ui::theme();
    if (!view.showLabels) return;
    float len = view.axisLength;

    if (space.kind() == scene::SpaceKind::Euclidean3D) {
        if (view.showAxes) {
            drawWorldLabel(camera, mathToWorld(len, 0, 0), "x", t.axisX);
            drawWorldLabel(camera, mathToWorld(0, len, 0), "y", t.axisY);
            drawWorldLabel(camera, mathToWorld(0, 0, len), "z", t.axisZ);
        }
    } else if (view.showAxes) {
        drawWorldLabel(camera, {len, 0, 0}, "Re", t.axisX);
        drawWorldLabel(camera, {0, len, 0}, "Im", t.axisY);
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
                drawWorldLabel(camera, mathToWorld(ev.vector * ev.re), text, colorOf(o), {8.0f, 14.0f});
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
        drawWorldLabel(camera, at, text, c);
    }
}

} // namespace linalg::view
