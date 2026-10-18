#include "ui/ColorPopover.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <raygui.h>
#include <algorithm>
#include <cstdio>

namespace linalg {

namespace {

// Presets chosen to read well on both dark and light backgrounds
const Color kPresets[] = {
    {236, 72, 72, 255},   // red
    {245, 140, 40, 255},  // orange
    {230, 190, 20, 255},  // yellow
    {60, 190, 100, 255},  // green
    {30, 180, 200, 255},  // teal
    {70, 130, 245, 255},  // blue
    {150, 100, 245, 255}, // violet
    {225, 80, 180, 255},  // pink
};

} // namespace

void ColorPopover::open(int slot, Rectangle anchor, const std::string& title, Color current) {
    m_open = true;
    m_justOpened = true;
    m_slot = slot;
    m_anchor = anchor;
    m_title = title;
    m_color = current;
}

ColorPopover::Result ColorPopover::draw() {
    Result result;
    if (!m_open) return result;
    const ui::Theme& t = ui::theme();

    float w = ui::px(276.0f), h = ui::px(300.0f);
    float x = std::min(m_anchor.x, static_cast<float>(GetScreenWidth()) - w - ui::px(8));
    float y = m_anchor.y + m_anchor.height + ui::px(8);
    if (y + h > GetScreenHeight() - ui::px(8)) y = m_anchor.y - h - ui::px(8);  // open upwards
    m_bounds = {std::max(ui::px(8), x), std::max(ui::px(8), y), w, h};

    if (!m_justOpened && ui::mousePressed(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(GetMousePosition(), m_bounds)) {
        m_open = false;
        return result;
    }
    m_justOpened = false;

    ui::card(m_bounds);
    ui::Column col(m_bounds.x + ui::px(ui::kPad), m_bounds.y + ui::px(14), m_bounds.width - 2 * ui::px(ui::kPad));
    Rectangle head = col.row(24);
    Rectangle preview = ui::takeRight(head, 24.0f, 4.0f);
    ui::textIn(head, m_title.c_str(), ui::FontRole::Title, t.text);
    ui::fillRounded(preview, ui::px(ui::kRadius), m_color);
    ui::strokeRounded(preview, ui::px(ui::kRadius), ui::pxi(1), t.borderStrong);

    // raygui picker: saturation/value square plus hue bar on the right
    Rectangle pickerRow = col.row(150);
    float hueSpace = ui::px(18.0f + 10.0f);
    Rectangle picker = {pickerRow.x, pickerRow.y, pickerRow.width - hueSpace, pickerRow.height};
    Color before = m_color;
    GuiColorPicker(picker, nullptr, &m_color);
    m_color.a = 255;
    if (m_color.r != before.r || m_color.g != before.g || m_color.b != before.b) {
        result.changed = true;
        result.color = m_color;
    }

    // Preset chips
    Rectangle chips = col.row(26);
    auto cells = ui::splitRow(chips, {1, 1, 1, 1, 1, 1, 1, 1}, 6.0f);
    for (int i = 0; i < 8; i++) {
        Rectangle c = {cells[i].x, cells[i].y, std::min(cells[i].width, chips.height), chips.height};
        if (ui::swatch(c, kPresets[i], ColorToInt(kPresets[i]) == ColorToInt(m_color))) {
            m_color = kPresets[i];
            result.changed = true;
            result.color = m_color;
        }
    }

    char hex[16];
    snprintf(hex, sizeof(hex), "#%02X%02X%02X", m_color.r, m_color.g, m_color.b);
    ui::textIn(col.row(18), hex, ui::FontRole::Mono, t.textMuted);

    auto buttons = ui::splitRow(col.row(ui::kRowHeight), {1, 1}, 8.0f);
    if (ui::button(buttons[0], "Theme color", ui::ButtonKind::Secondary, true, ui::Icon::Reset)) {
        result.reset = true;
        m_open = false;
    }
    if (ui::button(buttons[1], "Done", ui::ButtonKind::Primary)) m_open = false;
    return result;
}

} // namespace linalg
