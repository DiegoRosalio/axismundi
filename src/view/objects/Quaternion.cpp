// Quaternions in the 3D view: the rotation axis and angle, the rotated basis, a vector before and after
#include "view/ObjectViews.hpp"
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

void drawQuaternion(const scene::QuaternionObject& o, bool selected, Color c, bool fade, bool print,
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
            VectorRenderer::drawVectorArrow({0, 0, 0}, mathToWorld(a * kQuatAxisLength), c, selected, k);
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

void drawObject(const SceneObject& o, const DrawContext& c) {
    if (o.value().type != expr::Type::Quaternion) return;
    drawQuaternion(static_cast<const scene::QuaternionObject&>(o), c.selected, c.color, c.dim, c.print, c.printStyle);
}

// The handle is the axis tip, so only while the axis is drawn
bool axisTip(const SceneObject& o, Vector3& out) {
    expr::Rotation r;
    if (!static_cast<const scene::QuaternionObject&>(o).showAxis() || !expr::rotationOf(o.value(), r) || !r.hasAxis)
        return false;
    out = mathToWorld(r.axis * kQuatAxisLength);
    return true;
}

} // namespace

void registerQuaternionViews(ObjectViewTable& t) {
    ObjectView v;
    v.palette = Palette::Vectors;
    v.draw = drawObject;
    v.anchor = [](const SceneObject& o, const DrawOptions&, Vector3& out) { return axisTip(o, out); };
    v.label = [](const SceneObject& o) -> std::string {
        expr::Rotation r;
        expr::rotationOf(o.value(), r);
        auto c3 = [](float x) { return std::fabs(x) < 5e-3f ? 0.0f : x; };
        char text[96];
        if (r.hasAxis)
            snprintf(text, sizeof(text), "%s  %.0f° about (%.2g, %.2g, %.2g)", o.name().c_str(),
                     r.angle * 180.0 / 3.14159265358979, c3(r.axis.x), c3(r.axis.y), c3(r.axis.z));
        else
            snprintf(text, sizeof(text), "%s  no rotation", o.name().c_str());
        return text;
    };
    v.pick = [](const SceneObject& o, const DrawOptions&, std::vector<Segment>& out) {
        Vector3 tip;
        if (axisTip(o, tip)) out.push_back({{0, 0, 0}, tip});
    };
    t["quaternion"] = v;
}

} // namespace linalg::view
