#include "rendering/VectorRenderer.hpp"
#include <raylib.h>
#include <raymath.h>
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
        snprintf(text, sizeof(text), "%s  |%.2f|", vec.label.c_str(), vec.direction.magnitude());

        // Darken the vector color a little so light colors stay readable
        Color c = ColorBrightness(vec.color, -0.35f);
        drawWorldLabel(camera, {end.x, end.y, end.z}, text, 22, c);
    }
}

void VectorRenderer::drawArrow(const Vector3D& start, const Vector3D& end, Color color,
                               float radius, float headSize) {
    // Draw the main line
    Vector3 rlStart = {start.x, start.y, start.z};
    Vector3 rlEnd = {end.x, end.y, end.z};

    DrawCylinderEx(rlStart, rlEnd, radius, radius, 8, color);

    // Calculate arrow head
    Vector3D direction = end - start;
    float length = direction.magnitude();

    if (length < 0.001f) return; // Too short to draw arrow head

    Vector3D normalized = direction.normalized();

    // Arrow head position
    Vector3D headBase = end - normalized * headSize;

    // Draw cone for arrow head
    Vector3 coneBase = {headBase.x, headBase.y, headBase.z};
    Vector3 coneTip = {end.x, end.y, end.z};

    // Draw sphere at tip for better visibility
    DrawSphere(coneTip, headSize * 0.3f, color);

    // Draw cone body
    DrawCylinderEx(coneBase, coneTip, headSize * 0.5f, 0.0f, 8, color);
}

} // namespace linalg
