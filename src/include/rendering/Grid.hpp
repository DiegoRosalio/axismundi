#pragma once

// The floor grid of the 3D space (and the plane grid of the complex plane), drawn with GL lines that fade
// toward the edge. Colors come from the view style.

#include <raylib.h>

namespace linalg {

class Grid {
public:
    Grid(int size = 10, float spacing = 1.0f);

    // Lines every unit (stronger every 5) from -size to size, fading toward the edge
    void draw() const;          // ground grid (XZ plane)
    void drawPlaneXY() const;   // vertical grid (XY plane), for 2D views
    void drawAxes(float axisLength = 10.0f) const;

    void setSize(int size) { m_size = size; }
    int size() const { return m_size; }
    void setSpacing(float spacing) { m_spacing = spacing; }
    void setGridColor(Color color) { m_gridColor = color; }  // the view style's grid (alpha 0: theme)
    void setShowGrid(bool show) { m_showGrid = show; }
    void setShowAxes(bool show) { m_showAxes = show; }

private:
    int m_size;
    float m_spacing;
    Color m_gridColor;
    bool m_showGrid;
    bool m_showAxes;
};

} // namespace linalg
