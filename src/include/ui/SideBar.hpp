#pragma once

#include "ui/AppMode.hpp"

namespace linalg {

// Left column: a fixed mode selector on top and a scrollable area below it.
//
//   float y = sideBar.begin(mode);
//   y = somePanel.draw(SideBar::kPanelX, y, SideBar::kPanelWidth) + SideBar::kGap;
//   sideBar.end(y);
class SideBar {
public:
    static constexpr float kWidth = 470.0f;
    static constexpr float kPanelX = 10.0f;
    static constexpr float kPanelWidth = 450.0f;
    static constexpr float kGap = 10.0f;

    // Draws the background and mode selector, starts clipping, and returns
    // the (scrolled) y where content should begin.
    float begin(AppMode& mode);

    // Ends clipping; `contentBottom` is the y returned by the last panel.
    void end(float contentBottom);

    bool containsMouse() const;

private:
    float m_scroll = 0.0f;
    float m_contentHeight = 0.0f;
    float m_viewTop = 0.0f;
};

} // namespace linalg
