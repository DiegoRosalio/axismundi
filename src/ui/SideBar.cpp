#include "ui/SideBar.hpp"
#include "ui/TopBar.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Theme.hpp"
#include <raygui.h>
#include <algorithm>

namespace linalg {

float SideBar::width() { return ui::px(344.0f); }
float SideBar::top() { return TopBar::height(); }

Rectangle SideBar::begin() {
    const ui::Theme& t = ui::theme();
    float screenH = static_cast<float>(GetScreenHeight());
    float viewTop = top();
    float viewHeight = screenH - viewTop;

    Color bg = t.panel;
    bg.a = 255;  // opaque: 3D content must not show through
    DrawRectangleRec({0, viewTop, width(), viewHeight}, bg);
    DrawRectangleRec({width() - ui::pxi(1.0f), viewTop, ui::pxi(1.0f), viewHeight}, t.border);

    // Scroll with the mouse wheel when the content does not fit
    float maxScroll = std::max(0.0f, m_contentHeight - viewHeight);
    if (containsMouse() && ui::canInteract()) m_scroll -= GetMouseWheelMove() * ui::px(48.0f);
    m_scroll = std::clamp(m_scroll, 0.0f, maxScroll);

    BeginScissorMode(0, static_cast<int>(viewTop), static_cast<int>(width()), static_cast<int>(viewHeight));
    ui::setClip({0, viewTop, width(), viewHeight});
    float pad = ui::px(ui::kPad);
    return {pad, viewTop + pad - m_scroll, width() - 2 * pad, viewHeight};
}

void SideBar::end(float contentBottom) {
    EndScissorMode();
    ui::clearClip();

    float viewTop = top();
    float viewHeight = GetScreenHeight() - viewTop;
    m_contentHeight = contentBottom + m_scroll - viewTop + ui::px(ui::kPad);

    if (m_contentHeight > viewHeight) {
        float barHeight = std::max(ui::px(24.0f), viewHeight * viewHeight / m_contentHeight);
        float maxScroll = m_contentHeight - viewHeight;
        float barY = viewTop + (m_scroll / maxScroll) * (viewHeight - barHeight);
        float w = ui::px(4.0f);
        ui::fillRounded({width() - w - ui::px(3.0f), barY, w, barHeight}, w / 2, ui::theme().borderStrong);
    }
}

bool SideBar::containsMouse() const {
    Vector2 m = GetMousePosition();
    return m.x < width() && m.y > top();
}

} // namespace linalg
