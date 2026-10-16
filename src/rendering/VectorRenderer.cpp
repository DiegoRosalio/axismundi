#include "rendering/VectorRenderer.hpp"
#include <raylib.h>
#include <raymath.h>
#include <cmath>

namespace linalg {

VectorRenderer::VectorRenderer() : m_arrowHeadSize(0.2f) {}

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

        // TODO: Draw 3D labels (requires camera context)
    }
}

void VectorRenderer::drawArrow(const Vector3D& start, const Vector3D& end, Color color) const {
    // Draw the main line
    Vector3 rlStart = {start.x, start.y, start.z};
    Vector3 rlEnd = {end.x, end.y, end.z};

    DrawCylinderEx(rlStart, rlEnd, 0.02f, 0.02f, 8, color);

    // Calculate arrow head
    Vector3D direction = end - start;
    float length = direction.magnitude();

    if (length < 0.001f) return; // Too short to draw arrow head

    Vector3D normalized = direction.normalized();

    // Arrow head position
    Vector3D headBase = end - normalized * m_arrowHeadSize;

    // Draw cone for arrow head
    Vector3 coneBase = {headBase.x, headBase.y, headBase.z};
    Vector3 coneTip = {end.x, end.y, end.z};

    // Draw sphere at tip for better visibility
    DrawSphere(coneTip, m_arrowHeadSize * 0.3f, color);

    // Draw cone body
    DrawCylinderEx(coneBase, coneTip, m_arrowHeadSize * 0.5f, 0.0f, 8, color);
}

} // namespace linalg
