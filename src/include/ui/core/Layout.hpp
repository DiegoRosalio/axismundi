#pragma once

#include "ui/core/Ui.hpp"
#include <initializer_list>
#include <vector>

namespace linalg::ui {

// Vertical layout cursor. Positions are in pixels, sizes passed in design units.
//
//   Column col(area.x, area.y, area.width);
//   Rectangle title = col.row(24);
//   auto cells = splitRow(col.row(32), {1, 1, 1});
class Column {
public:
    Column(float x, float y, float width, float gapDesign = 8.0f)
        : m_x(x), m_y(y), m_width(width), m_gap(px(gapDesign)) {}

    // Next row of the given height (design units), followed by the gap
    Rectangle row(float heightDesign) {
        Rectangle r = {m_x, m_y, m_width, px(heightDesign)};
        m_y += r.height + m_gap;
        return r;
    }

    // Next row with a height already in pixels
    Rectangle rowPx(float heightPx) {
        Rectangle r = {m_x, m_y, m_width, heightPx};
        m_y += heightPx + m_gap;
        return r;
    }

    void space(float design) { m_y += px(design); }
    void removeGap() { m_y -= m_gap; }

    float x() const { return m_x; }
    float y() const { return m_y; }
    float width() const { return m_width; }
    void setY(float y) { m_y = y; }

private:
    float m_x, m_y, m_width, m_gap;
};

// Split a row into cells with relative weights and a gap (design units)
inline std::vector<Rectangle> splitRow(Rectangle r, std::initializer_list<float> weights, float gapDesign = 8.0f) {
    std::vector<Rectangle> cells;
    float total = 0.0f;
    for (float w : weights) total += w;
    float gap = px(gapDesign);
    float available = r.width - gap * (static_cast<float>(weights.size()) - 1.0f);
    float x = r.x;
    for (float w : weights) {
        float cw = available * w / total;
        cells.push_back({x, r.y, cw, r.height});
        x += cw + gap;
    }
    return cells;
}

// Split off a fixed-width cell (design units) from the left or right of a row
inline Rectangle takeLeft(Rectangle& r, float widthDesign, float gapDesign = 8.0f) {
    Rectangle cell = {r.x, r.y, px(widthDesign), r.height};
    float used = cell.width + px(gapDesign);
    r.x += used;
    r.width -= used;
    return cell;
}

inline Rectangle takeRight(Rectangle& r, float widthDesign, float gapDesign = 8.0f) {
    Rectangle cell = {r.x + r.width - px(widthDesign), r.y, px(widthDesign), r.height};
    r.width -= cell.width + px(gapDesign);
    return cell;
}

} // namespace linalg::ui
