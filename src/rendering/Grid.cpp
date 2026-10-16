#include "rendering/Grid.hpp"
#include <raylib.h>
#include <raymath.h>
#include "ui/core/Theme.hpp"

namespace linalg {

Grid::Grid(int size, float spacing)
    : m_size(size), m_spacing(spacing),
      m_gridColor({200, 200, 200, 100}),
      m_showGrid(true), m_showAxes(true) {}

void Grid::draw() const {
    if (!m_showGrid) return;
    Color color = ui::theme().grid;

    float extent = m_size * m_spacing;

    // Draw grid lines parallel to X axis
    for (int i = -m_size; i <= m_size; i++) {
        float z = i * m_spacing;
        DrawLine3D(
            {-extent, 0, z},
            {extent, 0, z},
            color
        );
    }

    // Draw grid lines parallel to Z axis
    for (int i = -m_size; i <= m_size; i++) {
        float x = i * m_spacing;
        DrawLine3D(
            {x, 0, -extent},
            {x, 0, extent},
            color
        );
    }
}

void Grid::drawPlaneXY() const {
    if (!m_showGrid) return;
    Color color = ui::theme().grid;

    float extent = m_size * m_spacing;
    for (int i = -m_size; i <= m_size; i++) {
        float t = i * m_spacing;
        DrawLine3D({-extent, t, 0}, {extent, t, 0}, color);
        DrawLine3D({t, -extent, 0}, {t, extent, 0}, color);
    }
}

void Grid::drawAxes(float axisLength) const {
    if (!m_showAxes) return;

    const ui::Theme& t = ui::theme();
    DrawCylinderEx({0, 0, 0}, {axisLength, 0, 0}, 0.02f, 0.02f, 8, t.axisX);
    DrawCylinderEx({0, 0, 0}, {0, axisLength, 0}, 0.02f, 0.02f, 8, t.axisY);
    DrawCylinderEx({0, 0, 0}, {0, 0, axisLength}, 0.02f, 0.02f, 8, t.axisZ);
    DrawSphere({axisLength, 0, 0}, 0.08f, t.axisX);
    DrawSphere({0, axisLength, 0}, 0.08f, t.axisY);
    DrawSphere({0, 0, axisLength}, 0.08f, t.axisZ);
}

} // namespace linalg
