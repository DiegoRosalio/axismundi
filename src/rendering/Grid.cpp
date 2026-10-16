#include "rendering/Grid.hpp"
#include <raylib.h>
#include <raymath.h>

namespace linalg {

Grid::Grid(int size, float spacing)
    : m_size(size), m_spacing(spacing),
      m_gridColor({200, 200, 200, 100}),
      m_showGrid(true), m_showAxes(true) {}

void Grid::draw() const {
    if (!m_showGrid) return;

    float extent = m_size * m_spacing;

    // Draw grid lines parallel to X axis
    for (int i = -m_size; i <= m_size; i++) {
        float z = i * m_spacing;
        DrawLine3D(
            {-extent, 0, z},
            {extent, 0, z},
            m_gridColor
        );
    }

    // Draw grid lines parallel to Z axis
    for (int i = -m_size; i <= m_size; i++) {
        float x = i * m_spacing;
        DrawLine3D(
            {x, 0, -extent},
            {x, 0, extent},
            m_gridColor
        );
    }
}

void Grid::drawAxes(float axisLength) const {
    if (!m_showAxes) return;

    // X axis - Red
    DrawLine3D({0, 0, 0}, {axisLength, 0, 0}, RED);
    DrawSphere({axisLength, 0, 0}, 0.1f, RED);

    // Y axis - Green
    DrawLine3D({0, 0, 0}, {0, axisLength, 0}, GREEN);
    DrawSphere({0, axisLength, 0}, 0.1f, GREEN);

    // Z axis - Blue
    DrawLine3D({0, 0, 0}, {0, 0, axisLength}, BLUE);
    DrawSphere({0, 0, axisLength}, 0.1f, BLUE);

    // Draw thicker lines for better visibility
    DrawCylinderEx({0, 0, 0}, {axisLength, 0, 0}, 0.02f, 0.02f, 8, RED);
    DrawCylinderEx({0, 0, 0}, {0, axisLength, 0}, 0.02f, 0.02f, 8, GREEN);
    DrawCylinderEx({0, 0, 0}, {0, 0, axisLength}, 0.02f, 0.02f, 8, BLUE);
}

} // namespace linalg
