#pragma once

#include <raylib.h>

namespace linalg {

// Left column below the top bar: a themed background with a scrollable area.
//
//   Rectangle area = sideBar.begin();          // area.y already includes scrolling
//   float y = somePanel.draw(area.x, area.y, area.width);
//   sideBar.end(y);
class SideBar {
public:
    static float width();   // pixels
    static float top();     // pixels (bottom of the top bar)

    Rectangle begin();
    void end(float contentBottom);

    bool containsMouse() const;

private:
    float m_scroll = 0.0f;
    float m_contentHeight = 0.0f;
};

} // namespace linalg
