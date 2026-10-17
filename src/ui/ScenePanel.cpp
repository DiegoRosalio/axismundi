#include "ui/ScenePanel.hpp"
#include "math/Decompositions.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>

namespace linalg {

using scene::SceneObject;
using scene::SpaceKind;

namespace {

constexpr float kPi = 3.14159265f;

std::string fmt(double v) {
    char buf[32];
    if (std::fabs(v) < 5e-5) v = 0;
    snprintf(buf, sizeof(buf), "%.4g", v);
    return buf;
}

// One-line description for the objects list
std::string summaryOf(const SceneObject& o) {
    const std::string& type = o.typeId();
    if (!o.error().empty()) return o.error();
    if (type == "parameter") {
        const auto& p = static_cast<const scene::ParameterObject&>(o);
        const char* how = p.mode() == scene::ParameterObject::Mode::Formula
                              ? "f(t)"
                              : p.mode() == scene::ParameterObject::Mode::Animate ? "animated" : "slider";
        return std::string(how) + " = " + fmt(p.number());
    }
    if (type == "eigen") {
        const scene::Field* f = o.field("of");
        return "eigenvectors of " + (f ? f->text : std::string("?"));
    }
    if (type == "function") return "y = " + o.field("f")->text;
    if (type == "surface") return "z = " + o.field("f")->text;
    if (type == "implicit") return o.field("f")->text + " = 0";
    if (type == "curve") return "curve in u";
    if (type == "product") return o.field("a")->text + " · " + o.field("b")->text + " = " + o.value().summary();
    if (type == "vector" && static_cast<const scene::VectorObject&>(o).byFormula()) {
        return o.field("value")->text + " = " + o.value().summary();
    }
    if (type == "matrix" && static_cast<const scene::MatrixObject&>(o).byFormula()) {
        return o.field("value")->text;
    }
    return o.value().summary();
}

const char* typeLabel(const SceneObject& o) {
    const scene::ObjectTypeInfo* t = scene::findType(o.typeId());
    return t ? t->label : o.typeId().c_str();
}

void note(ui::Column& col, const std::string& text, Color color, ui::FontRole role = ui::FontRole::Small) {
    float h = ui::textWrapped({col.x(), col.y(), col.width(), 0}, text.c_str(), role, color);
    col.space(h / ui::scale() + 4.0f);
}

// Sample functions per plot type, for the "Next example" button
struct PlotExample {
    const char* f;
    const char* x;
    const char* y;
    const char* z;
};

const std::vector<PlotExample>& plotExamples(const std::string& type) {
    static const std::vector<PlotExample> function = {
        {"sin(x)", "", "", ""}, {"x^3/10 - x", "", "", ""}, {"3exp(-x^2/2)", "", "", ""}, {"tan(x)", "", "", ""}};
    static const std::vector<PlotExample> surface = {{"sin(sqrt(x^2 + y^2))", "", "", ""},
                                                     {"(x^2 - y^2)/5", "", "", ""},
                                                     {"3exp(-(x^2 + y^2)/4)", "", "", ""},
                                                     {"sin(x) cos(y)", "", "", ""}};
    static const std::vector<PlotExample> implicit = {{"x^2 + y^2 - 9", "", "", ""},
                                                      {"x^2/16 + y^2/4 - 1", "", "", ""},
                                                      {"y^2 - x^3 + 3x", "", "", ""},
                                                      {"(x^2 + y^2)^2 - 8(x^2 - y^2)", "", "", ""}};
    static const std::vector<PlotExample> curve = {{"", "2cos(u)", "2sin(u)", "u/4"},
                                                   {"", "sin(u) + 2sin(2u)", "cos(u) - 2cos(2u)", "-sin(3u)"},
                                                   {"", "3sin(2u)", "3sin(3u)", "0"},
                                                   {"", "2cos(u + t)", "2sin(u + t)", "sin(3u - 2t)"}};
    if (type == "surface") return surface;
    if (type == "implicit") return implicit;
    if (type == "curve") return curve;
    return function;
}

} // namespace

int ScenePanel::fieldId(const SceneObject& obj, const std::string& key) {
    unsigned h = 2166136261u ^ obj.id();
    for (char c : key) {
        h ^= static_cast<unsigned char>(c);
        h *= 16777619u;
    }
    return static_cast<int>(h & 0x3fffffff);
}

ScenePanelRequests ScenePanel::takeRequests() {
    ScenePanelRequests r = m_requests;
    m_requests = ScenePanelRequests{};
    return r;
}

// ---------------------------------------------------------------------------
// Editing
// ---------------------------------------------------------------------------

void ScenePanel::finishEditing(scene::Scene&) {
    if (m_activeId < 0) return;
    int id = m_activeId;
    m_activeId = -1;
    for (const auto* list : {&m_editables, &m_lastEditables}) {
        for (const auto& e : *list) {
            if (e.id == id) {
                e.commit(m_buffer);
                return;
            }
        }
    }
}

void ScenePanel::update(scene::Scene&) {
    if (m_activeId < 0 || !IsKeyPressed(KEY_TAB) || m_lastEditables.empty()) return;
    bool back = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    size_t index = 0;
    for (size_t i = 0; i < m_lastEditables.size(); i++)
        if (m_lastEditables[i].id == m_activeId) index = i;
    m_lastEditables[index].commit(m_buffer);
    size_t n = m_lastEditables.size();
    size_t next = back ? (index + n - 1) % n : (index + 1) % n;
    m_activeId = m_lastEditables[next].id;
    snprintf(m_buffer, sizeof(m_buffer), "%s", m_lastEditables[next].text.c_str());
}

void ScenePanel::editText(Rectangle r, int id, const std::string& current, bool invalid,
                          std::function<bool(const std::string&)> commit) {
    m_editables.push_back({id, current, commit});
    char tmp[256];
    bool active = (m_activeId == id);
    char* buf = active ? m_buffer : tmp;
    if (!active) snprintf(tmp, sizeof(tmp), "%s", current.c_str());

    bool showInvalid = invalid && !active;
    if (active) {
        // Live syntax check while typing
        expr::Program probe;
        showInvalid = !probe.compile(m_buffer);
    }

    int before = m_activeId;
    bool finished = ui::textField(r, buf, sizeof(m_buffer), id, m_activeId, showInvalid);
    if (!active && m_activeId == id) {
        // Just activated: commit the field that was being edited, then take over
        if (before >= 0 && before != id) {
            for (const auto* list : {&m_editables, &m_lastEditables}) {
                bool done = false;
                for (const auto& e : *list) {
                    if (e.id == before) {
                        e.commit(m_buffer);
                        done = true;
                        break;
                    }
                }
                if (done) break;
            }
        }
        snprintf(m_buffer, sizeof(m_buffer), "%s", tmp);
    }
    if (finished) commit(m_buffer);
}

float ScenePanel::fieldRow(float x, float y, float width, SceneObject& obj, const std::string& key, scene::Scene& sc,
                           float labelWidth) {
    const ui::Theme& t = ui::theme();
    scene::Field* f = obj.field(key);
    if (!f) return y;
    ui::Column col(x, y, width, 6.0f);
    Rectangle r = col.row(34.0f);
    if (labelWidth > 0) {
        Rectangle l = ui::takeLeft(r, labelWidth, 6.0f);
        ui::textIn(l, ui::ellipsize(f->label.c_str(), ui::FontRole::Body, l.width).c_str(), ui::FontRole::Body,
                   t.textMuted);
    }
    int id = fieldId(obj, key);
    scene::ObjectId oid = obj.id();
    SpaceKind kind = m_kind;
    editText(r, id, f->text, !f->error.empty(), [&sc, kind, oid, key](const std::string& text) {
        SceneObject* o = sc.space(kind).find(oid);
        if (!o) return false;
        o->setField(key, text);
        sc.commit();
        return true;
    });
    if (!f->error.empty() && m_activeId != id) {
        col.removeGap();
        col.space(2.0f);
        note(col, f->error, t.danger);
    }
    return col.y();
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

float ScenePanel::draw(float x, float y, float width, scene::Scene& sc, SpaceKind kind, const view::SceneView& view) {
    m_kind = kind;
    m_lastEditables = std::move(m_editables);
    m_editables.clear();

    y = drawObjects(x, y, width, sc, kind);
    y = drawInspector(x, y, width, sc, kind, view);

    // Commit sliders once the mouse is released
    if (m_sliderDirty && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        sc.commit();
        m_sliderDirty = false;
    }
    return y;
}

float ScenePanel::drawObjects(float x, float y, float width, scene::Scene& sc, SpaceKind kind) {
    const ui::Theme& t = ui::theme();
    scene::Space& space = sc.space(kind);
    scene::Selection& sel = sc.selection(kind);
    ui::Column col(x, y, width, 4.0f);

    // Header: section title, undo / redo, add
    Rectangle head = col.row(32.0f);
    Rectangle add = ui::takeRight(head, 74.0f, 6.0f);
    Rectangle redo = ui::takeRight(head, 30.0f, 2.0f);
    Rectangle undo = ui::takeRight(head, 30.0f, 6.0f);
    ui::sectionHeader(head, "Objects", m_showObjects);
    if (ui::iconButton(undo, ui::Icon::Undo, false, sc.canUndo())) {
        m_activeId = -1;
        sc.undo();
    }
    if (ui::iconButton(redo, ui::Icon::Redo, false, sc.canRedo())) {
        m_activeId = -1;
        sc.redo();
    }
    ui::tooltip(undo, "Undo (Ctrl+Z in UI mode)", "Undo");
    ui::tooltip(redo, "Redo (Ctrl+Y in UI mode)", "Redo");
    if (ui::button(add, "Add", ui::ButtonKind::Primary, true, ui::Icon::Plus)) {
        m_requests.openAdd = true;
        m_requests.addAnchor = add;
    }
    ui::tooltip(add, "Add a vector, matrix, plot, parameter... Templates use the selected objects.", "Add object");
    ui::anchor("objects.add", add);
    if (!m_showObjects) return col.y() + ui::px(6.0f);

    Rectangle listArea = {col.x(), col.y(), col.width(), 0};
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    for (const auto& obj : space.objects()) {
        SceneObject& o = *obj;
        Rectangle row = col.row(36.0f);
        listArea = ui::unite(listArea, row);
        Rectangle eye = {row.x + row.width - ui::px(32), row.y + ui::px(3), ui::px(30), ui::px(30)};
        bool selected = sel.contains(o.id());
        bool hoverRow = ui::hovered(row) && !ui::hovered(eye);
        if (hoverRow && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (m_activeId >= 0) finishEditing(sc);
            if (ctrl) sel.toggle(o.id());
            else sel.set(o.id());
        }
        if (selected) ui::fillRounded(row, ui::px(ui::kRadius), sel.primary() == o.id() ? t.accentSoft : Fade(t.accentSoft, 0.5f));
        else if (hoverRow) ui::fillRounded(row, ui::px(ui::kRadius), t.hover);

        float dot = ui::px(9.0f);
        Color c = view::SceneView::colorOf(o);
        if (!o.visible) c = Fade(c, 0.3f);
        DrawCircleV({row.x + ui::px(12), row.y + row.height / 2}, dot / 2, c);
        float tx = row.x + ui::px(24);
        float nameW = std::min(ui::px(64.0f), ui::textWidth(o.name().c_str(), ui::FontRole::BodyStrong) + ui::px(2));
        ui::textIn({tx, row.y, nameW, row.height}, ui::ellipsize(o.name().c_str(), ui::FontRole::BodyStrong, ui::px(64)).c_str(),
                   ui::FontRole::BodyStrong, o.visible ? t.text : t.textFaint);
        float sx = tx + nameW + ui::px(8);
        float sw = eye.x - sx - ui::px(6);
        bool hasError = !o.error().empty();
        std::string summary = ui::ellipsize(summaryOf(o).c_str(), ui::FontRole::Small, sw);
        ui::textIn({sx, row.y, sw, row.height}, summary.c_str(), ui::FontRole::Small, hasError ? t.danger : t.textMuted);

        if (ui::iconButton(eye, ui::Icon::Eye, false, true)) {
            o.visible = !o.visible;
            sc.commit();
        }
        if (!o.visible) {
            // Strike through the eye when hidden
            DrawLineEx({eye.x + ui::px(8), eye.y + eye.height - ui::px(8)}, {eye.x + eye.width - ui::px(8), eye.y + ui::px(8)},
                       ui::px(1.6f), t.textMuted);
        }
        ui::tooltip(eye, o.visible ? "Hide" : "Show");
        if (hasError) ui::tooltip(row, o.error().c_str(), o.name().c_str());
    }
    if (space.objects().empty()) {
        note(col, "Nothing here yet. Use Add to create a vector, a matrix, a plot...", t.textMuted);
    }
    ui::anchor("objects.list", listArea);
    col.space(8.0f);
    return col.y();
}

float ScenePanel::drawInspector(float x, float y, float width, scene::Scene& sc, SpaceKind kind,
                                const view::SceneView& view) {
    const ui::Theme& t = ui::theme();
    scene::Space& space = sc.space(kind);
    scene::Selection& sel = sc.selection(kind);
    SceneObject* obj = space.find(sel.primary());
    ui::Column col(x, y, width, 6.0f);

    std::string title = obj ? std::string("Inspector · ") + obj->name() : std::string("Inspector");
    Rectangle headRow = col.row(26.0f);
    ui::sectionHeader(headRow, title.c_str(), m_showInspector, nullptr, true);
    Rectangle inspectorArea = headRow;
    if (!m_showInspector) return col.y() + ui::px(6.0f);
    if (!obj) {
        note(col, "Select an object in the list (or click it in the view) to edit it here.", t.textMuted);
        col.space(6.0f);
        return col.y();
    }
    SceneObject& o = *obj;
    const std::string& type = o.typeId();
    scene::ObjectId oid = o.id();

    // Type, name and color
    ui::textIn(col.row(16), ui::upper(typeLabel(o)).c_str(), ui::FontRole::Caption, t.accent);
    Rectangle nameRow = col.row(34.0f);
    Rectangle swatch = ui::takeRight(nameRow, 34.0f, 8.0f);
    Rectangle nameLabel = ui::takeLeft(nameRow, 52.0f, 6.0f);
    ui::textIn(nameLabel, "Name", ui::FontRole::Body, t.textMuted);
    editText(nameRow, fieldId(o, "#name"), o.name(), false, [this, &sc, kind, oid](const std::string& text) {
        std::string error;
        if (!sc.space(kind).rename(oid, text, error)) {
            m_requests.message = "Cannot rename: " + error;
            m_requests.messageIsError = true;
            return false;
        }
        sc.commit();
        return true;
    });
    ui::anchor("inspector.name", nameRow);
    Rectangle sw = {swatch.x + ui::px(4), swatch.y + ui::px(4), swatch.width - ui::px(8), swatch.height - ui::px(8)};
    if (type != "parameter" && ui::swatch(sw, view::SceneView::colorOf(o), o.customColor)) {
        m_requests.openColor = true;
        m_requests.colorAnchor = sw;
    }
    if (type != "parameter") ui::tooltip(sw, "Change the color", "Color");
    col.space(2.0f);

    auto commit = [&sc]() { sc.commit(); };

    // --- Type-specific content ------------------------------------------------
    if (type == "vector") {
        auto& v = static_cast<scene::VectorObject&>(o);
        int mode = v.byFormula() ? 1 : 0;
        if (ui::segmented(col.row(32), {"Components", "Formula"}, mode,
                          {"Type the x, y and z components.",
                           "Define it from other objects, e.g. a + b, cross(a, b), M*a, 2*normalize(a)."})) {
            m_activeId = -1;
            v.setByFormula(mode == 1);
            if (mode == 1) v.setField("value", "[" + v.field("x")->text + ", " + v.field("y")->text + ", " +
                                                    v.field("z")->text + "]");
            commit();
        }
        if (!v.byFormula()) {
            Rectangle row = col.row(34.0f);
            auto cells = ui::splitRow(row, {1, 1, 1}, 6.0f);
            const char* keys[3] = {"x", "y", "z"};
            for (int k = 0; k < 3; k++) {
                scene::Field* f = v.field(keys[k]);
                int id = fieldId(v, keys[k]);
                std::string key = keys[k];
                editText(cells[k], id, f->text, !f->error.empty(), [&sc, kind, oid, key](const std::string& text) {
                    SceneObject* ob = sc.space(kind).find(oid);
                    if (!ob) return false;
                    ob->setField(key, text);
                    sc.commit();
                    return true;
                });
                ui::tooltip(cells[k], (std::string(keys[k]) + " component: a number or a formula such as cos(theta)").c_str());
            }
            for (int k = 0; k < 3; k++) {
                if (!v.field(keys[k])->error.empty()) note(col, std::string(keys[k]) + ": " + v.field(keys[k])->error, t.danger);
            }
        } else {
            col.setY(fieldRow(col.x(), col.y(), col.width(), v, "value", sc, 0.0f));
        }
        col.setY(fieldRow(col.x(), col.y(), col.width(), v, "tail", sc));
        if (v.value().type == expr::Type::Vector) {
            Vector3D val = v.value().vector;
            char buf[96];
            snprintf(buf, sizeof(buf), "|%s| = %.3f", v.name().c_str(), val.magnitude());
            ui::textIn(col.row(18), buf, ui::FontRole::Mono, t.textMuted);
        }
    } else if (type == "matrix") {
        auto& m = static_cast<scene::MatrixObject&>(o);
        int mode = m.byFormula() ? 1 : 0;
        if (ui::segmented(col.row(32), {"Cells", "Formula"}, mode,
                          {"Type each entry. Entries may use parameters, e.g. cos(theta).",
                           "One formula: rotz(theta), scale(2), inv(A), A*B, [a, b, cross(a, b)] (columns)..."})) {
            m_activeId = -1;
            m.setByFormula(mode == 1);
            commit();
        }
        if (!m.byFormula()) {
            Rectangle top = col.row(32.0f);
            top.width = std::min(top.width, ui::px(150.0f));
            int size = m.size() == 4 ? 1 : 0;
            if (ui::segmented(top, {"3×3", "4×4"}, size,
                              {"A linear map of 3D space.",
                               "Adds a fourth row and column (homogeneous coordinates); the last column translates."})) {
                m_activeId = -1;
                m.setSize(size == 1 ? 4 : 3);
                commit();
            }
            int n = m.size();
            Rectangle grid = {col.x(), col.y(), col.width(), 0};
            for (int i = 0; i < n; i++) {
                Rectangle row = col.row(n == 4 ? 32.0f : 36.0f);
                grid = ui::unite(grid, row);
                auto cells = n == 4 ? ui::splitRow(row, {1, 1, 1, 1}, 6.0f) : ui::splitRow(row, {1, 1, 1}, 6.0f);
                for (int j = 0; j < n; j++) {
                    std::string key = scene::MatrixObject::cellKey(i, j);
                    scene::Field* f = m.field(key);
                    editText(cells[j], fieldId(m, key), f->text, !f->error.empty(), [&sc, kind, oid, key](const std::string& text) {
                        SceneObject* ob = sc.space(kind).find(oid);
                        if (!ob) return false;
                        ob->setField(key, text);
                        sc.commit();
                        return true;
                    });
                    static const char* columnTips[4] = {
                        "Column 1 is where M sends the x-axis vector (1, 0, 0).",
                        "Column 2 is where M sends the y-axis vector (0, 1, 0).",
                        "Column 3 is where M sends the z-axis vector (0, 0, 1).",
                        "Column 4 is the translation (4×4 only).",
                    };
                    if (m_activeId != fieldId(m, key)) ui::tooltip(cells[j], columnTips[j]);
                }
            }
            ui::anchor("matrix.grid", grid);
            if (!m.error().empty()) note(col, m.error(), t.danger);

            // Presets
            struct Preset { const char* label; const char* key; Matrix3D value; const char* tip; };
            const Preset presets[] = {
                {"Identity", "I", Matrix3D::identity(), "Leaves every vector where it is. det = 1."},
                {"Rotate 45°", "R", Matrix3D::rotation(kPi / 4), "45° about the z-axis. Lengths and angles are kept (det = 1)."},
                {"Scale 2×", "S", Matrix3D::scale(2, 2), "Doubles x and y, keeps z (det = 4)."},
                {"Shear", "X", Matrix3D::shear(0.5f, 0), "x' = x + 0.5 y: squares become parallelograms (det = 1)."},
                {"Reflect", "F", Matrix3D::scale(-1, 1), "x' = −x: a mirror image (det = −1)."},
            };
            col.space(2.0f);
            Rectangle presetArea = {col.x(), col.y(), col.width(), 0};
            for (int i = 0; i < 5; i += 2) {
                Rectangle row = col.row(30.0f);
                presetArea = ui::unite(presetArea, row);
                auto cells = ui::splitRow(row, {1, 1}, 6.0f);
                for (int k = 0; k < 2 && i + k < 5; k++) {
                    const Preset& p = presets[i + k];
                    if (ui::button(cells[k], p.label, ui::ButtonKind::Secondary, true, ui::Icon::None, p.key)) {
                        m_activeId = -1;
                        m.setSize(3);
                        for (int r = 0; r < 3; r++)
                            for (int c = 0; c < 3; c++) {
                                char buf[32];
                                formatNumber(buf, sizeof(buf), p.value.m[r][c]);
                                m.setField(scene::MatrixObject::cellKey(r, c), buf);
                            }
                        commit();
                    }
                    ui::tooltip(cells[k], p.tip, p.label);
                    if (i + k == 1) ui::anchor("preset.rotate", cells[k]);
                    if (i + k == 4) ui::anchor("preset.reflect", cells[k]);
                }
            }
            ui::anchor("presets", presetArea);
        } else {
            col.setY(fieldRow(col.x(), col.y(), col.width(), m, "value", sc, 0.0f));
            note(col, "Try rotz(theta), rot([1,1,1], pi/3), scale(1, 2, 1), translate(1, 0, 0), inv(M), A*B, "
                      "or [u, v, w] to use vectors as columns.", t.textFaint);
        }
        if (m.value().type == expr::Type::Matrix) {
            float det = m.value().size == 4 ? m.value().matrix.determinant() : m.value().matrix3().determinant();
            char buf[64];
            snprintf(buf, sizeof(buf), "det = %.4g", det);
            ui::textIn(col.row(18), buf, ui::FontRole::Mono, t.textMuted);
        }
    } else if (type == "eigen") {
        col.setY(fieldRow(col.x(), col.y(), col.width(), o, "of", sc));
        note(col, "Dashed lines show the directions this matrix only stretches; the dots mark λv.", t.textMuted);
    } else if (dynamic_cast<scene::PlotObject*>(&o)) {
        auto& p = static_cast<scene::PlotObject&>(o);
        bool curve = (type == "curve");
        if (curve) {
            for (const char* k : {"x", "y", "z"}) col.setY(fieldRow(col.x(), col.y(), col.width(), p, k, sc));
        } else {
            col.setY(fieldRow(col.x(), col.y(), col.width(), p, "f", sc));
            ui::anchor("plot.expr", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
        }
        auto rangeRow = [&](const char* lo, const char* hi, const char* label) {
            Rectangle row = col.row(34.0f);
            Rectangle l = ui::takeLeft(row, 34.0f, 6.0f);
            ui::textIn(l, label, ui::FontRole::Body, t.textMuted);
            auto cells = ui::splitRow(row, {1, 1}, 26.0f);
            const char* keys[2] = {lo, hi};
            for (int k = 0; k < 2; k++) {
                std::string key = keys[k];
                scene::Field* f = p.field(key);
                editText(cells[k], fieldId(p, key), f->text, !f->error.empty(), [&sc, kind, oid, key](const std::string& text) {
                    SceneObject* ob = sc.space(kind).find(oid);
                    if (!ob) return false;
                    ob->setField(key, text);
                    sc.commit();
                    return true;
                });
            }
            ui::textIn({cells[0].x + cells[0].width, row.y, ui::px(26), row.height}, "to", ui::FontRole::Small, t.textMuted,
                       ui::Align::Center);
        };
        if (curve) rangeRow("umin", "umax", "u");
        else {
            rangeRow("xmin", "xmax", "x");
            if (type != "function") rangeRow("ymin", "ymax", "y");
        }

        Rectangle row = col.row(30.0f);
        Rectangle label = ui::takeLeft(row, 52.0f, 8.0f);
        Rectangle value = ui::takeRight(row, 64.0f, 8.0f);
        ui::textIn(label, "Detail", ui::FontRole::Body, t.text);
        auto dr = scene::PlotObject::densityRange(p.kind());
        float density = static_cast<float>(p.density());
        if (ui::slider(row, density, static_cast<float>(dr.min), static_cast<float>(dr.max))) {
            p.setDensity(density < 10 ? std::round(density * 2) / 2.0 : std::round(static_cast<double>(density)));
            m_sliderDirty = true;
        }
        ui::anchor("plot.detail", row);
        bool grid2D = type == "surface" || type == "implicit";
        char dbuf[48];
        snprintf(dbuf, sizeof(dbuf), "%g/unit", p.density());
        ui::textIn(value, dbuf, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
        ui::tooltip({label.x, row.y, value.x + value.width - label.x, row.height},
                    grid2D ? "Grid cells per unit of x and y. A wider range keeps the same smoothness."
                           : (curve ? "Samples per unit of t. A longer range keeps the same smoothness."
                                    : "Samples per unit of x. A wider range keeps the same smoothness."),
                    "Detail");
        scene::PlotObject::Samples sm = p.samples();
        char sbuf[128];
        if (grid2D) {
            if (sm.capped)
                snprintf(sbuf, sizeof(sbuf), "%d × %d cells (capped from %d × %d for speed)", sm.nx, sm.ny, sm.wantedX,
                         sm.wantedY);
            else
                snprintf(sbuf, sizeof(sbuf), "%d × %d cells", sm.nx, sm.ny);
        } else {
            if (sm.capped) snprintf(sbuf, sizeof(sbuf), "%d samples (capped from %d for speed)", sm.nx, sm.wantedX);
            else snprintf(sbuf, sizeof(sbuf), "%d samples", sm.nx);
        }
        note(col, sbuf, sm.capped ? t.warning : t.textFaint);
        bool heat = p.heatmap();
        if (ui::toggleRow(col.row(30), "Heat map", heat)) {
            p.setProp("heatmap", heat);
            commit();
        }
        if (ui::button(col.row(30), "Next example", ui::ButtonKind::Secondary, true, ui::Icon::Gallery)) {
            m_activeId = -1;
            const auto& list = plotExamples(type);
            m_exampleIndex = (m_exampleIndex + 1) % static_cast<int>(list.size());
            const PlotExample& e = list[m_exampleIndex];
            if (curve) {
                p.setField("x", e.x);
                p.setField("y", e.y);
                p.setField("z", e.z);
            } else {
                p.setField("f", e.f);
            }
            commit();
        }
        const PlotRenderer* r = view.plot(p.id());
        if (!p.error().empty()) note(col, p.error(), t.danger);
        else if (r && !r->error().empty()) note(col, r->error(), t.danger);
        else if (r) {
            char buf[96];
            snprintf(buf, sizeof(buf), "Values range over [%.2f, %.2f]", r->minValue(), r->maxValue());
            note(col, buf, t.textMuted);
        }
        note(col, curve ? "u runs along the curve; t is the scene time. Parameters work by name too."
                        : "Use x, y, t (the scene time) and parameters by name, e.g. a*sin(x - t).",
             t.textFaint);
    } else if (type == "parameter") {
        auto& p = static_cast<scene::ParameterObject&>(o);
        using PMode = scene::ParameterObject::Mode;
        Rectangle modeRow = col.row(30.0f);
        int m = static_cast<int>(p.mode());
        if (ui::segmented(modeRow, {"Slider", "Animate", "Formula"}, m,
                          {"Set the value by hand. It stays where you put it.",
                           "Follows the scene clock from min to max, with a repeat style and an easing curve.",
                           "A formula of t, the scene time in seconds, e.g. 2pi*t/5 or sin(t)."})) {
            p.setMode(static_cast<PMode>(m));
            commit();
        }
        ui::anchor("parameter.mode", modeRow);

        if (p.mode() == PMode::Formula) {
            col.setY(fieldRow(col.x(), col.y(), col.width(), o, "formula", sc, 64.0f));
            ui::anchor("parameter.formula", {col.x(), col.y() - ui::px(40), col.width(), ui::px(36)});
        }
        if (p.mode() == PMode::Slider) {
            Rectangle row = col.row(30.0f);
            Rectangle value = ui::takeRight(row, 56.0f, 8.0f);
            float v = static_cast<float>(p.number());
            if (ui::slider(row, v, static_cast<float>(p.minimum()), static_cast<float>(p.maximum()))) {
                p.setNumber(v);
                m_sliderDirty = true;
            }
            ui::anchor("parameter.slider", row);
            ui::textIn(value, fmt(p.number()).c_str(), ui::FontRole::Mono, t.text, ui::Align::Right);
        }
        if (p.mode() != PMode::Formula) {
            Rectangle range = col.row(34.0f);
            Rectangle l = ui::takeLeft(range, 52.0f, 6.0f);
            ui::textIn(l, "Range", ui::FontRole::Body, t.textMuted);
            auto cells = ui::splitRow(range, {1, 1}, 26.0f);
            const char* keys[2] = {"min", "max"};
            for (int k = 0; k < 2; k++) {
                std::string key = keys[k];
                editText(cells[k], fieldId(p, std::string("#") + key), fmt(p.propNumber(key)), false,
                         [&sc, kind, oid, key](const std::string& text) {
                             float val;
                             auto* ob = dynamic_cast<scene::ParameterObject*>(sc.space(kind).find(oid));
                             if (!ob || !parseNumber(text.c_str(), val)) return false;
                             ob->setProp(key, static_cast<double>(val));
                             ob->setNumber(ob->number());
                             sc.commit();
                             return true;
                         });
            }
            ui::textIn({cells[0].x + cells[0].width, range.y, ui::px(26), range.height}, "to", ui::FontRole::Small,
                       t.textMuted, ui::Align::Center);
        }

        if (p.mode() == PMode::Animate) {
            Playback pb = p.playback();
            bool changed = false;
            changed |= ui::repeatControl(col.row(30.0f), pb.repeat);
            Rectangle row = col.row(32.0f);
            Rectangle easeRect = ui::takeLeft(row, 150.0f, 10.0f);
            if (ui::easingButton(easeRect, pb.easing, m_easingOpen)) m_easingOpen = !m_easingOpen;
            ui::anchor("parameter.easing", easeRect);
            Rectangle durValue = ui::takeRight(row, 44.0f, 6.0f);
            float dur = pb.duration;
            if (ui::slider(row, dur, 0.5f, 30.0f)) {
                pb.duration = std::round(dur * 10.0f) / 10.0f;
                changed = true;
                m_sliderDirty = true;
            }
            ui::tooltip(row, "Seconds for one run from min to max.", "Duration");
            char buf[32];
            snprintf(buf, sizeof(buf), "%.1fs", pb.duration);
            ui::textIn(durValue, buf, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
            if (m_easingOpen) {
                bool picked = false;
                float h = ui::easingGrid(col.x(), col.y(), col.width(), pb.easing, picked);
                col.space(h / ui::scale() + 6.0f);
                changed |= picked;
            }
            if (changed) {
                p.setPlayback(pb);
                if (!m_sliderDirty) commit();
            }
            // Start delay, so several parameters can run one after another
            Rectangle startRow = col.row(34.0f);
            Rectangle sl = ui::takeLeft(startRow, 64.0f, 6.0f);
            ui::textIn(sl, "Start at", ui::FontRole::Body, t.textMuted);
            Rectangle sf = ui::takeLeft(startRow, 80.0f, 6.0f);
            editText(sf, fieldId(p, "#start"), fmt(p.startDelay()), false, [&sc, kind, oid](const std::string& text) {
                float val;
                auto* ob = dynamic_cast<scene::ParameterObject*>(sc.space(kind).find(oid));
                if (!ob || !parseNumber(text.c_str(), val) || val < 0.0f) return false;
                ob->setProp("anim.start", static_cast<double>(val));
                sc.commit();
                return true;
            });
            ui::tooltip(sf, "Seconds of scene time before this parameter starts moving.", "Start delay");
            ui::textIn(startRow, "s", ui::FontRole::Small, t.textMuted);
        }

        if (p.timeDriven()) {
            // Current value and the link to the clock that drives it
            const scene::Timeline& tl = sc.timeline();
            char buf[96];
            snprintf(buf, sizeof(buf), "t = %.2f s     %s = %s", tl.time, p.name().c_str(), fmt(p.number()).c_str());
            ui::textIn(col.row(18.0f), buf, ui::FontRole::Mono, t.textMuted);
            Rectangle link = col.row(30.0f);
            if (ui::button(link, tl.playing ? "Follows the Time panel (playing)" : "Follows the Time panel (paused)",
                           ui::ButtonKind::Secondary, true, tl.playing ? ui::Icon::Pause : ui::Icon::Play)) {
                m_requests.openTime = true;
            }
            ui::anchor("parameter.time", link);
            ui::tooltip(link, "Time-driven parameters follow the scene clock. Play, pause and scrub it in the Time "
                              "panel (P plays or pauses).", "Scene clock");
        }
        note(col, "Use " + p.name() + " in any formula, e.g. rotz(" + p.name() + ") or lerp(eye(), M, " + p.name() +
                      ").",
             t.textFaint);
    } else if (type == "complex") {
        col.setY(fieldRow(col.x(), col.y(), col.width(), o, "value", sc, 52.0f));
        const expr::Value& v = o.value();
        if (v.isNumber()) {
            double r = std::hypot(v.re, v.im), a = std::atan2(v.im, v.re) * 180.0 / kPi;
            if (std::fabs(a) < 0.05) a = 0.0;  // no "-0.0°"
            char buf[96];
            snprintf(buf, sizeof(buf), "|%s| = %.3f   arg = %.1f°", o.name().c_str(), r, a);
            ui::textIn(col.row(18), buf, ui::FontRole::Mono, t.textMuted);
        }
        struct Toggle { const char* label; const char* key; const char* tip; };
        const Toggle toggles[] = {
            {"Circle through it", "circle", "The circle of all numbers with the same length"},
            {"cos / sin projections", "projections", "Its shadows on the real and imaginary axes"},
            {"Helix of e^(it)", "helix", "The path r·e^(it) for t from 0 to its angle, rising along Z"},
        };
        for (const auto& tg : toggles) {
            Rectangle r = col.row(30.0f);
            bool on = o.propBool(tg.key);
            if (ui::toggleRow(r, tg.label, on)) {
                o.setProp(tg.key, on);
                commit();
            }
            ui::tooltip(r, tg.tip);
        }
    } else if (type == "product") {
        col.setY(fieldRow(col.x(), col.y(), col.width(), o, "a", sc, 104.0f));
        col.setY(fieldRow(col.x(), col.y(), col.width(), o, "b", sc, 104.0f));
        const scene::Field* fa = o.field("a");
        const scene::Field* fb = o.field("b");
        if (o.value().isNumber() && fa->value.isNumber() && fb->value.isNumber()) {
            double ra = std::hypot(fa->value.re, fa->value.im), rb = std::hypot(fb->value.re, fb->value.im);
            double aa = std::atan2(fa->value.im, fa->value.re) * 180 / kPi, ab = std::atan2(fb->value.im, fb->value.re) * 180 / kPi;
            char buf[128];
            snprintf(buf, sizeof(buf), "length %.2f × %.2f = %.2f", ra, rb, ra * rb);
            ui::textIn(col.row(18), buf, ui::FontRole::Mono, t.textMuted);
            snprintf(buf, sizeof(buf), "angle %.1f° + %.1f° = %.1f°", aa, ab, aa + ab);
            ui::textIn(col.row(18), buf, ui::FontRole::Mono, t.textMuted);
        }
        bool angles = o.propBool("angles", true);
        if (ui::toggleRow(col.row(30), "Show rotation and scaling", angles)) {
            o.setProp("angles", angles);
            commit();
        }
    }

    if (!o.error().empty() && type != "matrix" && !dynamic_cast<scene::PlotObject*>(&o)) {
        bool shown = false;
        for (const auto& f : o.fields()) {
            if (!f.error.empty()) shown = true;
        }
        if (!shown) note(col, o.error(), t.danger);
    }

    // Duplicate / delete
    col.space(4.0f);
    auto buttons = ui::splitRow(col.row(30.0f), {1, 1}, 8.0f);
    if (ui::button(buttons[0], "Duplicate", ui::ButtonKind::Secondary, true, ui::Icon::Plus, "Ctrl+D")) {
        m_activeId = -1;
        if (SceneObject* copy = space.duplicate(oid)) {
            sel.set(copy->id());
            sc.commit();
        }
    }
    if (ui::button(buttons[1], "Delete", ui::ButtonKind::Danger, true, ui::Icon::Close, "Del")) {
        m_activeId = -1;
        space.remove(oid);
        sel.remove(oid);
        sc.commit();
    }
    inspectorArea = ui::unite(inspectorArea, buttons[1]);
    ui::anchor("inspector", inspectorArea);
    col.space(8.0f);
    return col.y();
}

} // namespace linalg
