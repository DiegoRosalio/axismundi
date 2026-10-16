#include "ui/ViewPanel.hpp"
#include "ui/SceneIO.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <cstdio>

namespace linalg {

const char* vectorSlotName(int slot) {
    static const char* names[kVectorSlotCount] = {"A", "B", "A+B", "A×B", "MA", "MB"};
    return names[slot];
}

namespace {
// Keys for the preferences file (ASCII)
const char* kColorKeys[kVectorSlotCount] = {"a", "b", "sum", "cross", "ma", "mb"};
}

Color ViewSettings::colorFor(int slot) const {
    if (customColor[slot]) return vectorColor[slot];
    const ui::Theme& t = ui::theme();
    const Color defaults[kVectorSlotCount] = {t.vecA, t.vecB, t.vecSum, t.vecCross, t.vecMA, t.vecMB};
    return defaults[slot];
}

int ViewPanel::takeColorRequest(Rectangle& anchor) {
    int slot = m_colorRequest;
    anchor = m_colorAnchor;
    m_colorRequest = -1;
    return slot;
}

float ViewPanel::draw(float x, float y, float width, AppMode mode) {
    const ui::Theme& t = ui::theme();
    ui::Column col(x, y, width, 4.0f);
    ui::sectionHeader(col.row(24), "View", m_expanded);
    if (!m_expanded) return col.y() + ui::px(6.0f);

    ui::toggleRow(col.row(30), "Grid", m_settings.showGrid, "G");
    ui::toggleRow(col.row(30), "Axes", m_settings.showAxes);
    ui::toggleRow(col.row(30), "Labels", m_settings.showLabels, "L");
    if (mode == AppMode::LinearAlgebra) {
        Rectangle eig = col.row(30);
        ui::toggleRow(eig, "Eigenvectors", m_settings.showEigenvectors);
        ui::anchor("view.eigen", eig);
        ui::tooltip(eig, "Dashed lines along the directions M only stretches; the dot marks λv.", "Eigenvectors");
        Rectangle tr = col.row(30);
        ui::toggleRow(tr, "Transformed A, B", m_settings.showTransformedVectors);
        ui::tooltip(tr, "MA and MB: where the matrix sends the vectors A and B.", "Transformed vectors");

        // Vector colors: one swatch per vector, label underneath
        col.space(6.0f);
        ui::textIn(col.row(16), "Vector colors", ui::FontRole::Small, t.textMuted);
        Rectangle row = col.row(44);
        auto cells = ui::splitRow(row, {1, 1, 1, 1, 1, 1}, 6.0f);
        for (int i = 0; i < kVectorSlotCount; i++) {
            float s = ui::px(24.0f);
            Rectangle sw = {cells[i].x + (cells[i].width - s) / 2, cells[i].y, s, s};
            if (ui::swatch(sw, m_settings.colorFor(i), m_settings.customColor[i])) {
                m_colorRequest = i;
                m_colorAnchor = sw;
            }
            ui::textIn({cells[i].x, sw.y + s + ui::px(2), cells[i].width, ui::px(16)}, vectorSlotName(i),
                       ui::FontRole::Small, t.textMuted, ui::Align::Center);
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
    putBool(data, "view.eigenvectors", m_settings.showEigenvectors);
    putBool(data, "view.transformed", m_settings.showTransformedVectors);
    putFloat(data, "view.axis_length", m_settings.axisLength);
}

void ViewPanel::saveColors(SceneData& prefs) const {
    for (int i = 0; i < kVectorSlotCount; i++) {
        if (!m_settings.customColor[i]) continue;
        Color c = m_settings.vectorColor[i];
        char hex[16];
        snprintf(hex, sizeof(hex), "#%02x%02x%02x", c.r, c.g, c.b);
        prefs[std::string("color.") + kColorKeys[i]] = hex;
    }
}

void ViewPanel::loadColors(const SceneData& prefs) {
    for (int i = 0; i < kVectorSlotCount; i++) {
        std::string hex;
        unsigned int r, g, b;
        if (getString(prefs, std::string("color.") + kColorKeys[i], hex) &&
            std::sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b) == 3) {
            m_settings.customColor[i] = true;
            m_settings.vectorColor[i] = {static_cast<unsigned char>(r), static_cast<unsigned char>(g),
                                         static_cast<unsigned char>(b), 255};
        }
    }
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
