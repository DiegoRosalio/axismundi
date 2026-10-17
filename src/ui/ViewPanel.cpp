#include "ui/ViewPanel.hpp"
#include "ui/SceneIO.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <cstdio>

namespace linalg {

float ViewPanel::draw(float x, float y, float width, AppMode mode) {
    const ui::Theme& t = ui::theme();
    ui::Column col(x, y, width, 4.0f);
    ui::sectionHeader(col.row(24), "View", m_expanded);
    if (!m_expanded) return col.y() + ui::px(6.0f);

    ui::toggleRow(col.row(30), "Grid", m_settings.showGrid, "G");
    ui::toggleRow(col.row(30), "Axes", m_settings.showAxes);
    ui::toggleRow(col.row(30), "Labels", m_settings.showLabels, "L");
    (void)mode;
    col.space(4.0f);

    // Axis length
    Rectangle row = col.row(30);
    Rectangle label = ui::takeLeft(row, 92.0f, 8.0f);
    Rectangle value = ui::takeRight(row, 36.0f, 8.0f);
    ui::textIn(label, "Axis length", ui::FontRole::Body, t.text);
    ui::slider(row, m_settings.axisLength, 1.0f, 20.0f);
    char lengthText[16];
    snprintf(lengthText, sizeof(lengthText), "%.1f", m_settings.axisLength);
    ui::textIn(value, lengthText, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
    col.space(6.0f);

    Rectangle reset = col.row(ui::kRowHeight);
    if (ui::button(reset, "Reset camera", ui::ButtonKind::Secondary, true, ui::Icon::Reset, "C")) {
        m_resetCamera = true;
    }
    ui::tooltip(reset, "Back to the default viewpoint for this mode.");
    auto cells = ui::splitRow(col.row(ui::kRowHeight), {1, 1}, 8.0f);
    if (ui::button(cells[0], "Save", ui::ButtonKind::Secondary, true, ui::Icon::Save, "F5")) m_save = true;
    if (ui::button(cells[1], "Load", ui::ButtonKind::Secondary, true, ui::Icon::Load, "F9")) m_load = true;
    ui::tooltip(cells[0], "Save matrices, vectors, plots, camera and keyframes to scene.txt in the current folder.",
                "Save scene");
    ui::tooltip(cells[1], "Load scene.txt from the current folder. Loading can be undone.", "Load scene");
    col.space(10.0f);
    return col.y();
}

void ViewPanel::saveState(SceneData& data) const {
    putBool(data, "view.grid", m_settings.showGrid);
    putBool(data, "view.axes", m_settings.showAxes);
    putBool(data, "view.labels", m_settings.showLabels);
    putFloat(data, "view.axis_length", m_settings.axisLength);
}

void ViewPanel::loadState(const SceneData& data) {
    getBool(data, "view.grid", m_settings.showGrid);
    getBool(data, "view.axes", m_settings.showAxes);
    getBool(data, "view.labels", m_settings.showLabels);
    if (getFloat(data, "view.axis_length", m_settings.axisLength)) {
        if (m_settings.axisLength < 1.0f) m_settings.axisLength = 1.0f;
        if (m_settings.axisLength > 20.0f) m_settings.axisLength = 20.0f;
    }
}

} // namespace linalg
