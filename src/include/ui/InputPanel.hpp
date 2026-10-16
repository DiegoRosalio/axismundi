#pragma once

#include "math/Matrix3D.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"
#include "ui/SceneIO.hpp"
#include <raylib.h>
#include <vector>

namespace linalg {

enum class MatrixPreset {
    Identity,
    Rotate45Z,
    Scale2,
    ShearX,
    ReflectX,
};

// Matrix + vector editor for the linear algebra mode, with presets and
// undo/redo. Fields accept numbers or expressions such as "cos(pi/4)".
class InputPanel {
public:
    InputPanel();

    // Keyboard handling for the fields (Tab = next field)
    void update();

    // Draws the matrix, vector and preset groups at (x, y); returns the bottom y.
    float draw(float x, float y, float width);

    // Committed values (what the visualization uses)
    Matrix3D getMatrix3D() const;
    Matrix4D getMatrix4D() const;
    Vector3D getVectorA() const;
    Vector3D getVectorB() const;

    // The active transform as a 4x4 matrix (3x3 is embedded as the linear part)
    Matrix4D getActiveMatrix4D() const;

    bool matrixChanged() const { return m_matrixChanged; }
    bool vectorsChanged() const { return m_vectorsChanged; }
    void resetChangedFlags();

    bool isUsing4x4() const { return m_committed.use4x4; }
    bool isEditing() const { return m_activeField >= 0; }
    void finishEditing();  // accept or reject the field being edited
    void cancelEditing();  // discard the edit in progress

    void applyPreset(MatrixPreset preset);

    bool canUndo() const { return !m_undo.empty(); }
    bool canRedo() const { return !m_redo.empty(); }
    void undo();
    void redo();

    void saveState(SceneData& data) const;
    void loadState(const SceneData& data);

private:
    static constexpr int kTextSize = 32;
    static constexpr int kVectorFieldBase = 100;  // ids 100-102 = A, 103-105 = B
    static constexpr size_t kMaxHistory = 100;

    struct State {
        char mat3[3][3][kTextSize];
        char mat4[4][4][kTextSize];
        char vecA[3][kTextSize];
        char vecB[3][kTextSize];
        bool use4x4 = false;

        bool operator==(const State& other) const;
    };

    float drawMatrixGroup(float x, float y, float width);
    float drawVectorGroup(float x, float y, float width);
    float drawPresetGroup(float x, float y, float width);

    // Text buffer for a field id in a state
    char* fieldText(State& s, int id) const;
    std::vector<int> visibleFields() const;

    // Validate the edited field; keep it if valid, otherwise restore it
    void finishField(int id);
    // Record m_state as the new committed state (with undo history)
    void commit();
    void restore(const State& s);

    static void setMatrix3(State& s, const Matrix3D& m);
    static void setMatrix4(State& s, const Matrix4D& m);
    static Matrix3D parseMatrix3(const State& s);
    static Matrix4D parseMatrix4(const State& s);

    State m_state;      // what the text boxes show (may contain edits in progress)
    State m_committed;  // last accepted state
    std::vector<State> m_undo;
    std::vector<State> m_redo;

    int m_activeField;
    bool m_matrixChanged;
    bool m_vectorsChanged;
};

} // namespace linalg
