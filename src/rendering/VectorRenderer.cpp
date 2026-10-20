#include "rendering/VectorRenderer.hpp"
#include <raylib.h>
#include <raymath.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "rendering/Labels.hpp"

namespace linalg {

VectorRenderer::VectorRenderer() {}

void VectorRenderer::addVector(const VectorObject& vec) {
    m_vectors.push_back(vec);
}

void VectorRenderer::clearVectors() {
    m_vectors.clear();
}

void VectorRenderer::draw() const {
    for (const auto& vec : m_vectors) {
        Vector3D end = vec.position + vec.direction;
        drawArrow(vec.position, end, vec.color);
    }
}

void VectorRenderer::drawLabels(const Camera3D& camera) const {
    for (const auto& vec : m_vectors) {
        if (!vec.showLabel || vec.label.empty()) continue;

        Vector3D end = vec.position + vec.direction;
        char text[96];
        snprintf(text, sizeof(text), "%s  %.2f", vec.label.c_str(), vec.direction.magnitude());
        drawWorldLabel(camera, {end.x, end.y, end.z}, text, vec.color);
    }
}

namespace {
Vector3 g_camera{0, 0, 0};
float g_worldPerPixel = 0.0f;  // 0: not set (fixed sizes)
}  // namespace

void VectorRenderer::setScreenScale(Vector3 cameraPosition, float worldPerPixelAtOne) {
    g_camera = cameraPosition;
    g_worldPerPixel = worldPerPixelAtOne;
}

float VectorRenderer::pixelsAt(Vector3 point, float pixels) {
    if (g_worldPerPixel <= 0.0f) return pixels * 0.011f;  // about right at the default camera distance
    return pixels * g_worldPerPixel * std::max(0.05f, Vector3Distance(g_camera, point));
}

void VectorRenderer::drawArrow(const Vector3D& start, const Vector3D& end, Color color, float radius, float headSize) {
    Vector3 a = {start.x, start.y, start.z}, b = {end.x, end.y, end.z};
    Vector3D direction = end - start;
    float length = direction.magnitude();
    if (length < 1e-4f) return;
    Vector3 mid = Vector3Lerp(a, b, 0.5f);
    float r = std::clamp(radius, pixelsAt(mid, 0.5f), pixelsAt(mid, 1.75f));
    float head = std::min({headSize, 0.35f * length, pixelsAt(b, 18.0f)});
    float headR = std::max(head * 0.45f, r * 1.8f);
    Vector3 neck = Vector3Lerp(a, b, (length - head) / length);
    DrawCylinderEx(a, neck, r, r, 8, color);
    DrawCylinderEx(neck, b, headR, 0.0f, 10, color);
}

void VectorRenderer::drawVectorArrow(Vector3 from, Vector3 to, Color color, bool selected, float lineScale) {
    float length = Vector3Distance(from, to);
    if (length < 1e-4f) return;
    Vector3 mid = Vector3Lerp(from, to, 0.5f);
    float k = std::sqrt(std::max(0.25f, lineScale));
    float shaft = pixelsAt(mid, (selected ? 1.4f : 1.0f) * k);
    float head = std::min(0.3f * length, pixelsAt(to, 16.0f * k));
    float headR = std::max(0.38f * head, 2.2f * shaft);
    Vector3 neck = Vector3Lerp(from, to, (length - head) / length);
    if (selected) {
        // A narrow, faint outline around the shaft and the head
        Color glow = {color.r, color.g, color.b, 70};
        float pad = pixelsAt(mid, 1.6f * k);
        DrawCylinderEx(from, neck, shaft + pad, shaft + pad, 10, glow);
        DrawCylinderEx(neck, to, headR + pad, pad * 0.5f, 12, glow);
    }
    DrawCylinderEx(from, neck, shaft, shaft, 8, color);
    DrawCylinderEx(neck, to, headR, 0.0f, 10, color);
}

} // namespace linalg
