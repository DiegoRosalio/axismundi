#include "rendering/ArgandRenderer.hpp"
#include "rendering/Labels.hpp"
#include "rendering/VectorRenderer.hpp"
#include "ui/core/Theme.hpp"
#include <raymath.h>
#include <cmath>
#include <cstdio>

namespace linalg {

namespace {

constexpr float kHelixPitch = 0.3f;  // Z distance per radian of the Euler helix

Vector3 toWorld(const Complex& z, float depth = 0.0f) {
    return {z.re, z.im, depth};
}

Vector3D toVec(const Complex& z) {
    return {z.re, z.im, 0.0f};
}

void drawArc(float radius, float from, float to, Color color) {
    int segments = static_cast<int>(std::fabs(to - from) / 0.05f) + 2;
    for (int i = 0; i < segments; i++) {
        float a0 = from + (to - from) * i / segments;
        float a1 = from + (to - from) * (i + 1) / segments;
        DrawCylinderEx({radius * std::cos(a0), radius * std::sin(a0), 0},
                       {radius * std::cos(a1), radius * std::sin(a1), 0}, 0.012f, 0.012f, 4, color);
    }
}

void drawDashed(Vector3 a, Vector3 b, Color color) {
    float len = Vector3Distance(a, b);
    const float dash = 0.08f;
    for (float s = 0.0f; s < len; s += 2.0f * dash) {
        float e = std::fmin(s + dash, len);
        DrawLine3D(Vector3Lerp(a, b, s / len), Vector3Lerp(a, b, e / len), color);
    }
}

} // namespace

void ArgandRenderer::draw(const ComplexScene& scene, float extent, bool showAxes) const {
    const ui::Theme& th = ui::theme();
    // Axes with integer ticks
    if (showAxes) {
        DrawCylinderEx({-extent, 0, 0}, {extent, 0, 0}, 0.015f, 0.015f, 6, th.axisX);
        DrawCylinderEx({0, -extent, 0}, {0, extent, 0}, 0.015f, 0.015f, 6, th.axisY);
        for (int i = -static_cast<int>(extent); i <= static_cast<int>(extent); i++) {
            if (i == 0) continue;
            float t = static_cast<float>(i);
            DrawLine3D({t, -0.08f, 0}, {t, 0.08f, 0}, th.axisX);
            DrawLine3D({-0.08f, t, 0}, {0.08f, t, 0}, th.axisY);
        }
    }

    // Unit circle
    const int segments = 96;
    for (int i = 0; i < segments; i++) {
        float a0 = 2.0f * PI * i / segments;
        float a1 = 2.0f * PI * (i + 1) / segments;
        DrawLine3D({std::cos(a0), std::sin(a0), 0}, {std::cos(a1), std::sin(a1), 0}, th.textFaint);
    }

    // z1, z2 and their product
    VectorRenderer::drawArrow(Vector3D::zero(), toVec(scene.z1), th.z1, 0.025f, 0.2f);
    VectorRenderer::drawArrow(Vector3D::zero(), toVec(scene.z2), th.z2, 0.025f, 0.2f);

    if (scene.showProduct) {
        Complex product = scene.z1 * scene.z2;
        float a1 = scene.z1.arg();
        float a2 = scene.z2.arg();

        // Rotation step: z1 turned by arg(z2) but not yet scaled
        float r2 = scene.z2.abs();
        if (r2 > 1e-6f) {
            Complex rotated = scene.z1 * (scene.z2 * (1.0f / r2));
            VectorRenderer::drawArrow(Vector3D::zero(), toVec(rotated), Fade(th.zProduct, 0.35f), 0.015f, 0.15f);
            // Scaling step: along the same direction, from |z1| to |z1||z2|
            drawDashed(toWorld(rotated), toWorld(product), th.zProduct);
        }
        VectorRenderer::drawArrow(Vector3D::zero(), toVec(product), th.zProduct, 0.03f, 0.22f);

        // Angles add: arc for arg z1, then arg z2 stacked on top of it
        drawArc(0.35f, 0.0f, a1, th.z1);
        drawArc(0.5f, a1, a1 + a2, th.z2);
    }

    if (scene.showEuler) {
        float c = std::cos(scene.theta);
        float s = std::sin(scene.theta);
        Vector3 point = {c, s, 0};

        // Radius, projections and the cos / sin segments on the axes
        DrawCylinderEx({0, 0, 0}, point, 0.015f, 0.015f, 6, th.euler);
        drawDashed(point, {c, 0, 0}, th.textMuted);
        drawDashed(point, {0, s, 0}, th.textMuted);
        DrawCylinderEx({0, 0, 0}, {c, 0, 0}, 0.035f, 0.035f, 6, th.axisX);
        DrawCylinderEx({0, 0, 0}, {0, s, 0}, 0.035f, 0.035f, 6, th.axisY);
        DrawSphere(point, 0.07f, th.euler);
        drawArc(0.2f, 0.0f, scene.theta, th.euler);

        // Helix e^(it), t in [0, θ], rising along Z: its shadow is the circle
        const float step = 0.05f;
        Vector3 prev = {1, 0, 0};
        for (float t = step; t <= scene.theta + 1e-4f; t += step) {
            Vector3 next = {std::cos(t), std::sin(t), t * kHelixPitch};
            DrawLine3D(prev, next, th.helix);
            prev = next;
        }
        Vector3 tip = {c, s, scene.theta * kHelixPitch};
        DrawSphere(tip, 0.04f, th.helix);
        drawDashed(tip, point, th.helix);
    }
}

void ArgandRenderer::drawLabels(const Camera3D& camera, const ComplexScene& scene, float extent,
                                bool showAxes) const {
    const ui::Theme& th = ui::theme();
    if (showAxes) {
        drawWorldLabel(camera, {extent, 0, 0}, "Re", th.axisX);
        drawWorldLabel(camera, {0, extent, 0}, "Im", th.axisY);
        drawWorldLabel(camera, {1, 0, 0}, "1", th.textMuted, {4.0f, 14.0f});
        drawWorldLabel(camera, {0, 1, 0}, "i", th.textMuted, {-18.0f, -10.0f});
    }

    drawWorldLabel(camera, toWorld(scene.z1), "z₁", th.z1);
    drawWorldLabel(camera, toWorld(scene.z2), "z₂", th.z2);
    if (scene.showProduct) {
        drawWorldLabel(camera, toWorld(scene.z1 * scene.z2), "z₁z₂", th.zProduct);
    }

    if (scene.showEuler) {
        float c = std::cos(scene.theta);
        float s = std::sin(scene.theta);
        char text[48];
        snprintf(text, sizeof(text), "e^(i·%.2f)", scene.theta);
        drawWorldLabel(camera, {c, s, 0}, text, th.euler);
        drawWorldLabel(camera, {c, 0, 0}, "cos θ", th.axisX, {-14.0f, 16.0f});
        drawWorldLabel(camera, {0, s, 0}, "sin θ", th.axisY, {-52.0f, 0.0f});
    }
}

} // namespace linalg
