#pragma once

#include "math/Matrix3D.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"
#include <raylib.h>
#include <string>
#include <array>

namespace linalg {

class InputPanel {
public:
    InputPanel();

    void update();
    void draw();

    // Get current matrix values
    Matrix3D getMatrix3D() const;
    Matrix4D getMatrix4D() const;

    // Get vector values
    Vector3D getVectorA() const;
    Vector3D getVectorB() const;

    // Check if matrices/vectors have changed
    bool matrixChanged() const { return m_matrixChanged; }
    bool vectorsChanged() const { return m_vectorsChanged; }
    void resetChangedFlags();

    // UI state
    bool isUsing4x4() const { return m_use4x4; }
    bool showVectorOperations() const { return m_showVectorOps; }

private:
    void drawMatrixPanel();
    void drawVectorPanel();
    void drawPresetPanel();

    bool m_use4x4;
    bool m_showVectorOps;
    bool m_matrixChanged;
    bool m_vectorsChanged;

    // Matrix 3x3 storage as text buffers
    char m_mat3Text[3][3][32];

    // Matrix 4x4 storage as text buffers
    char m_mat4Text[4][4][32];

    // Vector storage as text buffers
    char m_vecAText[3][32];
    char m_vecBText[3][32];

    // Active text box (-1 if none)
    int m_activeMatrix3Cell;
    int m_activeMatrix4Cell;
    int m_activeVecACell;
    int m_activeVecBCell;

    // UI positions
    Rectangle m_matrixPanelRect;
    Rectangle m_vectorPanelRect;
    Rectangle m_presetPanelRect;

    // Scroll state
    Vector2 m_scrollPanel;
    Rectangle m_scrollView;
};

} // namespace linalg
