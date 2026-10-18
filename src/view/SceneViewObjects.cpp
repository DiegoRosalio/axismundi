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

// --- Derivatives ------------------------------------------------------------------------

namespace {
// A curve on the floor (like y = f(x)) from (x, y) samples; NaN or huge values leave gaps
void floorPolyline(const std::vector<std::pair<double, double>>& pts, Color col, float radius, bool dashedLine) {
    auto at = [](double x, double y) { return mathToWorld(static_cast<float>(x), static_cast<float>(y), 0.006f); };
    for (size_t i = 1; i < pts.size(); i++) {
        const auto &a = pts[i - 1], &b = pts[i];
        if (!std::isfinite(a.second) || !std::isfinite(b.second) || std::fabs(a.second) > 100 || std::fabs(b.second) > 100)
            continue;
        if (dashedLine && (i / 4) % 2 == 1) continue;
        if (radius <= 0) DrawLine3D(at(a.first, a.second), at(b.first, b.second), col);
        else DrawCylinderEx(at(a.first, a.second), at(b.first, b.second), radius, radius, 5, col);
    }
}

// A straight line through (x, y) with slope m on the floor, `half` units long on each side
void floorLine(double x, double y, double m, double half, Color col, float radius) {
    double dx = half / std::sqrt(1 + m * m);
    Vector3 a = mathToWorld(static_cast<float>(x - dx), static_cast<float>(y - m * dx), 0.01f);
    Vector3 b = mathToWorld(static_cast<float>(x + dx), static_cast<float>(y + m * dx), 0.01f);
    if (radius <= 0) DrawLine3D(a, b, col);
    else DrawCylinderEx(a, b, radius, radius, 5, col);
}
}  // namespace

Color SceneView::derivativeColor(const scene::SceneObject& o) {
    return paletteColor(activeStyle().vectors, o.paletteIndex + 1);
}

void SceneView::drawTangent(const scene::TangentObject& o, Color c, bool fade, bool print, Color ink, bool selected) {
    const ViewStyle& st = activeStyle();
    const float k = print ? 0.8f : st.lineScale;
    auto shade = [&](Color col) { return print ? ink : (fade ? faded(col) : col); };
    if (o.showDerivative()) {
        Color d = print ? Color{140, 140, 140, 255} : shade(derivativeColor(o));
        floorPolyline(o.derivative(), d, print ? 0.0f : 0.014f * k, print);
    }
    if (o.showFunction()) floorPolyline(o.function(), shade(c), print ? 0.0f : 0.022f * k, false);
    if (!o.error().empty()) return;
    double x0 = o.x0(), y0 = o.y0(), m = o.slope();
    Color muted = print ? Color{150, 150, 150, 255} : withAlpha(fade ? faded(st.muted) : st.muted, 220);
    if (o.hasSecant()) {
        double h = o.secantH(), y1 = y0 + o.secantSlope() * h;
        floorLine(x0, y0, o.secantSlope(), 3.5, muted, print ? 0.0f : 0.009f * k);
        DrawSphere(mathToWorld(static_cast<float>(x0 + h), static_cast<float>(y1), 0.01f), 0.06f * std::sqrt(k), muted);
    }
    if (o.showTriangle() && std::fabs(m) < 20) {
        // Run 1, rise f'(x0): the slope read off the picture
        Vector3 p0 = mathToWorld(static_cast<float>(x0), static_cast<float>(y0), 0.01f);
        Vector3 p1 = mathToWorld(static_cast<float>(x0 + 1), static_cast<float>(y0), 0.01f);
        Vector3 p2 = mathToWorld(static_cast<float>(x0 + 1), static_cast<float>(y0 + m), 0.01f);
        if (print) {
            DrawLine3D(p0, p1, muted);
            DrawLine3D(p1, p2, muted);
        } else {
            DrawCylinderEx(p0, p1, 0.008f * k, 0.008f * k, 5, muted);
            DrawCylinderEx(p1, p2, 0.008f * k, 0.008f * k, 5, muted);
        }
    }
    Color line = print ? ink : withAlpha(fade ? faded(st.text) : st.text, 230);
    floorLine(x0, y0, m, 3.0, line, print ? 0.0f : 0.012f * k);
    DrawSphere(mathToWorld(static_cast<float>(x0), static_cast<float>(y0), 0.01f), (selected ? 0.11f : 0.08f) * std::sqrt(k),
               shade(c));
}

void SceneView::drawTangentPlane(const scene::TangentPlaneObject& o, Color c, bool fade, bool print, Color ink,
                                 bool selected) {
    const ViewStyle& st = activeStyle();
    const float k = print ? 0.8f : st.lineScale;
    auto shade = [&](Color col) { return print ? ink : (fade ? faded(col) : col); };
    if (o.showSurface()) {
        Color wire = print ? Color{170, 170, 170, 255} : withAlpha(shade(c), 170);
        for (const auto& l : o.surfaceLines())
            for (size_t i = 1; i < l.size(); i++) DrawLine3D(w3(l[i - 1]), w3(l[i]), wire);
    }
    if (!o.error().empty()) return;
    integrals::D3 p = o.point();
    double fx = o.fx(), fy = o.fy();
    auto onPlane = [&](double dx, double dy) { return w3({p.x + dx, p.y + dy, p.z + fx * dx + fy * dy}); };
    if (o.showPlane()) {
        const double s = scene::TangentPlaneObject::kPatch;
        Vector3 a = onPlane(-s, -s), b = onPlane(s, -s), cc = onPlane(s, s), d = onPlane(-s, s);
        if (!print) {
            Color fill = withAlpha(fade ? faded(st.muted) : st.muted, 70);
            // Both windings, so the patch shows from above and below
            DrawTriangle3D(a, b, cc, fill);
            DrawTriangle3D(a, cc, d, fill);
            DrawTriangle3D(a, cc, b, fill);
            DrawTriangle3D(a, d, cc, fill);
        }
        Color edge = print ? ink : withAlpha(fade ? faded(st.text) : st.text, 160);
        DrawLine3D(a, b, edge);
        DrawLine3D(b, cc, edge);
        DrawLine3D(cc, d, edge);
        DrawLine3D(d, a, edge);
    }
    Vector3 pw = w3(p);
    if (o.showSlopes()) {
        // One step in x climbs fx, one step in y climbs fy (the partial derivatives as slopes)
        VectorRenderer::drawArrow(toVec(pw), toVec(onPlane(1, 0)), print ? ink : shade(st.axisX), 0.018f * k, 0.15f);
        VectorRenderer::drawArrow(toVec(pw), toVec(onPlane(0, 1)), print ? ink : shade(st.axisY), 0.018f * k, 0.15f);
    }
    if (o.showNormal()) {
        double len = std::sqrt(fx * fx + fy * fy + 1);
        integrals::D3 n = {p.x - fx / len, p.y - fy / len, p.z + 1 / len};
        VectorRenderer::drawArrow(toVec(pw), toVec(w3(n)), print ? ink : shade(st.axisZ), 0.018f * k, 0.15f);
    }
    if (o.showGradient()) {
        // On the floor under the point: the direction of steepest climb, as long as the climb rate
        Vector3 foot = mathToWorld(static_cast<float>(p.x), static_cast<float>(p.y), 0.0f);
        dashed(foot, pw, print ? Color{150, 150, 150, 255} : withAlpha(fade ? faded(st.muted) : st.muted, 200));
        Vector3 tip = mathToWorld(static_cast<float>(p.x + fx), static_cast<float>(p.y + fy), 0.0f);
        if (std::sqrt(fx * fx + fy * fy) > 1e-6)
            VectorRenderer::drawArrow(toVec(foot), toVec(tip), print ? ink : shade(derivativeColor(o)), 0.026f * k, 0.2f);
    }
    DrawSphere(pw, (selected ? 0.1f : 0.075f) * std::sqrt(k), shade(c));
}

void SceneView::drawVelocity(const scene::VelocityObject& o, Color c, bool fade, bool print, Color ink, bool selected) {
    const ViewStyle& st = activeStyle();
    const float k = print ? 0.8f : st.lineScale;
    auto shade = [&](Color col) { return print ? ink : (fade ? faded(col) : col); };
    if (o.showCurve()) {
        Color curve = print ? Color{120, 120, 120, 255} : withAlpha(fade ? faded(st.muted) : st.muted, 230);
        const auto& pts = o.curve();
        for (size_t i = 1; i < pts.size(); i++) {
            if (print) DrawLine3D(w3(pts[i - 1]), w3(pts[i]), curve);
            else DrawCylinderEx(w3(pts[i - 1]), w3(pts[i]), 0.014f * k, 0.014f * k, 5, curve);
        }
    }
    if (!o.error().empty()) return;
    integrals::D3 p = o.point();
    double s = o.arrowScale();
    auto tipOf = [&](const integrals::D3& v) { return w3(geometry::add(p, geometry::scale(v, s))); };
    Vector3 pw = w3(p);
    Color acc = print ? Color{110, 110, 110, 255} : shade(derivativeColor(o));
    if (o.showAcceleration()) {
        if (o.splitAcceleration()) {
            Color part = print ? Color{150, 150, 150, 255} : withAlpha(acc, 200);
            VectorRenderer::drawArrow(toVec(pw), toVec(tipOf(o.tangential())), part, 0.016f * k, 0.14f);
            VectorRenderer::drawArrow(toVec(pw), toVec(tipOf(o.normal())), part, 0.016f * k, 0.14f);
            // The parallelogram they span ends at a
            Color guide = withAlpha(part, 120);
            dashed(tipOf(o.tangential()), tipOf(o.acceleration()), guide);
            dashed(tipOf(o.normal()), tipOf(o.acceleration()), guide);
        }
        VectorRenderer::drawArrow(toVec(pw), toVec(tipOf(o.acceleration())), acc, 0.022f * k, 0.17f);
    }
    if (o.showVelocity())
        VectorRenderer::drawArrow(toVec(pw), toVec(tipOf(o.velocity())), shade(c), 0.026f * k, 0.2f);
    DrawSphere(pw, (selected ? 0.11f : 0.08f) * std::sqrt(k), shade(c));
}

} // namespace linalg::view
