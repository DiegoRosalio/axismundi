// The complex plane: numbers as arrows (with Euler-style extras), products as a turn plus a stretch,
// Fourier epicycles
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

void drawEpicycles(const scene::EpicyclesObject& o, Color c, bool fade, bool print, Color ink,
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
    dot(tip, (selected ? 0.1f : 0.07f) * std::sqrt(st.lineScale), print ? ink : (fade ? faded(c) : c));
}

// Paper: the circle |z| and the projections (when switched on), then the arrow in the export's style
void printNumber(const SceneObject& o, const DrawContext& c, float re, float im) {
    float r = std::hypot(re, im);
    if (o.propBool("circle") && std::fabs(r - 1.0f) > 1e-3f) {
        for (int i = 0; i < 96; i += 2) {
            float a0 = 2.0f * PI * i / 96, a1 = 2.0f * PI * (i + 1) / 96;
            DrawLine3D({r * std::cos(a0), r * std::sin(a0), 0}, {r * std::cos(a1), r * std::sin(a1), 0}, c.color);
        }
    }
    if (o.propBool("projections")) {
        printLine({re, 0, 0}, {re, im, 0}, c.color, 1, 0.006f);
        printLine({0, im, 0}, {re, im, 0}, c.color, 1, 0.006f);
    }
    printArrow({0, 0, 0}, {re, im, 0}, c.color, dashOf(c.printStyle), c.selected);
}

float printIm(const expr::Value& v) { return static_cast<float>(v.type == expr::Type::Complex ? v.im : 0.0); }

void drawNumber(const SceneObject& o, const DrawContext& c) {
    const expr::Value& v = o.value();
    if (c.print) {
        printNumber(o, c, static_cast<float>(v.re), printIm(v));
        return;
    }
    const ViewStyle& t = activeStyle();
    float re = static_cast<float>(v.re), im = static_cast<float>(v.im);
    Vector3 tip = {re, im, 0};
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
            DrawLine3D({r * std::cos(a0), r * std::sin(a0), 0}, {r * std::cos(a1), r * std::sin(a1), 0},
                       withAlpha(c.color, 120));
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
        dot(top, 0.04f, t.helix);
        dashed(top, tip, t.helix, 0.08f);
        arcXY(0.2f, 0.0f, theta, c.color);
    }
    arrow({0, 0, 0}, tip, c.color, c.selected);
}

void drawProduct(const SceneObject& o, const DrawContext& c) {
    const expr::Value& v = o.value();
    const expr::Value& a = o.field("a")->value;
    const expr::Value& b = o.field("b")->value;
    if (c.print) {
        if (o.propBool("angles", true) && a.isNumber() && b.isNumber()) {
            float argA = std::atan2(printIm(a), static_cast<float>(a.re));
            float argB = std::atan2(printIm(b), static_cast<float>(b.re));
            arcXY(0.35f, 0.0f, argA, Color{110, 110, 110, 255});
            arcXY(0.5f, argA, argA + argB, Color{110, 110, 110, 255});
        }
        printNumber(o, c, static_cast<float>(v.re), printIm(v));
        return;
    }
    const ViewStyle& t = activeStyle();
    Vector3 tip = {static_cast<float>(v.re), static_cast<float>(v.im), 0};
    if (o.propBool("angles", true) && a.isNumber() && b.isNumber()) {
        float ar = static_cast<float>(a.re), ai = printIm(a);
        float br = static_cast<float>(b.re), bi = printIm(b);
        float argA = std::atan2(ai, ar), argB = std::atan2(bi, br);
        float lenB = std::hypot(br, bi), lenA = std::hypot(ar, ai);
        // Rotation step: a turned by arg b; scaling step: stretched by |b|
        Vector3 rotated = {lenA * std::cos(argA + argB), lenA * std::sin(argA + argB), 0};
        if (lenB > 1e-6f) {
            VectorRenderer::drawArrow(Vector3D::zero(), toVec(rotated), withAlpha(c.color, 90), 0.015f, 0.15f);
            dashed(rotated, tip, c.color, 0.08f);
        }
        arcXY(0.35f, 0.0f, argA, paletteColor({t.z1}, 0));
        arcXY(0.5f, argA, argA + argB, t.z2);
    }
    arrow({0, 0, 0}, tip, c.color, c.selected);
}

// Numbers sit at (re, im) on the plane; a click near the arrow from 0 picks them
bool numberPoint(const SceneObject& o, const DrawOptions&, Vector3& out) {
    const expr::Value& v = o.value();
    if (!v.isNumber()) return false;
    out = {static_cast<float>(v.re), static_cast<float>(v.im), 0.0f};
    return true;
}

void pickNumber(const SceneObject& o, const DrawOptions& options, std::vector<Segment>& out) {
    Vector3 p;
    if (numberPoint(o, options, p)) out.push_back({{0, 0, 0}, p});
}

} // namespace

void registerComplexViews(ObjectViewTable& t) {
    ObjectView number;
    number.palette = Palette::Complexes;
    number.draw = drawNumber;
    number.anchor = numberPoint;
    number.pick = pickNumber;
    t["complex"] = number;

    ObjectView product = number;
    product.palette = Palette::Product;
    product.draw = drawProduct;
    t["product"] = product;

    ObjectView epicycles = number;
    epicycles.draw = [](const SceneObject& o, const DrawContext& c) {
        drawEpicycles(static_cast<const scene::EpicyclesObject&>(o), c.color, c.dim, c.print,
                      c.print ? c.color : BLACK, c.print ? false : c.selected);
    };
    t["epicycles"] = epicycles;
}

} // namespace linalg::view
