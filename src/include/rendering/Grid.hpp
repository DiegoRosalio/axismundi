#pragma once

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
    void setGridColor(Color color) { m_gridColor = color; }
    void setShowGrid(bool show) { m_showGrid = show; }
    void setShowAxes(bool show) { m_showAxes = show; }
    void setPaper(bool paper) { m_paper = paper; }  // light gray lines for the white print preview

private:
    int m_size;
    float m_spacing;
    Color m_gridColor;
    bool m_showGrid;
    bool m_showAxes;
    bool m_paper = false;
};

} // namespace linalg
