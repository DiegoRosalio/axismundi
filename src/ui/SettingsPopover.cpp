#include "ui/SettingsPopover.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace linalg {

SettingsActions SettingsPopover::draw(UiSettings& settings) {
    SettingsActions actions;
    if (!m_open) return actions;
    const ui::Theme& t = ui::theme();

    float w = ui::px(300.0f);
    float h = ui::px(330.0f);
    float x = std::min(m_anchor.x + m_anchor.width - w, static_cast<float>(GetScreenWidth()) - w - ui::px(8));
    m_bounds = {std::max(ui::px(8), x), m_anchor.y + m_anchor.height + ui::px(6.0f), w, h};

    // Close when clicking outside (not on the frame that opened it)
    if (!m_justOpened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(GetMousePosition(), m_bounds)) {
        m_open = false;
        return actions;
    }
    m_justOpened = false;

    ui::card(m_bounds);
    ui::Column col(m_bounds.x + ui::px(ui::kPad), m_bounds.y + ui::px(14.0f), m_bounds.width - 2 * ui::px(ui::kPad));
    ui::textIn(col.row(24), "Settings", ui::FontRole::Title, t.text);
    col.space(4.0f);

    ui::textIn(col.row(18), ui::upper("Theme").c_str(), ui::FontRole::Caption, t.textMuted);
    int theme = ui::themeKind() == ui::ThemeKind::Dark ? 0 : 1;
    if (ui::segmented(col.row(32), {"Dark", "Light"}, theme)) {
        ui::setTheme(theme == 0 ? ui::ThemeKind::Dark : ui::ThemeKind::Light);
        actions.themeChanged = true;
    }
    col.space(6.0f);

    ui::textIn(col.row(18), ui::upper("Interface size").c_str(), ui::FontRole::Caption, t.textMuted);
    Rectangle row = col.row(30);
    Rectangle value = ui::takeRight(row, 48.0f, 8.0f);
    float zoom = ui::zoom();
    // Apply the new size when the drag ends, so the slider does not move under the cursor
    if (ui::slider(row, zoom, 0.7f, 2.0f)) m_pendingZoom = std::round(zoom * 20.0f) / 20.0f;
    float shown = m_pendingZoom > 0.0f ? m_pendingZoom : ui::zoom();
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(std::lround(shown * 100.0f)));
    ui::textIn(value, buf, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
    if (m_pendingZoom > 0.0f && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        ui::setZoom(m_pendingZoom);
        m_pendingZoom = 0.0f;
    }
    col.space(6.0f);

    ui::toggleRow(col.row(30), "Explanatory tooltips", settings.tooltips);
    ui::toggleRow(col.row(30), "Status overlay", settings.showHud);
    col.space(4.0f);
    if (ui::button(col.row(ui::kRowHeight), "Reset panel layout", ui::ButtonKind::Secondary, true, ui::Icon::Reset)) {
        actions.resetLayout = true;
    }
    col.space(2.0f);
    std::string where = ui::ellipsize(("Saved in " + Preferences::path()).c_str(), ui::FontRole::Small,
                                      col.width());
    ui::textIn(col.row(18), where.c_str(), ui::FontRole::Small, t.textFaint);
    return actions;
}

} // namespace linalg
