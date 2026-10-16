#include "ui/InputPanel.hpp"
#include "ui/Widgets.hpp"
#include <raygui.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace linalg {

namespace {

float parseOrZero(const char* text) {
    float v = 0.0f;
    parseNumber(text, v);
    return v;
}

bool isValid(const char* text) {
    float v;
    return parseNumber(text, v);
}

} // namespace

bool InputPanel::State::operator==(const State& o) const {
    if (use4x4 != o.use4x4) return false;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (std::strcmp(mat3[i][j], o.mat3[i][j]) != 0) return false;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (std::strcmp(mat4[i][j], o.mat4[i][j]) != 0) return false;
    for (int i = 0; i < 3; i++)
        if (std::strcmp(vecA[i], o.vecA[i]) != 0 || std::strcmp(vecB[i], o.vecB[i]) != 0) return false;
    return true;
}

InputPanel::InputPanel()
    : m_state{}, m_committed{}, m_activeField(-1),
      m_matrixChanged(true), m_vectorsChanged(true) {
    setMatrix3(m_state, Matrix3D::identity());
    setMatrix4(m_state, Matrix4D::identity());

    const float a[3] = {1, 0, 0};
    const float b[3] = {0, 1, 0};
    for (int i = 0; i < 3; i++) {
        formatNumber(m_state.vecA[i], kTextSize, a[i]);
        formatNumber(m_state.vecB[i], kTextSize, b[i]);
    }
    m_committed = m_state;
}

// ---------------------------------------------------------------------------
// Fields
// ---------------------------------------------------------------------------

char* InputPanel::fieldText(State& s, int id) const {
    if (id >= kVectorFieldBase) {
        int v = id - kVectorFieldBase;
        return (v < 3) ? s.vecA[v] : s.vecB[v - 3];
    }
    if (s.use4x4) return s.mat4[id / 4][id % 4];
    return s.mat3[id / 3][id % 3];
}

std::vector<int> InputPanel::visibleFields() const {
    std::vector<int> ids;
    int cells = m_state.use4x4 ? 16 : 9;
    for (int i = 0; i < cells; i++) ids.push_back(i);
    for (int i = 0; i < 6; i++) ids.push_back(kVectorFieldBase + i);
    return ids;
}

void InputPanel::update() {
    if (m_activeField < 0) return;

    if (IsKeyPressed(KEY_TAB)) {
        int current = m_activeField;
        std::vector<int> ids = visibleFields();
        auto it = std::find(ids.begin(), ids.end(), current);
        bool back = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        size_t idx = (it == ids.end()) ? 0 : static_cast<size_t>(it - ids.begin());
        idx = back ? (idx + ids.size() - 1) % ids.size() : (idx + 1) % ids.size();

        finishField(current);
        m_activeField = ids[idx];
    }
}

void InputPanel::finishEditing() {
    if (m_activeField < 0) return;
    int id = m_activeField;
    m_activeField = -1;
    finishField(id);
}

void InputPanel::finishField(int id) {
    char* text = fieldText(m_state, id);
    if (isValid(text)) {
        commit();
    } else {
        // Reject the edit: put back the last accepted text
        std::snprintf(text, kTextSize, "%s", fieldText(m_committed, id));
    }
}

void InputPanel::commit() {
    if (m_state == m_committed) return;

    m_undo.push_back(m_committed);
    if (m_undo.size() > kMaxHistory) m_undo.erase(m_undo.begin());
    m_redo.clear();

    m_committed = m_state;
    m_matrixChanged = true;
    m_vectorsChanged = true;
}

void InputPanel::restore(const State& s) {
    m_state = s;
    m_committed = s;
    m_activeField = -1;
    m_matrixChanged = true;
    m_vectorsChanged = true;
}

void InputPanel::undo() {
    if (m_undo.empty()) return;
    m_redo.push_back(m_committed);
    State s = m_undo.back();
    m_undo.pop_back();
    restore(s);
}

void InputPanel::redo() {
    if (m_redo.empty()) return;
    m_undo.push_back(m_committed);
    State s = m_redo.back();
    m_redo.pop_back();
    restore(s);
}

// ---------------------------------------------------------------------------
// Presets
// ---------------------------------------------------------------------------

void InputPanel::setMatrix3(State& s, const Matrix3D& m) {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            formatNumber(s.mat3[i][j], kTextSize, m.m[i][j]);
}

void InputPanel::setMatrix4(State& s, const Matrix4D& m) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            formatNumber(s.mat4[i][j], kTextSize, m.m[i][j]);
}

void InputPanel::applyPreset(MatrixPreset preset) {
    Matrix3D m;
    switch (preset) {
        case MatrixPreset::Identity:  m = Matrix3D::identity(); break;
        case MatrixPreset::Rotate45Z: m = Matrix3D::rotation(45.0f * DEG2RAD); break;
        case MatrixPreset::Scale2:    m = Matrix3D::scale(2.0f, 2.0f); break;
        case MatrixPreset::ShearX:    m = Matrix3D::shear(0.5f, 0.0f); break;
        case MatrixPreset::ReflectX:  m = Matrix3D::scale(-1.0f, 1.0f); break;
    }

    // Apply to both sizes so the preset is visible in either mode
    Matrix4D m4 = Matrix4D::identity();
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            m4.m[i][j] = m.m[i][j];

    m_activeField = -1;
    m_state = m_committed;  // drop any half-typed edit
    setMatrix3(m_state, m);
    setMatrix4(m_state, m4);
    commit();
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

float InputPanel::draw(float x, float y, float width) {
    setGuiTextSize(kUiTextLarge);
    y = drawMatrixGroup(x, y, width) + 10.0f;
    y = drawVectorGroup(x, y, width) + 10.0f;
    y = drawPresetGroup(x, y, width);
    return y;
}

float InputPanel::drawMatrixGroup(float x, float y, float width) {
    const float pad = 15.0f;
    float top = y;
    float cx = x + pad;
    float cy = y + 30.0f;
    float inner = width - 2 * pad;

    // 3x3 / 4x4 toggle
    bool use4x4 = m_state.use4x4;
    GuiToggle({cx, cy, 190, 40}, use4x4 ? "4x4 Matrix" : "3x3 Matrix", &use4x4);
    if (use4x4 != m_state.use4x4) {
        m_activeField = -1;
        m_state = m_committed;
        m_state.use4x4 = use4x4;
        commit();
    }

    // Undo / redo
    float btnW = (inner - 200) / 2.0f;
    if (!canUndo()) GuiDisable();
    if (GuiButton({cx + 200, cy, btnW - 5, 40}, "Undo")) undo();
    GuiEnable();
    if (!canRedo()) GuiDisable();
    if (GuiButton({cx + 200 + btnW, cy, btnW - 5, 40}, "Redo")) redo();
    GuiEnable();
    cy += 55.0f;

    int n = m_state.use4x4 ? 4 : 3;
    float spacing = m_state.use4x4 ? 6.0f : 8.0f;
    float cellW = (inner - (n - 1) * spacing) / n;
    float cellH = m_state.use4x4 ? 46.0f : 50.0f;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int id = i * n + j;
            Rectangle rect = {cx + j * (cellW + spacing), cy + i * (cellH + spacing), cellW, cellH};
            char* text = fieldText(m_state, id);
            if (textField(rect, text, kTextSize, id, m_activeField, !isValid(text))) {
                finishField(id);
            }
        }
    }
    cy += n * (cellH + spacing) + 10.0f;

    GuiGroupBox({x, top, width, cy - top}, "Matrix Transformation");
    return cy;
}

float InputPanel::drawVectorGroup(float x, float y, float width) {
    const float pad = 15.0f;
    float top = y;
    float cx = x + pad;
    float cy = y + 25.0f;
    float labelW = 40.0f;
    float spacing = 8.0f;
    float cellW = (width - 2 * pad - labelW - 2 * spacing) / 3.0f;
    float cellH = 48.0f;

    // Column headers
    const char* axes[3] = {"x", "y", "z"};
    for (int k = 0; k < 3; k++) {
        GuiLabel({cx + labelW + k * (cellW + spacing) + cellW / 2 - 8, cy, 30, 28}, axes[k]);
    }
    cy += 30.0f;

    const char* names[2] = {"A", "B"};
    for (int v = 0; v < 2; v++) {
        GuiLabel({cx, cy + 8, labelW, 32}, names[v]);
        for (int k = 0; k < 3; k++) {
            int id = kVectorFieldBase + v * 3 + k;
            Rectangle rect = {cx + labelW + k * (cellW + spacing), cy, cellW, cellH};
            char* text = fieldText(m_state, id);
            if (textField(rect, text, kTextSize, id, m_activeField, !isValid(text))) {
                finishField(id);
            }
        }
        cy += cellH + spacing;
    }
    cy += 8.0f;

    GuiGroupBox({x, top, width, cy - top}, "Vectors");
    return cy;
}

float InputPanel::drawPresetGroup(float x, float y, float width) {
    const float pad = 15.0f;
    float top = y;
    float cx = x + pad;
    float cy = y + 30.0f;
    float btnW = (width - 2 * pad - 10.0f) / 2.0f;
    float btnH = 48.0f;

    struct Button { const char* label; MatrixPreset preset; };
    const Button buttons[] = {
        {"Identity [I]", MatrixPreset::Identity},
        {"Rot 45 [R]", MatrixPreset::Rotate45Z},
        {"Scale 2 [S]", MatrixPreset::Scale2},
        {"Shear [X]", MatrixPreset::ShearX},
        {"Reflect [F]", MatrixPreset::ReflectX},
    };

    int count = static_cast<int>(sizeof(buttons) / sizeof(buttons[0]));
    for (int i = 0; i < count; i++) {
        float bx = cx + (i % 2) * (btnW + 10.0f);
        float by = cy + (i / 2) * (btnH + 10.0f);
        if (GuiButton({bx, by, btnW, btnH}, buttons[i].label)) {
            applyPreset(buttons[i].preset);
        }
    }
    cy += ((count + 1) / 2) * (btnH + 10.0f) + 5.0f;

    GuiGroupBox({x, top, width, cy - top}, "Preset Transformations");
    return cy;
}

// ---------------------------------------------------------------------------
// Values
// ---------------------------------------------------------------------------

Matrix3D InputPanel::parseMatrix3(const State& s) {
    Matrix3D m;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            m.m[i][j] = parseOrZero(s.mat3[i][j]);
    return m;
}

Matrix4D InputPanel::parseMatrix4(const State& s) {
    Matrix4D m;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            m.m[i][j] = parseOrZero(s.mat4[i][j]);
    return m;
}

Matrix3D InputPanel::getMatrix3D() const { return parseMatrix3(m_committed); }
Matrix4D InputPanel::getMatrix4D() const { return parseMatrix4(m_committed); }

Matrix4D InputPanel::getActiveMatrix4D() const {
    if (m_committed.use4x4) return getMatrix4D();
    Matrix3D m3 = getMatrix3D();
    Matrix4D m4 = Matrix4D::identity();
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            m4.m[i][j] = m3.m[i][j];
    return m4;
}

Vector3D InputPanel::getVectorA() const {
    return {parseOrZero(m_committed.vecA[0]), parseOrZero(m_committed.vecA[1]),
            parseOrZero(m_committed.vecA[2])};
}

Vector3D InputPanel::getVectorB() const {
    return {parseOrZero(m_committed.vecB[0]), parseOrZero(m_committed.vecB[1]),
            parseOrZero(m_committed.vecB[2])};
}

void InputPanel::resetChangedFlags() {
    m_matrixChanged = false;
    m_vectorsChanged = false;
}

// ---------------------------------------------------------------------------
// Save / load
// ---------------------------------------------------------------------------

void InputPanel::saveState(SceneData& data) const {
    putBool(data, "matrix.use4x4", m_committed.use4x4);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            data["matrix3." + std::to_string(i) + std::to_string(j)] = m_committed.mat3[i][j];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            data["matrix4." + std::to_string(i) + std::to_string(j)] = m_committed.mat4[i][j];
    for (int k = 0; k < 3; k++) {
        data["vector_a." + std::to_string(k)] = m_committed.vecA[k];
        data["vector_b." + std::to_string(k)] = m_committed.vecB[k];
    }
}

void InputPanel::loadState(const SceneData& data) {
    State s = m_committed;
    getBool(data, "matrix.use4x4", s.use4x4);

    // Only accept values that parse; keep the current ones otherwise
    auto load = [&](const std::string& key, char* buf) {
        char tmp[kTextSize];
        if (getText(data, key, tmp, sizeof(tmp)) && isValid(tmp)) {
            std::snprintf(buf, kTextSize, "%s", tmp);
        }
    };
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            load("matrix3." + std::to_string(i) + std::to_string(j), s.mat3[i][j]);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            load("matrix4." + std::to_string(i) + std::to_string(j), s.mat4[i][j]);
    for (int k = 0; k < 3; k++) {
        load("vector_a." + std::to_string(k), s.vecA[k]);
        load("vector_b." + std::to_string(k), s.vecB[k]);
    }

    m_activeField = -1;
    m_state = s;
    commit();  // loading is undoable
}

} // namespace linalg
