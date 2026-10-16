#include "ui/InputPanel.hpp"
#include <raygui.h>
#include <cstring>
#include <cstdio>
#include <cmath>

namespace linalg {

InputPanel::InputPanel()
    : m_use4x4(false), m_showVectorOps(true),
      m_matrixChanged(true), m_vectorsChanged(true),
      m_activeMatrix3Cell(-1), m_activeMatrix4Cell(-1),
      m_activeVecACell(-1), m_activeVecBCell(-1),
      m_scrollPanel({0, 0}) {

    // Initialize matrix 3x3 to identity
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            float val = (i == j) ? 1.0f : 0.0f;
            snprintf(m_mat3Text[i][j], 32, "%.2f", val);
        }
    }

    // Initialize matrix 4x4 to identity
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float val = (i == j) ? 1.0f : 0.0f;
            snprintf(m_mat4Text[i][j], 32, "%.2f", val);
        }
    }

    // Initialize vectors
    snprintf(m_vecAText[0], 32, "1.00");
    snprintf(m_vecAText[1], 32, "0.00");
    snprintf(m_vecAText[2], 32, "0.00");

    snprintf(m_vecBText[0], 32, "0.00");
    snprintf(m_vecBText[1], 32, "1.00");
    snprintf(m_vecBText[2], 32, "0.00");

    // Panel positions will be updated in draw() based on window size
    m_matrixPanelRect = {10, 10, 450, 400};
    m_vectorPanelRect = {10, 420, 450, 250};
    m_presetPanelRect = {10, 680, 450, 180};

    m_scrollView = {10, 10, 450, 850};
}

void InputPanel::update() {
    // Handle keyboard input for text boxes
}

void InputPanel::draw() {
    GuiSetStyle(DEFAULT, TEXT_SIZE, 28);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 2);

    // Update panel positions based on current window size
    int screenHeight = GetScreenHeight();

    // Minimum heights for each panel
    const float minMatrixHeight = 250.0f;
    const float minVectorHeight = 230.0f;
    const float minPresetHeight = 150.0f;

    // Calculate panel heights dynamically
    float totalHeight = screenHeight - 20; // Leave 10px margin top and bottom
    float matrixHeight = fmax(minMatrixHeight, totalHeight * 0.35f); // 35% for matrix
    float vectorHeight = fmax(minVectorHeight, totalHeight * 0.30f); // 30% for vectors
    float presetHeight = fmax(minPresetHeight, totalHeight * 0.30f); // 30% for presets

    // Update panel rectangles
    float currentY = 10 - m_scrollPanel.y;
    m_matrixPanelRect = {10, currentY, 450, matrixHeight};

    currentY += matrixHeight + 10;
    m_vectorPanelRect = {10, currentY, 450, vectorHeight};

    currentY += vectorHeight + 10;
    m_presetPanelRect = {10, currentY, 450, presetHeight};

    // Calculate total content height
    float totalContentHeight = matrixHeight + vectorHeight + presetHeight + 40; // +40 for spacing

    // Handle scrolling if content doesn't fit
    if (totalContentHeight > screenHeight) {
        float mouseWheel = GetMouseWheelMove();
        Vector2 mousePos = GetMousePosition();

        // Only scroll if mouse is over the panel area
        if (mousePos.x < 470) {
            m_scrollPanel.y -= mouseWheel * 20;

            // Clamp scroll
            float maxScroll = totalContentHeight - screenHeight + 20;
            if (m_scrollPanel.y < 0) m_scrollPanel.y = 0;
            if (m_scrollPanel.y > maxScroll) m_scrollPanel.y = maxScroll;
        }
    } else {
        m_scrollPanel.y = 0; // Reset scroll if everything fits
    }

    // Draw panels
    BeginScissorMode(0, 0, 470, screenHeight);
    drawMatrixPanel();
    drawVectorPanel();
    drawPresetPanel();
    EndScissorMode();

    // Draw scrollbar indicator if needed
    if (totalContentHeight > screenHeight) {
        float scrollBarHeight = (screenHeight / totalContentHeight) * screenHeight;
        float scrollBarY = (m_scrollPanel.y / (totalContentHeight - screenHeight)) * (screenHeight - scrollBarHeight);
        DrawRectangle(465, scrollBarY, 5, scrollBarHeight, DARKGRAY);
    }
}

void InputPanel::drawMatrixPanel() {
    GuiGroupBox(m_matrixPanelRect, "Matrix Transformation");

    float x = m_matrixPanelRect.x + 15;
    float y = m_matrixPanelRect.y + 30;

    // Matrix type toggle
    if (GuiToggle({x, y, 200, 30}, m_use4x4 ? "4x4 Matrix" : "3x3 Matrix", &m_use4x4)) {
        m_matrixChanged = true;
    }
    y += 45;

    if (!m_use4x4) {
        // Draw 3x3 matrix input
        GuiLabel({x, y, 100, 20}, "3x3 Matrix");
        y += 25;

        float cellWidth = 110;
        float cellHeight = 50;
        float spacing = 8;

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                int cellId = i * 3 + j;
                Rectangle rect = {
                    x + j * (cellWidth + spacing),
                    y + i * (cellHeight + spacing),
                    cellWidth,
                    cellHeight
                };

                bool wasActive = (m_activeMatrix3Cell == cellId);
                if (GuiTextBox(rect, m_mat3Text[i][j], 32, m_activeMatrix3Cell == cellId)) {
                    m_activeMatrix3Cell = (m_activeMatrix3Cell == cellId) ? -1 : cellId;
                    if (!wasActive) {
                        // Just activated, don't mark as changed yet
                    } else {
                        // Deactivated, mark as changed
                        m_matrixChanged = true;
                    }
                }
            }
        }
    } else {
        // Draw 4x4 matrix input
        GuiLabel({x, y, 100, 20}, "4x4 Matrix");
        y += 25;

        float cellWidth = 95;
        float cellHeight = 48;
        float spacing = 6;

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                int cellId = i * 4 + j;
                Rectangle rect = {
                    x + j * (cellWidth + spacing),
                    y + i * (cellHeight + spacing),
                    cellWidth,
                    cellHeight
                };

                bool wasActive = (m_activeMatrix4Cell == cellId);
                if (GuiTextBox(rect, m_mat4Text[i][j], 32, m_activeMatrix4Cell == cellId)) {
                    m_activeMatrix4Cell = (m_activeMatrix4Cell == cellId) ? -1 : cellId;
                    if (!wasActive) {
                        // Just activated
                    } else {
                        // Deactivated
                        m_matrixChanged = true;
                    }
                }
            }
        }
    }
}

void InputPanel::drawVectorPanel() {
    GuiGroupBox(m_vectorPanelRect, "Vectors");

    float x = m_vectorPanelRect.x + 15;
    float y = m_vectorPanelRect.y + 30;

    // Vector A
    GuiLabel({x, y, 100, 20}, "Vector A");
    y += 25;

    const char* labels[3] = {"X:", "Y:", "Z:"};

    for (int i = 0; i < 3; i++) {
        GuiLabel({x, y, 50, 40}, labels[i]);

        Rectangle rect = {x + 55, y, 150, 48};
        bool wasActive = (m_activeVecACell == i);
        if (GuiTextBox(rect, m_vecAText[i], 32, m_activeVecACell == i)) {
            m_activeVecACell = (m_activeVecACell == i) ? -1 : i;
            if (wasActive) {
                m_vectorsChanged = true;
            }
        }
        y += 55;
    }

    y += 10;

    // Vector B
    GuiLabel({x, y, 100, 20}, "Vector B");
    y += 25;

    for (int i = 0; i < 3; i++) {
        GuiLabel({x, y, 50, 40}, labels[i]);

        Rectangle rect = {x + 55, y, 150, 48};
        bool wasActive = (m_activeVecBCell == i);
        if (GuiTextBox(rect, m_vecBText[i], 32, m_activeVecBCell == i)) {
            m_activeVecBCell = (m_activeVecBCell == i) ? -1 : i;
            if (wasActive) {
                m_vectorsChanged = true;
            }
        }
        y += 55;
    }
}

void InputPanel::drawPresetPanel() {
    GuiGroupBox(m_presetPanelRect, "Preset Transformations");

    float x = m_presetPanelRect.x + 15;
    float y = m_presetPanelRect.y + 30;

    float btnWidth = 200;
    float btnHeight = 52;
    float spacing = 15;

    // Identity button
    if (GuiButton({x, y, btnWidth, btnHeight}, "Identity Matrix")) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                float val = (i == j) ? 1.0f : 0.0f;
                snprintf(m_mat3Text[i][j], 32, "%.2f", val);
            }
        }
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                float val = (i == j) ? 1.0f : 0.0f;
                snprintf(m_mat4Text[i][j], 32, "%.2f", val);
            }
        }
        m_matrixChanged = true;
    }
    y += btnHeight + spacing;

    // Rotation button
    if (GuiButton({x, y, btnWidth, btnHeight}, "Rotate 45° (Z-axis)")) {
        float angle = 45.0f * DEG2RAD;
        float c = cosf(angle);
        float s = sinf(angle);

        snprintf(m_mat3Text[0][0], 32, "%.2f", c);
        snprintf(m_mat3Text[0][1], 32, "%.2f", -s);
        snprintf(m_mat3Text[0][2], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[1][0], 32, "%.2f", s);
        snprintf(m_mat3Text[1][1], 32, "%.2f", c);
        snprintf(m_mat3Text[1][2], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[2][0], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[2][1], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[2][2], 32, "%.2f", 1.0f);

        m_matrixChanged = true;
    }
    y += btnHeight + spacing;

    // Scale button
    if (GuiButton({x, y, btnWidth, btnHeight}, "Scale 2x")) {
        snprintf(m_mat3Text[0][0], 32, "%.2f", 2.0f);
        snprintf(m_mat3Text[0][1], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[0][2], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[1][0], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[1][1], 32, "%.2f", 2.0f);
        snprintf(m_mat3Text[1][2], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[2][0], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[2][1], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[2][2], 32, "%.2f", 1.0f);

        m_matrixChanged = true;
    }

    // Second column of buttons
    x += btnWidth + 20;
    y = m_presetPanelRect.y + 30;

    if (GuiButton({x, y, btnWidth, btnHeight}, "Shear X by 0.5")) {
        snprintf(m_mat3Text[0][0], 32, "%.2f", 1.0f);
        snprintf(m_mat3Text[0][1], 32, "%.2f", 0.5f);
        snprintf(m_mat3Text[0][2], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[1][0], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[1][1], 32, "%.2f", 1.0f);
        snprintf(m_mat3Text[1][2], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[2][0], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[2][1], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[2][2], 32, "%.2f", 1.0f);

        m_matrixChanged = true;
    }
    y += btnHeight + spacing;

    if (GuiButton({x, y, btnWidth, btnHeight}, "Reflect X-axis")) {
        snprintf(m_mat3Text[0][0], 32, "%.2f", -1.0f);
        snprintf(m_mat3Text[0][1], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[0][2], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[1][0], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[1][1], 32, "%.2f", 1.0f);
        snprintf(m_mat3Text[1][2], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[2][0], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[2][1], 32, "%.2f", 0.0f);
        snprintf(m_mat3Text[2][2], 32, "%.2f", 1.0f);

        m_matrixChanged = true;
    }
}

Matrix3D InputPanel::getMatrix3D() const {
    std::array<std::array<float, 3>, 3> mat;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            float val = 0.0f;
            sscanf(m_mat3Text[i][j], "%f", &val);
            mat[i][j] = val;
        }
    }

    return Matrix3D(mat);
}

Matrix4D InputPanel::getMatrix4D() const {
    std::array<std::array<float, 4>, 4> mat;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float val = 0.0f;
            sscanf(m_mat4Text[i][j], "%f", &val);
            mat[i][j] = val;
        }
    }

    return Matrix4D(mat);
}

Vector3D InputPanel::getVectorA() const {
    float x = 0, y = 0, z = 0;
    sscanf(m_vecAText[0], "%f", &x);
    sscanf(m_vecAText[1], "%f", &y);
    sscanf(m_vecAText[2], "%f", &z);
    return Vector3D(x, y, z);
}

Vector3D InputPanel::getVectorB() const {
    float x = 0, y = 0, z = 0;
    sscanf(m_vecBText[0], "%f", &x);
    sscanf(m_vecBText[1], "%f", &y);
    sscanf(m_vecBText[2], "%f", &z);
    return Vector3D(x, y, z);
}

void InputPanel::resetChangedFlags() {
    m_matrixChanged = false;
    m_vectorsChanged = false;
}

} // namespace linalg
