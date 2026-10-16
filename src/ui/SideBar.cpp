#include "ui/SideBar.hpp"
#include "ui/Widgets.hpp"
#include <raygui.h>
#include <algorithm>

namespace linalg {

float SideBar::begin(AppMode& mode) {
    int screenHeight = GetScreenHeight();
    DrawRectangle(0, 0, static_cast<int>(kWidth), screenHeight, Fade(RAYWHITE, 0.88f));

    // Mode selector
    setGuiTextSize(kUiTextSmall + 2);
    int active = static_cast<int>(mode);
    float tabWidth = (kPanelWidth - 2 * 4.0f) / kAppModeCount;
    GuiSetStyle(TOGGLE, GROUP_PADDING, 4);
    GuiToggleGroup({kPanelX, 10, tabWidth, 40}, "Matrices [1];Complex [2];Plots [3]", &active);
    mode = static_cast<AppMode>(active);
    setGuiTextSize(kUiTextLarge);

    m_viewTop = 60.0f;
    float viewHeight = screenHeight - m_viewTop;

    // Scroll with the mouse wheel when the content does not fit
    float maxScroll = std::max(0.0f, m_contentHeight - viewHeight + kGap);
    if (containsMouse() && !GuiIsLocked()) {
        m_scroll -= GetMouseWheelMove() * 40.0f;
    }
    m_scroll = std::clamp(m_scroll, 0.0f, maxScroll);

    BeginScissorMode(0, static_cast<int>(m_viewTop), static_cast<int>(kWidth),
                     static_cast<int>(viewHeight));
    return m_viewTop - m_scroll;
}

void SideBar::end(float contentBottom) {
    EndScissorMode();

    m_contentHeight = contentBottom + m_scroll - m_viewTop;

    float viewHeight = GetScreenHeight() - m_viewTop;
    if (m_contentHeight > viewHeight) {
        float barHeight = viewHeight * viewHeight / m_contentHeight;
        float maxScroll = m_contentHeight - viewHeight + kGap;
        float barY = m_viewTop + (m_scroll / maxScroll) * (viewHeight - barHeight);
        DrawRectangle(static_cast<int>(kWidth) - 6, static_cast<int>(barY), 5,
                      static_cast<int>(barHeight), DARKGRAY);
    }
}

bool SideBar::containsMouse() const {
    return GetMousePosition().x < kWidth;
}

} // namespace linalg
