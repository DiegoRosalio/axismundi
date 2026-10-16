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
    if (mode == AppMode::LinearAlgebra) {
        ui::toggleRow(col.row(30), "Eigenvectors", m_settings.showEigenvectors);
        ui::toggleRow(col.row(30), "Transformed A, B", m_settings.showTransformedVectors);
    }
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

    if (ui::button(col.row(ui::kRowHeight), "Reset camera", ui::ButtonKind::Secondary, true, ui::Icon::Reset, "C")) {
        m_resetCamera = true;
    }
    auto cells = ui::splitRow(col.row(ui::kRowHeight), {1, 1}, 8.0f);
    if (ui::button(cells[0], "Save", ui::ButtonKind::Secondary, true, ui::Icon::Save, "F5")) m_save = true;
    if (ui::button(cells[1], "Load", ui::ButtonKind::Secondary, true, ui::Icon::Load, "F9")) m_load = true;
    col.space(10.0f);
    return col.y();
}

void ViewPanel::saveState(SceneData& data) const {
    putBool(data, "view.grid", m_settings.showGrid);
    putBool(data, "view.axes", m_settings.showAxes);
    putBool(data, "view.labels", m_settings.showLabels);
    putBool(data, "view.eigenvectors", m_settings.showEigenvectors);
    putBool(data, "view.transformed", m_settings.showTransformedVectors);
    putFloat(data, "view.axis_length", m_settings.axisLength);
}

void ViewPanel::loadState(const SceneData& data) {
    getBool(data, "view.grid", m_settings.showGrid);
    getBool(data, "view.axes", m_settings.showAxes);
    getBool(data, "view.labels", m_settings.showLabels);
    getBool(data, "view.eigenvectors", m_settings.showEigenvectors);
    getBool(data, "view.transformed", m_settings.showTransformedVectors);
    if (getFloat(data, "view.axis_length", m_settings.axisLength)) {
        if (m_settings.axisLength < 1.0f) m_settings.axisLength = 1.0f;
        if (m_settings.axisLength > 20.0f) m_settings.axisLength = 20.0f;
    }
}

} // namespace linalg
