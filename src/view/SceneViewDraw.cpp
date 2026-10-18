// Small drawing helpers shared by SceneView.cpp and SceneViewObjects.cpp
#include "view/SceneViewDraw.hpp"
#include "math/Decompositions.hpp"
#include "rendering/VectorRenderer.hpp"
#include "rendering/WorldMapping.hpp"
#include "ui/core/Theme.hpp"
#include "view/ViewStyle.hpp"
#include <raymath.h>
#include <algorithm>
#include <cmath>

namespace linalg::view::draw {

using scene::SceneObject;

Vector3D toVec(Vector3 v) { return {v.x, v.y, v.z}; }

Vector3 w3(const integrals::D3& p) {
    return mathToWorld(p);
}


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

void dashed(Vector3 a, Vector3 b, Color color, float dash) {
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

Color derivativeColor(const scene::SceneObject& o) {
    return paletteColor(activeStyle().vectors, o.paletteIndex + 1);
}

void basis(const Matrix4D& m, float alpha, bool selected, bool fade) {
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



Color mixColor(Color a, Color b, float k) {
    k = std::clamp(k, 0.0f, 1.0f);
    auto m = [&](unsigned char x, unsigned char y) { return static_cast<unsigned char>(x + (y - x) * k); };
    return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a)};
}


} // namespace linalg::view::draw
