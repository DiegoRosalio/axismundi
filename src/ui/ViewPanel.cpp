#include "ui/ViewPanel.hpp"
#include "ui/SceneIO.hpp"
#include "ui/Widgets.hpp"
#include <raygui.h>
#include <cstdio>

namespace linalg {

float ViewPanel::draw(float x, float y, float width, AppMode mode) {
    const float pad = 15.0f;
    const float rowH = 40.0f;
    const float boxSize = 28.0f;
    float top = y;
    float cx = x + pad;
    float cy = y + 30.0f;
    float half = (width - 2 * pad) / 2.0f;

    // Checkboxes, two per row
    GuiCheckBox({cx, cy, boxSize, boxSize}, "Grid [G]", &m_settings.showGrid);
    GuiCheckBox({cx + half, cy, boxSize, boxSize}, "Axes", &m_settings.showAxes);
    cy += rowH;
    GuiCheckBox({cx, cy, boxSize, boxSize}, "Labels [L]", &m_settings.showLabels);
    if (mode == AppMode::LinearAlgebra) {
        GuiCheckBox({cx + half, cy, boxSize, boxSize}, "Eigenvecs", &m_settings.showEigenvectors);
        cy += rowH;
        GuiCheckBox({cx, cy, boxSize, boxSize}, "Transformed A, B (MA, MB)",
                    &m_settings.showTransformedVectors);
    }
    cy += rowH + 5.0f;

    // Axis length
    char lengthText[32];
    snprintf(lengthText, sizeof(lengthText), "%.1f", m_settings.axisLength);
    GuiLabel({cx, cy, 120, 32}, "Axes len");
    GuiSlider({cx + 130, cy, width - 2 * pad - 210, 32}, nullptr, lengthText,
              &m_settings.axisLength, 1.0f, 20.0f);
    cy += rowH + 5.0f;

    // Buttons
    float btnH = 46.0f;
    if (GuiButton({cx, cy, width - 2 * pad, btnH}, "Reset Camera [C]")) m_resetCamera = true;
    cy += btnH + 10.0f;
    if (GuiButton({cx, cy, half - 5, btnH}, "Save [F5]")) m_save = true;
    if (GuiButton({cx + half + 5, cy, half - 5, btnH}, "Load [F9]")) m_load = true;
    cy += btnH + 15.0f;

    GuiGroupBox({x, top, width, cy - top}, "View");
    return cy;
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
