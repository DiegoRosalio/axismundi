#include "ui/InputPanel.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
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

void InputPanel::cancelEditing() {
    if (m_activeField < 0) return;
    char* text = fieldText(m_state, m_activeField);
    std::snprintf(text, kTextSize, "%s", fieldText(m_committed, m_activeField));
    m_activeField = -1;
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

void InputPanel::setMatrix3(const Matrix3D& m) {
    m_activeField = -1;
    m_state = m_committed;
    m_state.use4x4 = false;
    setMatrix3(m_state, m);
    commit();
}

void InputPanel::setMatrix4(const Matrix4D& m) {
    m_activeField = -1;
    m_state = m_committed;
    m_state.use4x4 = true;
    setMatrix4(m_state, m);
    commit();
}

void InputPanel::setVectors(const Vector3D& a, const Vector3D& b) {
    m_activeField = -1;
    m_state = m_committed;
    const float av[3] = {a.x, a.y, a.z};
    const float bv[3] = {b.x, b.y, b.z};
    for (int i = 0; i < 3; i++) {
        formatNumber(m_state.vecA[i], kTextSize, av[i]);
        formatNumber(m_state.vecB[i], kTextSize, bv[i]);
    }
    commit();
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

float InputPanel::draw(float x, float y, float width) {
    int previous = m_activeField;
    y = drawMatrixGroup(x, y, width);
    y = drawVectorGroup(x, y, width);
    y = drawPresetGroup(x, y, width);

    // Clicking straight into another field can move focus before the old
    // field sees its "finished" event; make sure it gets validated.
    if (previous >= 0 && m_activeField != previous) finishField(previous);
    return y;
}

float InputPanel::drawMatrixGroup(float x, float y, float width) {
    ui::Column col(x, y, width);
    if (ui::sectionHeader(col.row(24), "Matrix", m_showMatrix)) {}
    if (!m_showMatrix) return col.y() + ui::px(6.0f);

    // 3x3 / 4x4 switch and undo / redo
    Rectangle top = col.row(ui::kRowHeight);
    Rectangle redoRect = ui::takeRight(top, ui::kRowHeight, 4.0f);
    Rectangle undoRect = ui::takeRight(top, ui::kRowHeight, 8.0f);
    top.width = std::min(top.width, ui::px(150.0f));
    int size = m_state.use4x4 ? 1 : 0;
    if (ui::segmented(top, {"3×3", "4×4"}, size,
                      {"A linear map of 3D space: rotations, scales, shears, reflections, projections.",
                       "Adds a fourth row and column (homogeneous coordinates) so the map can also "
                       "translate. The last column holds the translation."})) {
        m_activeField = -1;
        m_state = m_committed;
        m_state.use4x4 = (size == 1);
        commit();
    }
    if (ui::iconButton(undoRect, ui::Icon::Undo, false, canUndo())) undo();
    if (ui::iconButton(redoRect, ui::Icon::Redo, false, canRedo())) redo();
    ui::tooltip(undoRect, "Undo the last change (Ctrl+Z in UI mode)", "Undo");
    ui::tooltip(redoRect, "Redo (Ctrl+Y in UI mode)", "Redo");
    col.space(4.0f);

    // Cells
    int n = m_state.use4x4 ? 4 : 3;
    float cellH = m_state.use4x4 ? 32.0f : 36.0f;
    for (int i = 0; i < n; i++) {
        Rectangle row = col.row(cellH);
        auto cells = n == 4 ? ui::splitRow(row, {1, 1, 1, 1}, 6.0f) : ui::splitRow(row, {1, 1, 1}, 6.0f);
        for (int j = 0; j < n; j++) {
            int id = i * n + j;
            char* text = fieldText(m_state, id);
            if (ui::textField(cells[j], text, kTextSize, id, m_activeField, !isValid(text))) finishField(id);
            if (m_activeField != id) {
                static const char* columnTips[4] = {
                    ("Column 1 is where M sends the x-axis vector (1, 0, 0). Type a number or an expression "
                     "such as cos(pi/4)."),
                    ("Column 2 is where M sends the y-axis vector (0, 1, 0). Type a number or an expression "
                     "such as cos(pi/4)."),
                    ("Column 3 is where M sends the z-axis vector (0, 0, 1). Type a number or an expression "
                     "such as cos(pi/4)."),
                    "Column 4 is the translation (4×4 only): it moves every point by this amount.",
                };
                ui::tooltip(cells[j], columnTips[j]);
            }
        }
    }
    col.space(10.0f);
    return col.y();
}

float InputPanel::drawVectorGroup(float x, float y, float width) {
    const ui::Theme& t = ui::theme();
    ui::Column col(x, y, width);
    ui::sectionHeader(col.row(24), "Vectors", m_showVectors);
    if (!m_showVectors) return col.y() + ui::px(6.0f);

    // Column headers
    Rectangle head = col.row(16);
    ui::takeLeft(head, 28.0f, 6.0f);
    auto heads = ui::splitRow(head, {1, 1, 1}, 6.0f);
    const char* axes[3] = {"x", "y", "z"};
    for (int k = 0; k < 3; k++) ui::textIn(heads[k], axes[k], ui::FontRole::Small, t.textMuted, ui::Align::Center);

    const char* names[2] = {"A", "B"};
    const Color colors[2] = {m_colorA, m_colorB};
    for (int v = 0; v < 2; v++) {
        Rectangle row = col.row(36.0f);
        Rectangle label = ui::takeLeft(row, 28.0f, 6.0f);
        float dot = ui::px(8.0f);
        DrawCircleV({label.x + dot / 2 + ui::px(2), label.y + label.height / 2}, dot / 2, colors[v]);
        ui::textIn({label.x + dot + ui::px(6), label.y, label.width, label.height}, names[v],
                   ui::FontRole::BodyStrong, t.text);
        auto cells = ui::splitRow(row, {1, 1, 1}, 6.0f);
        for (int k = 0; k < 3; k++) {
            int id = kVectorFieldBase + v * 3 + k;
            char* text = fieldText(m_state, id);
            if (ui::textField(cells[k], text, kTextSize, id, m_activeField, !isValid(text))) finishField(id);
        }
    }
    col.space(10.0f);
    return col.y();
}

float InputPanel::drawPresetGroup(float x, float y, float width) {
    ui::Column col(x, y, width);
    ui::sectionHeader(col.row(24), "Presets", m_showPresets);
    if (!m_showPresets) return col.y() + ui::px(6.0f);

    struct Button { const char* label; const char* key; MatrixPreset preset; };
    const Button buttons[] = {
        {"Identity", "I", MatrixPreset::Identity},
        {"Rotate 45°", "R", MatrixPreset::Rotate45Z},
        {"Scale 2×", "S", MatrixPreset::Scale2},
        {"Shear", "X", MatrixPreset::ShearX},
        {"Reflect", "F", MatrixPreset::ReflectX},
    };
    static const char* tips[] = {
        "Leaves every vector where it is: I v = v. det = 1.",
        ("Turns the xy-plane by θ = 45° about the z-axis: x' = x cos θ − y sin θ, y' = x sin θ + y cos θ. "
         "Lengths and angles are kept (det = 1)."),
        "Doubles x and y and keeps z: areas in the xy-plane grow 4× (det = 4).",
        "x' = x + 0.5 y: squares lean into parallelograms of the same area (det = 1).",
        "x' = −x: a mirror image across the y-z plane. det = −1, so orientation flips.",
    };
    const int count = static_cast<int>(sizeof(buttons) / sizeof(buttons[0]));
    for (int i = 0; i < count; i += 2) {
        auto cells = ui::splitRow(col.row(ui::kRowHeight), {1, 1}, 8.0f);
        for (int k = 0; k < 2 && i + k < count; k++) {
            const Button& b = buttons[i + k];
            if (ui::button(cells[k], b.label, ui::ButtonKind::Secondary, true, ui::Icon::None, b.key)) {
                applyPreset(b.preset);
            }
            ui::tooltip(cells[k], tips[i + k], b.label);
        }
    }
    col.space(10.0f);
    return col.y();
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
