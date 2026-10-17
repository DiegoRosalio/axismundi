#include "ui/ViewPanel.hpp"
#include "ui/SceneIO.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <algorithm>
#include <cmath>
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

    // Style: 2x2 cards, each with a swatch of the style's board and palette
    ui::textIn(col.row(18), ui::upper("Style").c_str(), ui::FontRole::Caption, t.textMuted);
    for (int rowIndex = 0; rowIndex < 2; rowIndex++) {
        auto cards = ui::splitRow(col.row(34), {1, 1}, 8.0f);
        for (int k = 0; k < 2; k++) {
            auto id = static_cast<view::StyleId>(rowIndex * 2 + k);
            if (styleCard(cards[k], id, m_settings.style == id)) m_settings.style = id;
        }
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

    // Grid size
    row = col.row(30);
    label = ui::takeLeft(row, 92.0f, 8.0f);
    value = ui::takeRight(row, 36.0f, 8.0f);
    ui::textIn(label, "Grid size", ui::FontRole::Body, t.text);
    if (ui::slider(row, m_settings.gridSize, 5.0f, 100.0f)) m_settings.gridSize = std::round(m_settings.gridSize);
    ui::anchor("view.gridsize", row);
    ui::tooltip({label.x, row.y, value.x + value.width - label.x, row.height},
                "How far the grid reaches from the origin, in units. Lines every unit, stronger every 5.", "Grid size");
    snprintf(lengthText, sizeof(lengthText), "±%.0f", m_settings.gridSize);
    ui::textIn(value, lengthText, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
    col.space(6.0f);

    Rectangle reset = col.row(ui::kRowHeight);
    if (ui::button(reset, "Reset camera", ui::ButtonKind::Secondary, true, ui::Icon::Reset, "C")) {
        m_resetCamera = true;
    }
    ui::tooltip(reset, "Back to the default viewpoint for this mode.");
    auto cells = ui::splitRow(col.row(ui::kRowHeight), {1, 1}, 8.0f);
    if (ui::button(cells[0], "New", ui::ButtonKind::Secondary, true, ui::Icon::Plus)) m_new = true;
    if (ui::button(cells[1], "Open…", ui::ButtonKind::Secondary, true, ui::Icon::Load, "F9")) m_load = true;
    ui::tooltip(cells[0], "Start an empty scene (both spaces). Ctrl+Z brings the previous one back.", "New scene");
    ui::tooltip(cells[1], "Pick a scene file to open (Ctrl+O). Opening can be undone.", "Open scene");
    ui::anchor("view.open", cells[1]);
    cells = ui::splitRow(col.row(ui::kRowHeight), {1, 1}, 8.0f);
    if (ui::button(cells[0], "Save", ui::ButtonKind::Secondary, true, ui::Icon::Save, "F5")) m_save = true;
    if (ui::button(cells[1], "Save as…", ui::ButtonKind::Secondary, true)) m_saveAs = true;
    ui::tooltip(cells[0], "Save objects, camera, view and the clock to the current file (Ctrl+S). The first time "
                          "it asks for a name.", "Save scene");
    ui::tooltip(cells[1], "Save under a new name or in another folder (Ctrl+Shift+S).", "Save as");
    std::string file = m_currentFile.empty() ? std::string("Not saved yet")
                                             : "File: " + m_currentFile.substr(m_currentFile.find_last_of("/\\") + 1);
    Rectangle fileRow = col.row(18.0f);
    ui::textIn(fileRow, ui::ellipsize(file.c_str(), ui::FontRole::Small, fileRow.width).c_str(), ui::FontRole::Small,
               t.textFaint);
    if (!m_currentFile.empty()) ui::tooltip(fileRow, m_currentFile.c_str(), "Current file");
    col.space(10.0f);
    return col.y();
}

bool ViewPanel::styleCard(Rectangle r, view::StyleId id, bool selected) {
    const ui::Theme& t = ui::theme();
    view::ViewStyle s = view::styleFor(id, t);
    bool hover = ui::hovered(r);
    float radius = ui::px(7.0f);
    ui::fillRounded(r, radius, selected ? t.accentSoft : hover ? t.hover : t.panelAlt);
    ui::strokeRounded(r, radius, ui::px(selected ? 2.0f : 1.0f), selected ? t.accent : t.border);

    // Swatch: the board color with three palette dots
    float pad = ui::px(6.0f);
    Rectangle sw = {r.x + pad, r.y + pad, r.height - 2 * pad + ui::px(10.0f), r.height - 2 * pad};
    ui::fillRounded(sw, ui::px(4.0f), s.background);
    ui::strokeRounded(sw, ui::px(4.0f), ui::px(1.0f), t.border);
    Color dots[3] = {s.vectors[0], s.vectors[1], s.id == view::StyleId::Paper ? Color{120, 120, 120, 255} : s.vectors[2]};
    if (s.id == view::StyleId::Paper) dots[0] = dots[1] = Color{20, 20, 20, 255};
    for (int i = 0; i < 3; i++) {
        float cx = sw.x + sw.width * (0.25f + 0.25f * i), cy = sw.y + sw.height / 2;
        DrawCircleV({cx, cy}, ui::px(3.2f), dots[i]);
    }
    Rectangle label = {sw.x + sw.width + ui::px(8.0f), r.y, r.x + r.width - sw.x - sw.width - ui::px(12.0f), r.height};
    ui::textIn(label, ui::ellipsize(view::styleName(id), ui::FontRole::Body, label.width).c_str(),
               selected ? ui::FontRole::BodyStrong : ui::FontRole::Body, t.text);
    ui::anchor((std::string("view.style.") + view::styleKey(id)).c_str(), r);
    ui::tooltip(r, view::styleDescription(id), view::styleName(id));
    return ui::clicked(r);
}

void ViewPanel::saveState(SceneData& data) const {
    putBool(data, "view.grid", m_settings.showGrid);
    putBool(data, "view.axes", m_settings.showAxes);
    putBool(data, "view.labels", m_settings.showLabels);
    putFloat(data, "view.axis_length", m_settings.axisLength);
    putFloat(data, "view.grid_size", m_settings.gridSize);
    data["view.style"] = view::styleKey(m_settings.style);
}

void ViewPanel::loadState(const SceneData& data) {
    getBool(data, "view.grid", m_settings.showGrid);
    getBool(data, "view.axes", m_settings.showAxes);
    getBool(data, "view.labels", m_settings.showLabels);
    if (getFloat(data, "view.axis_length", m_settings.axisLength)) {
        if (m_settings.axisLength < 1.0f) m_settings.axisLength = 1.0f;
        if (m_settings.axisLength > 20.0f) m_settings.axisLength = 20.0f;
    }
    if (getFloat(data, "view.grid_size", m_settings.gridSize))
        m_settings.gridSize = std::clamp(std::round(m_settings.gridSize), 5.0f, 100.0f);
    // Files without a style (older ones) keep the current style
    std::string style;
    if (getString(data, "view.style", style)) m_settings.style = view::styleFromKey(style, m_settings.style);
}

} // namespace linalg
