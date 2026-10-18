// Drawing of the richer object types (quaternions, integrals, frames, Fourier); the basic ones are
// in SceneView.cpp
#include "view/SceneView.hpp"
#include "math/Decompositions.hpp"
#include "rendering/VectorRenderer.hpp"
#include "rendering/WorldMapping.hpp"
#include "ui/core/Theme.hpp"
#include "view/SceneViewDraw.hpp"
#include "view/ViewStyle.hpp"
#include <raymath.h>
#include <algorithm>
#include <cmath>

namespace linalg::view {

using scene::SceneObject;
using namespace draw;

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

void SceneView::drawFrame(const scene::FrameObject& o, Color c, bool fade, bool print, Color ink, bool selected) {
    const ViewStyle& st = activeStyle();
    float k = print ? 0.8f : st.lineScale;
    const auto& curve = o.curve();
    Color curveColor = print ? ink : (fade ? faded(c) : c);
    if (o.showCurve()) {
        for (size_t i = 1; i < curve.size(); i++) {
            if (print) DrawLine3D(w3(curve[i - 1]), w3(curve[i]), curveColor);
            else DrawCylinderEx(w3(curve[i - 1]), w3(curve[i]), 0.016f * k, 0.016f * k, 5, curveColor);
        }
    }
    if (o.showComb()) {
        Color comb = print ? Color{150, 150, 150, 255} : withAlpha(fade ? faded(st.muted) : st.muted, 200);
        const auto& teeth = o.comb();
        for (size_t i = 0; i < teeth.size(); i++) {
            DrawLine3D(w3(teeth[i].first), w3(teeth[i].second), comb);
            if (i > 0) DrawLine3D(w3(teeth[i - 1].second), w3(teeth[i].second), comb);
        }
    }
    if (!o.error().empty()) return;
    const geometry::Frame& f = o.frame();
    Vector3 p = w3(f.p);
    if (o.showCircle() && f.hasNormal && f.kappa > 1e-3 && 1.0 / f.kappa < 60.0) {
        // The osculating circle: radius 1/κ, centered on the normal line, in the T–N plane
        double radius = 1.0 / f.kappa;
        geometry::D3 center = geometry::add(f.p, geometry::scale(f.N, radius));
        Color circle = print ? Color{120, 120, 120, 255} : withAlpha(fade ? faded(st.muted) : st.muted, 220);
        const int n = 96;
        for (int i = 0; i < n; i++) {
            double a0 = 2 * PI * i / n, a1 = 2 * PI * (i + 1) / n;
            auto at = [&](double a) {
                return w3(geometry::add(center, geometry::add(geometry::scale(f.N, -radius * std::cos(a)),
                                                              geometry::scale(f.T, radius * std::sin(a)))));
            };
            DrawLine3D(at(a0), at(a1), circle);
        }
        DrawSphere(w3(center), 0.04f, circle);
        DrawLine3D(p, w3(center), withAlpha(circle, 120));
    }
    if (o.showFrame()) {
        // T, N, B in the axis colors (like x, y, z): a right-handed frame riding the curve
        const geometry::D3 dirs[3] = {f.T, f.N, f.B};
        const Color colors[3] = {st.axisX, st.axisY, st.axisZ};
        for (int i = 0; i < 3; i++) {
            if (i > 0 && !f.hasNormal) continue;
            Color col = print ? ink : (fade ? faded(colors[i]) : colors[i]);
            geometry::D3 tip = geometry::add(f.p, dirs[i]);
            VectorRenderer::drawArrow(toVec(p), toVec(w3(tip)), col, 0.022f * k, 0.17f);
        }
    }
    DrawSphere(p, (selected ? 0.11f : 0.08f) * std::sqrt(k), print ? ink : (fade ? faded(c) : c));
}

void SceneView::drawEpicycles(const scene::EpicyclesObject& o, Color c, bool fade, bool print, Color ink,
                              bool selected) {
    const ViewStyle& st = activeStyle();
    auto at = [](const fourier::cplx& z) { return Vector3{static_cast<float>(z.real()), static_cast<float>(z.imag()), 0}; };
    // The target curve, faint
    Color faint = print ? Color{200, 200, 200, 255} : withAlpha(fade ? faded(st.muted) : st.muted, 110);
    const auto& target = o.target();
    for (size_t i = 0; i < target.size(); i++) DrawLine3D(at(target[i]), at(target[(i + 1) % target.size()]), faint);
    if (!o.error().empty() || o.terms().empty()) return;
    int count = o.circles() + 1;  // with the center term
    double u = o.angle();
    // The traced path: the last lap up to the tip (a function of the time, so scrubbing works)
    Color path = print ? ink : (fade ? faded(c) : c);
    double start = std::max(0.0, u - 2 * PI);
    int steps = std::max(2, static_cast<int>((u - start) / 0.01));
    fourier::cplx prev = fourier::evaluate(o.terms(), start, count);
    for (int i = 1; i <= steps; i++) {
        fourier::cplx next = fourier::evaluate(o.terms(), start + (u - start) * i / steps, count);
        if (print) DrawLine3D(at(prev), at(next), path);
        else DrawCylinderEx(at(prev), at(next), 0.02f * st.lineScale, 0.02f * st.lineScale, 5, path);
        prev = next;
    }
    // The circles and their radius arms, largest first
    auto centers = fourier::chain(o.terms(), u, count);
    Color ring = print ? Color{150, 150, 150, 255} : withAlpha(fade ? faded(st.text) : st.text, 70);
    Color arm = print ? Color{90, 90, 90, 255} : withAlpha(fade ? faded(st.text) : st.text, 200);
    for (size_t i = 1; i < centers.size(); i++) {
        double r = std::abs(o.terms()[i].c);
        Vector3 center = at(centers[i - 1]);
        if (r > 0.02) {
            const int seg = 64;
            for (int k = 0; k < seg; k++) {
                float a0 = 2.0f * PI * k / seg, a1 = 2.0f * PI * (k + 1) / seg;
                DrawLine3D({center.x + static_cast<float>(r) * std::cos(a0), center.y + static_cast<float>(r) * std::sin(a0), 0},
                           {center.x + static_cast<float>(r) * std::cos(a1), center.y + static_cast<float>(r) * std::sin(a1), 0},
                           ring);
            }
        }
        DrawLine3D(center, at(centers[i]), arm);
    }
    Vector3 tip = at(centers.back());
    DrawSphere(tip, (selected ? 0.1f : 0.07f) * std::sqrt(st.lineScale), print ? ink : (fade ? faded(c) : c));
}

void SceneView::drawFourierSeries(const scene::FourierSeriesObject& o, Color c, bool fade, bool print, Color ink) {
    const ViewStyle& st = activeStyle();
    // On the floor like y = f(x), a hair above it
    auto at = [](double x, double y) { return mathToWorld(static_cast<float>(x), static_cast<float>(y), 0.006f); };
    auto polyline = [&](const std::vector<std::pair<double, double>>& pts, Color col, float radius) {
        for (size_t i = 1; i < pts.size(); i++) {
            const auto &a = pts[i - 1], &b = pts[i];
            if (!std::isfinite(a.second) || !std::isfinite(b.second) || std::fabs(a.second) > 100 || std::fabs(b.second) > 100)
                continue;
            if (radius <= 0) DrawLine3D(at(a.first, a.second), at(b.first, b.second), col);
            else DrawCylinderEx(at(a.first, a.second), at(b.first, b.second), radius, radius, 5, col);
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

} // namespace linalg::view
