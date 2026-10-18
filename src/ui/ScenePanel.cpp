#include "ui/ScenePanel.hpp"
#include "math/VectorField.hpp"
#include "math/Decompositions.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/TextEditor.hpp"
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
    if (type == "field") return "F = " + o.field("F")->text;
    if (type == "doubleintegral" || type == "lineintegral") return "= " + o.value().summary();
    if (type == "fourierseries") return "N = " + std::to_string(static_cast<const scene::FourierSeriesObject&>(o).terms()) + " terms";
    if (type == "epicycles") return std::to_string(static_cast<const scene::EpicyclesObject&>(o).circles()) + " circles";
    if (type == "frame") {
        char buf[64];
        snprintf(buf, sizeof(buf), "κ = %.3g at u = %.3g", static_cast<const scene::FrameObject&>(o).frame().kappa,
                 static_cast<const scene::FrameObject&>(o).u0());
        return buf;
    }
    if (type == "quaternion") {
        expr::Rotation r;
        if (!expr::rotationOf(o.value(), r)) return o.value().summary();
        char buf[64];
        snprintf(buf, sizeof(buf), "%.0f° rotation", r.angle * 180.0 / 3.14159265358979);
        return buf;
    }
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
    // Short fields (cells, names, ranges): one line, the text is selected on focus
    editField(r, id, current, invalid, std::move(commit), scene::kNoObject, std::string(), true);
}

void ScenePanel::editFormula(Rectangle r, int id, const std::string& current, bool invalid,
                             std::function<bool(const std::string&)> commit, scene::ObjectId object,
                             const std::string& key) {
    editField(r, id, current, invalid, std::move(commit), object, key, false);
}

void ScenePanel::editField(Rectangle r, int id, const std::string& current, bool invalid,
                           std::function<bool(const std::string&)> commit, scene::ObjectId object,
                           const std::string& key, bool shortField) {
    m_editables.push_back({id, current, commit});
    bool active = (m_activeId == id);
    std::string text = active ? std::string(m_buffer) : current;
    bool showInvalid = invalid && !active;
    if (active) {
        expr::Program probe;  // live syntax check while typing
        showInvalid = !probe.compile(text);
    }
    ui::EditorEvent ev = ui::textEditor(r, id, text, active, showInvalid, ui::inlineEditorState(), shortField);
    if (ev == ui::EditorEvent::Activated) {
        // Commit the field that was being edited, then take over
        int before = m_activeId;
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
        m_activeId = id;
        snprintf(m_buffer, sizeof(m_buffer), "%s", current.c_str());
        return;
    }
    if (!active) return;
    snprintf(m_buffer, sizeof(m_buffer), "%s", text.c_str());
    if (ev == ui::EditorEvent::Commit) {
        m_activeId = -1;
        commit(m_buffer);
    } else if (ev == ui::EditorEvent::Expand && !key.empty()) {
        // The big editor takes over the unsaved text
        m_activeId = -1;
        m_requests.openFormula = true;
        m_requests.formulaObject = object;
        m_requests.formulaKey = key;
        m_requests.formulaText = m_buffer;
        m_requests.formulaCommit = commit;
    }
}

float ScenePanel::fieldRow(float x, float y, float width, SceneObject& obj, const std::string& key, scene::Scene& sc,
                           float labelWidth) {
    const ui::Theme& t = ui::theme();
    scene::Field* f = obj.field(key);
    if (!f) return y;
    ui::Column col(x, y, width, 6.0f);
    int id = fieldId(obj, key);
    // The field grows to up to 4 lines for long formulas (measured on the text shown)
    float fieldW = width - (labelWidth > 0 ? ui::px(labelWidth + 6.0f) : 0.0f) - ui::px(30.0f);
    std::string shown = m_activeId == id ? std::string(m_buffer) : f->text;
    float h = std::max(ui::px(34.0f), ui::editorHeight(shown, fieldW, 4));
    Rectangle r = col.row(h / ui::scale());
    if (labelWidth > 0) {
        Rectangle l = ui::takeLeft(r, labelWidth, 6.0f);
        l.height = ui::px(34.0f);
        ui::textIn(l, ui::ellipsize(f->label.c_str(), ui::FontRole::Body, l.width).c_str(), ui::FontRole::Body,
                   t.textMuted);
    }
    Rectangle expand = ui::takeRight(r, 26.0f, 4.0f);
    expand.height = ui::px(26.0f);
    expand.y = r.y + ui::px(4.0f);
    scene::ObjectId oid = obj.id();
    SpaceKind kind = m_kind;
    auto commit = [&sc, kind, oid, key](const std::string& text) {
        SceneObject* o = sc.space(kind).find(oid);
        if (!o) return false;
        o->setField(key, text);
        sc.commit();
        return true;
    };
    editFormula(r, id, f->text, !f->error.empty(), commit, oid, key);
    if (ui::iconButton(expand, ui::Icon::Fullscreen)) {
        m_requests.openFormula = true;
        m_requests.formulaObject = oid;
        m_requests.formulaKey = key;
        m_requests.formulaText = m_activeId == id ? std::string(m_buffer) : f->text;
        m_requests.formulaCommit = commit;
        if (m_activeId == id) m_activeId = -1;
    }
    ui::tooltip(expand, "Edit in a big editor with the live result (Ctrl+Enter while typing)", "Expand");
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
    Rectangle undo = ui::takeRight(head, 30.0f, 2.0f);
    Rectangle solo = ui::takeRight(head, 30.0f, 6.0f);
    ui::sectionHeader(head, "Objects", m_showObjects);
    bool soloActive = m_solo && !sel.empty();
    if (ui::iconButton(solo, ui::Icon::Solo, soloActive, !sel.empty() || m_solo)) m_requests.toggleSolo = true;
    ui::tooltip(solo, m_solo ? "Showing only the selected objects. Click (or O) to show everything again."
                             : "Show only the selected objects in the view (O). The others stay in the list.",
                "Only the selection");
    ui::anchor("objects.solo", solo);
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

    if (space.objects().empty()) {
        note(col, "No objects yet. Click Add (or press N), pick an example in the gallery (E), or open a saved "
                  "scene (F9).", t.textMuted);
        Rectangle demo = col.row(30.0f);
        if (ui::button(demo, "Demo scene", ui::ButtonKind::Secondary, true, ui::Icon::Gallery)) m_requests.loadDemo = true;
        ui::tooltip(demo, "A matrix M, vectors a and b and a few vectors computed from them (both spaces). "
                          "Ctrl+Z goes back.", "Demo scene");
        ui::anchor("objects.demo", demo);
        col.space(4.0f);
    }
    Rectangle listArea = {col.x(), col.y(), col.width(), 0};
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    for (const auto& obj : space.objects()) {
        SceneObject& o = *obj;
        Rectangle row = col.row(36.0f);
        listArea = ui::unite(listArea, row);
        Rectangle eye = {row.x + row.width - ui::px(32), row.y + ui::px(3), ui::px(30), ui::px(30)};
        bool selected = sel.contains(o.id());
        bool hoverRow = ui::hovered(row) && !ui::hovered(eye);
        if (hoverRow && ui::mousePressed(MOUSE_BUTTON_LEFT)) {
            if (m_activeId >= 0) finishEditing(sc);
            if (ctrl) sel.toggle(o.id());
            else sel.set(o.id());
        }
        if (selected) ui::fillRounded(row, ui::px(ui::kRadius), sel.primary() == o.id() ? t.accentSoft : Fade(t.accentSoft, 0.5f));
        else if (hoverRow) ui::fillRounded(row, ui::px(ui::kRadius), t.hover);

        float dot = ui::px(9.0f);
        Color c = view::SceneView::colorOf(o);
        bool hiddenBySolo = soloActive && !selected;
        if (!o.visible || hiddenBySolo) c = Fade(c, 0.3f);
        DrawCircleV({row.x + ui::px(12), row.y + row.height / 2}, dot / 2, c);
        // Outline, so pale colors (Chalkboard's chalk white) stay visible on a light list
        DrawCircleLinesV({row.x + ui::px(12), row.y + row.height / 2}, dot / 2, Fade(t.borderStrong, 0.8f));
        float tx = row.x + ui::px(24);
        float nameW = std::min(ui::px(64.0f), ui::textWidth(o.name().c_str(), ui::FontRole::BodyStrong) + ui::px(2));
        ui::textIn({tx, row.y, nameW, row.height}, ui::ellipsize(o.name().c_str(), ui::FontRole::BodyStrong, ui::px(64)).c_str(),
                   ui::FontRole::BodyStrong, o.visible && !hiddenBySolo ? t.text : t.textFaint);
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
        else if (hiddenBySolo) ui::tooltip(row, "Hidden in the view while only the selection is shown (O).");
    }
    ui::anchor("objects.list", listArea);

    // Show all: undoes hiding (one undo step) and turns "only the selection" off
    bool anyHidden = false;
    for (const auto& obj : space.objects()) anyHidden = anyHidden || !obj->visible;
    if (anyHidden || soloActive) {
        col.space(2.0f);
        Rectangle all = col.row(28.0f);
        if (ui::button(all, "Show all", ui::ButtonKind::Secondary, true, ui::Icon::Eye)) {
            if (anyHidden) {
                for (const auto& obj : space.objects()) obj->visible = true;
                sc.commit();
            }
            m_requests.soloOff = true;
        }
        ui::tooltip(all, "Show every object again (hidden ones and the ones left out by 'only the selection').",
                    "Show all");
        ui::anchor("objects.showall", all);
    }
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
    Rectangle refButton = ui::takeRight(headRow, 26.0f, 0.0f);
    if (ui::iconButton(refButton, ui::Icon::Formula)) m_requests.openReference = true;
    ui::anchor("inspector.reference", refButton);
    ui::tooltip(refButton, "Everything you can type in a formula, with examples (F2)", "Formula reference");
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
                {"Identity", "I", Matrix3D::identity(), "Start over: back to the identity, which leaves every vector "
                                                        "where it is."},
                {"Rotate 45°", "R", Matrix3D::rotation(kPi / 4), "Adds a 45° turn about the z-axis on top of the "
                                                                 "current matrix (det unchanged)."},
                {"Scale 2×", "S", Matrix3D::scale(2, 2), "Then doubles x and y, keeping z (det × 4)."},
                {"Shear", "X", Matrix3D::shear(0.5f, 0), "Then x' = x + 0.5 y: squares become parallelograms (det "
                                                        "unchanged)."},
                {"Reflect", "F", Matrix3D::scale(-1, 1), "Then x' = −x: a mirror image (det changes sign)."},
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
                        m.applyPreset(p.value);  // on top of the current matrix; Identity starts over
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
    } else if (type == "epicycles") {
        auto& ep = static_cast<scene::EpicyclesObject&>(o);
        col.setY(fieldRow(col.x(), col.y(), col.width(), ep, "z", sc, 46.0f));
        ui::anchor("epicycles.z", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
        col.setY(fieldRow(col.x(), col.y(), col.width(), ep, "N", sc, 110.0f));
        ui::anchor("epicycles.N", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
        col.setY(fieldRow(col.x(), col.y(), col.width(), ep, "speed", sc, 110.0f));
        if (ep.error().empty() && !ep.terms().empty()) {
            char buf[160];
            snprintf(buf, sizeof(buf), "%d circles of %d frequencies (−%d..%d), biggest first", ep.circles(),
                     2 * scene::EpicyclesObject::kMaxK + 1, scene::EpicyclesObject::kMaxK, scene::EpicyclesObject::kMaxK);
            note(col, buf, t.text);
            std::string list;
            for (size_t i = 1; i < std::min<size_t>(ep.terms().size(), 6); i++) {
                const auto& term = ep.terms()[i];
                char one[48];
                snprintf(one, sizeof(one), "%sk = %d: r %.2f", i > 1 ? ",  " : "", term.k, std::abs(term.c));
                list += one;
            }
            note(col, list, t.textMuted);
        }
        if (ui::button(col.row(30), "Next example", ui::ButtonKind::Secondary, true, ui::Icon::Gallery)) {
            m_activeId = -1;
            static const char* kShapes[] = {
                "(16sin(u)^3 + i*(13cos(u) - 5cos(2u) - 2cos(3u) - cos(4u)))/8",  // heart
                "2(cos(u) + i*sin(u))/max(abs(cos(u)), abs(sin(u)))",             // square
                "2.5exp(i*u)*(1 + 0.35cos(5u))",                                    // flower
                "2sin(u) + i*sin(2u)",                                              // figure eight
                "1.6exp(i*u) + 0.8exp(6i*u)",                                       // epitrochoid
                "2exp(i*u)",                                                        // a circle: one circle
            };
            int n = static_cast<int>(sizeof(kShapes) / sizeof(kShapes[0]));
            m_exampleIndex = (m_exampleIndex + 1) % n;
            ep.setField("z", kShapes[m_exampleIndex]);
            commit();
        }
        note(col, "u runs once around the curve (0 to 2π). Each circle turns k times per lap; more circles follow "
                  "the corners better. Press P to play.",
             t.textFaint);
    } else if (type == "fourierseries") {
        auto& fs = static_cast<scene::FourierSeriesObject&>(o);
        col.setY(fieldRow(col.x(), col.y(), col.width(), fs, "f", sc, 52.0f));
        ui::anchor("series.f", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
        {
            Rectangle row = col.row(34.0f);
            Rectangle l = ui::takeLeft(row, 34.0f, 6.0f);
            ui::textIn(l, "x", ui::FontRole::Body, t.textMuted);
            auto cells = ui::splitRow(row, {1, 1}, 26.0f);
            const char* keys[2] = {"a", "b"};
            for (int k = 0; k < 2; k++) {
                std::string key = keys[k];
                scene::Field* f = fs.field(key);
                editText(cells[k], fieldId(fs, key), f->text, !f->error.empty(),
                         [&sc, kind, oid, key](const std::string& text) {
                             SceneObject* ob = sc.space(kind).find(oid);
                             if (!ob) return false;
                             ob->setField(key, text);
                             sc.commit();
                             return true;
                         });
            }
            ui::textIn({cells[0].x + cells[0].width, row.y, ui::px(26), row.height}, "to", ui::FontRole::Small,
                       t.textMuted, ui::Align::Center);
        }
        col.setY(fieldRow(col.x(), col.y(), col.width(), fs, "N", sc, 52.0f));
        ui::anchor("series.N", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
        auto toggleProp = [&](const char* text, const char* key, bool current, const char* tip) {
            Rectangle tr = col.row(30);
            bool v = current;
            if (ui::toggleRow(tr, text, v)) {
                fs.setProp(key, v);
                commit();
            }
            ui::tooltip(tr, tip, text);
        };
        toggleProp("Show f", "original", fs.showOriginal(), "Draw f itself (gray) next to the partial sum.");
        toggleProp("Harmonics", "harmonics", fs.showHarmonics(), "Draw the separate waves a_k cos + b_k sin (the first 8).");
        ui::anchor("series.harmonics", {col.x(), col.y() - ui::px(34), col.width(), ui::px(30)});
        toggleProp("Repeat outside", "extend", fs.extended(),
                   "Draw the sum over three periods: the series repeats f with period b − a.");
        if (fs.error().empty()) {
            // Spectrum: the amplitude √(a_k² + b_k²) of each wave
            const auto& se = fs.series();
            int n = static_cast<int>(se.ak.size());
            int bars = std::min(n, 40);
            if (bars > 0) {
                Rectangle chart = col.row(78.0f);
                ui::fillRounded(chart, ui::px(6), t.panelAlt);
                double amax = 1e-12;
                for (int k = 0; k < bars; k++) amax = std::max(amax, std::hypot(se.ak[k], se.bk[k]));
                float pad = ui::px(8), bw = (chart.width - 2 * pad) / bars;
                float base = chart.y + chart.height - ui::px(16);
                for (int k = 0; k < bars; k++) {
                    double amp = std::hypot(se.ak[k], se.bk[k]);
                    float h = static_cast<float>(amp / amax) * (chart.height - ui::px(26));
                    Rectangle bar = {chart.x + pad + k * bw + bw * 0.15f, base - h, bw * 0.7f, std::max(1.0f, h)};
                    DrawRectangleRec(bar, view::SceneView::colorOf(fs));
                    if ((k + 1) % 5 == 0 || k == 0) {
                        char kb[8];
                        snprintf(kb, sizeof(kb), "%d", k + 1);
                        ui::textIn({chart.x + pad + k * bw - ui::px(6), base + ui::px(1), bw + ui::px(12), ui::px(14)}, kb,
                                   ui::FontRole::Small, t.textFaint, ui::Align::Center);
                    }
                }
                ui::anchor("series.spectrum", chart);
                ui::tooltip(chart, "How strong each wave k is: √(a_k² + b_k²). A square wave has only odd k, falling "
                                   "like 1/k.",
                            "Spectrum");
            }
            char buf[160];
            snprintf(buf, sizeof(buf), "a₀/2 = %.3f", se.a0 / 2);
            note(col, buf, t.text);
            for (int k = 1; k <= std::min(n, 4); k++) {
                char one[64];
                snprintf(one, sizeof(one), "a%d = %.3f    b%d = %.3f", k, std::fabs(se.ak[k - 1]) < 5e-4 ? 0.0 : se.ak[k - 1],
                         k, std::fabs(se.bk[k - 1]) < 5e-4 ? 0.0 : se.bk[k - 1]);
                note(col, one, t.textMuted);
            }
            if (fs.undefinedPoints() > 0) {
                snprintf(buf, sizeof(buf), "f is undefined at %d sample points (counted as 0).", fs.undefinedPoints());
                note(col, buf, t.warning);
            }
        }
        if (ui::button(col.row(30), "Next example", ui::ButtonKind::Secondary, true, ui::Icon::Gallery)) {
            m_activeId = -1;
            static const char* kFns[] = {
                "sign(x)",            // square wave
                "x",                           // sawtooth
                "abs(x)",                      // triangle
                "abs(sin(x))",                 // rectified sine
                "exp(-x^2)",                   // a bump
            };
            int n = 5;
            m_exampleIndex = (m_exampleIndex + 1) % n;
            fs.setField("f", kFns[m_exampleIndex]);
            commit();
        }
        note(col, "The series repeats f with period b − a. Near a jump the partial sums overshoot by about 9% "
                  "(Gibbs), however many terms.",
             t.textFaint);
    } else if (type == "frame") {
        auto& fr = static_cast<scene::FrameObject&>(o);
        for (const char* k : {"x", "y", "z"}) col.setY(fieldRow(col.x(), col.y(), col.width(), fr, k, sc, 64.0f));
        ui::anchor("frame.curve", {col.x(), col.y() - ui::px(120), col.width(), ui::px(116)});
        {
            Rectangle row = col.row(34.0f);
            Rectangle l = ui::takeLeft(row, 34.0f, 6.0f);
            ui::textIn(l, "u", ui::FontRole::Body, t.textMuted);
            auto cells = ui::splitRow(row, {1, 1}, 26.0f);
            const char* keys[2] = {"umin", "umax"};
            for (int k = 0; k < 2; k++) {
                std::string key = keys[k];
                scene::Field* f = fr.field(key);
                editText(cells[k], fieldId(fr, key), f->text, !f->error.empty(),
                         [&sc, kind, oid, key](const std::string& text) {
                             SceneObject* ob = sc.space(kind).find(oid);
                             if (!ob) return false;
                             ob->setField(key, text);
                             sc.commit();
                             return true;
                         });
            }
            ui::textIn({cells[0].x + cells[0].width, row.y, ui::px(26), row.height}, "to", ui::FontRole::Small,
                       t.textMuted, ui::Align::Center);
        }
        col.setY(fieldRow(col.x(), col.y(), col.width(), fr, "u0", sc, 64.0f));
        ui::anchor("frame.u0", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
        auto toggleProp = [&](const char* text, const char* key, bool current, const char* tip) {
            Rectangle tr = col.row(30);
            bool v = current;
            if (ui::toggleRow(tr, text, v)) {
                fr.setProp(key, v);
                commit();
            }
            ui::tooltip(tr, tip, text);
        };
        toggleProp("Frame T, N, B", "frame", fr.showFrame(),
                   "Tangent T (direction of motion), normal N (towards the center of the turn), binormal B = T × N.");
        toggleProp("Osculating circle", "circle", fr.showCircle(),
                   "The circle that hugs the curve best at the point: radius 1/κ, in the plane of T and N.");
        toggleProp("Curvature comb", "comb", fr.showComb(),
                   "Teeth along the curve, as long as the curvature there: where it bends most, they are longest.");
        ui::anchor("frame.comb", {col.x(), col.y() - ui::px(34), col.width(), ui::px(30)});
        toggleProp("Curve", "curve", fr.showCurve(), "Draw the curve itself.");
        if (fr.error().empty()) {
            const geometry::Frame& f = fr.frame();
            char buf[160];
            snprintf(buf, sizeof(buf), "κ = %.4f   (radius %s)", f.kappa,
                     f.kappa > 1e-9 ? std::to_string(1.0 / f.kappa).substr(0, 6).c_str() : "∞, straight");
            note(col, buf, t.text);
            snprintf(buf, sizeof(buf), "τ = %.4f   speed |r'| = %.3f", std::fabs(f.tau) < 5e-5 ? 0.0 : f.tau, f.speed);
            note(col, buf, t.text);
            note(col, "κ: how sharply it turns (1/radius). τ: how fast it twists out of its plane (0 for flat curves). "
                      "Use t in \"At u\" to ride along.",
                 t.textFaint);
        }
    } else if (type == "doubleintegral" || type == "lineintegral") {
        bool dbl = type == "doubleintegral";
        auto rangeRow = [&](const char* lo, const char* hi, const char* label) {
            Rectangle row = col.row(34.0f);
            Rectangle l = ui::takeLeft(row, 34.0f, 6.0f);
            ui::textIn(l, label, ui::FontRole::Body, t.textMuted);
            auto cells = ui::splitRow(row, {1, 1}, 26.0f);
            const char* keys[2] = {lo, hi};
            for (int k = 0; k < 2; k++) {
                std::string key = keys[k];
                scene::Field* f = o.field(key);
                editText(cells[k], fieldId(o, key), f->text, !f->error.empty(),
                         [&sc, kind, oid, key](const std::string& text) {
                             SceneObject* ob = sc.space(kind).find(oid);
                             if (!ob) return false;
                             ob->setField(key, text);
                             sc.commit();
                             return true;
                         });
            }
            ui::textIn({cells[0].x + cells[0].width, row.y, ui::px(26), row.height}, "to", ui::FontRole::Small,
                       t.textMuted, ui::Align::Center);
        };
        char buf[160];
        if (dbl) {
            auto& d = static_cast<scene::DoubleIntegralObject&>(o);
            col.setY(fieldRow(col.x(), col.y(), col.width(), d, "f", sc, 34.0f));
            ui::anchor("integral.f", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
            rangeRow("xmin", "xmax", "x");
            rangeRow("ymin", "ymax", "y");
            col.setY(fieldRow(col.x(), col.y(), col.width(), d, "n", sc, 110.0f));
            ui::anchor("integral.n", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
            Rectangle ruleRow = col.row(30.0f);
            int rule = static_cast<int>(d.rule());
            ui::anchor("integral.rule", ruleRow);
            if (ui::segmented(ruleRow, {"Left", "Middle", "Right"}, rule,
                              {"Each box takes the height of f at its lower-left corner.",
                               "Each box takes the height of f at its center (usually the most accurate).",
                               "Each box takes the height of f at its upper-right corner."})) {
                d.setProp("rule", rule == 0 ? "left" : rule == 2 ? "right" : "middle");
                commit();
            }
            bool boxes = d.showBoxes(), surface = d.showSurface();
            if (ui::toggleRow(col.row(30), "Boxes", boxes)) {
                d.setProp("boxes", boxes);
                commit();
            }
            if (ui::toggleRow(col.row(30), "Surface", surface)) {
                d.setProp("surface", surface);
                commit();
            }
            if (d.error().empty()) {
                int n = d.boxesPerSide();
                const char* ruleName = rule == 0 ? "left" : rule == 2 ? "right" : "middle";
                snprintf(buf, sizeof(buf), "Boxes (%d × %d, %s): %.4f", n, n, ruleName, d.riemann().sum);
                note(col, buf, t.text);
                snprintf(buf, sizeof(buf), "Volume ∫∫ f dA ≈ %.4f", d.exact());
                note(col, buf, t.text);
                snprintf(buf, sizeof(buf), "Error of the boxes: %.4f", d.riemann().sum - d.exact());
                note(col, buf, t.textMuted);
                if (d.riemann().skipped > 0) {
                    snprintf(buf, sizeof(buf), "%d boxes left out: f is undefined there.", d.riemann().skipped);
                    note(col, buf, t.warning);
                }
            }
            note(col, "More boxes get closer to the exact volume. Volume below the floor counts negative. Use a "
                      "parameter in n (e.g. round(2 + 18*s)) to watch the boxes refine.",
                 t.textFaint);
        } else {
            auto& li = static_cast<scene::LineIntegralObject&>(o);
            col.setY(fieldRow(col.x(), col.y(), col.width(), li, "F", sc, 92.0f));
            ui::anchor("integral.F", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
            for (const char* k : {"x", "y", "z"}) col.setY(fieldRow(col.x(), col.y(), col.width(), li, k, sc, 64.0f));
            rangeRow("umin", "umax", "u");
            col.setY(fieldRow(col.x(), col.y(), col.width(), li, "upto", sc, 64.0f));
            ui::anchor("integral.upto", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
            if (li.error().empty()) {
                snprintf(buf, sizeof(buf), "Work so far (%.0f%% of the path): %.4f", li.upto() * 100, li.value().re);
                note(col, buf, t.text);
                snprintf(buf, sizeof(buf), "Whole path: %.4f", li.total());
                note(col, buf, t.textMuted);
            }
            note(col, "Green where F pushes along the motion, red where it pushes against it. Up to: 0 to 1, e.g. a "
                      "parameter or mod(t/4, 1) to travel the path.",
                 t.textFaint);
        }
    } else if (type == "quaternion") {
        auto& qo = static_cast<scene::QuaternionObject&>(o);
        col.setY(fieldRow(col.x(), col.y(), col.width(), qo, "value", sc, 34.0f));
        ui::anchor("quaternion.value", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
        expr::Rotation r;
        if (qo.error().empty() && expr::rotationOf(qo.value(), r)) {
            note(col, "= " + qo.value().summary(), t.textMuted);
            char buf[128];
            if (r.hasAxis)
                snprintf(buf, sizeof(buf), "%.1f° about (%.2f, %.2f, %.2f)", r.angle * 180.0 / 3.14159265358979,
                         r.axis.x, r.axis.y, r.axis.z);
            else
                snprintf(buf, sizeof(buf), "No rotation (angle 0)");
            note(col, buf, t.text);
            if (std::fabs(r.norm - 1.0) > 1e-3) {
                snprintf(buf, sizeof(buf), "|q| = %.3f, not 1: drawn as the rotation of q/|q|. unit(q) makes it 1.", r.norm);
                note(col, buf, t.warning);
            } else {
                note(col, "|q| = 1", t.textFaint);
            }
        } else if (qo.error().empty() && qo.value().type == expr::Type::Quaternion) {
            note(col, "The zero quaternion is not a rotation.", t.warning);
        }
        auto toggleProp = [&](const char* text, const char* key, bool current, const char* tip) {
            Rectangle tr = col.row(30);
            bool v = current;
            if (ui::toggleRow(tr, text, v)) {
                qo.setProp(key, v);
                commit();
            }
            ui::tooltip(tr, tip, text);
            return v;
        };
        toggleProp("Axis and angle", "axis", qo.showAxis(),
                   "The rotation axis through the origin, and an arc showing how far it turns (right-hand rule).");
        toggleProp("Rotated basis", "basis", qo.showBasis(),
                   "Where the rotation sends the x, y and z axes (the faint ones are the originals).");
        bool acts = toggleProp("Acts on a vector", "act", qo.actsOnVector(),
                               "Shows a vector v (gray) and the turned vector q v q⁻¹, with the circle its tip follows.");
        ui::anchor("quaternion.act", {col.x(), col.y() - ui::px(34), col.width(), ui::px(30)});
        if (acts) col.setY(fieldRow(col.x(), col.y(), col.width(), qo, "v", sc, 64.0f));
        if (ui::button(col.row(30), "Next example", ui::ButtonKind::Secondary, true, ui::Icon::Gallery)) {
            m_activeId = -1;
            static const char* kQuats[] = {
                "qaxis([0, 0, 1], pi/2)",      // a quarter turn about z
                "qaxis([1, 0, 0], pi)",        // a half turn about x
                "qaxis([1, 1, 1], 2pi/3)",     // 120° about the diagonal: x -> y -> z
                "quat(1, 0, 0, 0)",            // no rotation
                "quat(1, 1, 0, 0)",            // not unit: still a rotation (90° about x)
                "qaxis([0, 0, 1], t)",         // turning with the clock
            };
            int n = static_cast<int>(sizeof(kQuats) / sizeof(kQuats[0]));
            m_exampleIndex = (m_exampleIndex + 1) % n;
            qo.setField("value", kQuats[m_exampleIndex]);
            commit();
        }
        note(col, "qaxis(axis, angle) makes a rotation; q1*q2 does q2 first, then q1. The formula reference (F2) has "
                  "the rest.",
             t.textFaint);
    } else if (type == "field") {
        auto& fo = static_cast<scene::VectorFieldObject&>(o);
        col.setY(fieldRow(col.x(), col.y(), col.width(), fo, "F", sc, 92.0f));
        ui::anchor("field.formula", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});

        Rectangle domainRow = col.row(30.0f);
        int domain = fo.volume() ? 1 : 0;
        if (ui::segmented(domainRow, {"Plane", "Volume"}, domain,
                          {"On the floor z = 0: F's x and y components, e.g. [-y, x, 0].",
                           "A box of arrows in space: F(x, y, z) with all three components."})) {
            fo.setProp("domain", domain == 1 ? "volume" : "plane");
            commit();
        }
        ui::anchor("field.domain", domainRow);

        auto rangeRow = [&](const char* lo, const char* hi, const char* label) {
            Rectangle row = col.row(34.0f);
            Rectangle l = ui::takeLeft(row, 34.0f, 6.0f);
            ui::textIn(l, label, ui::FontRole::Body, t.textMuted);
            auto cells = ui::splitRow(row, {1, 1}, 26.0f);
            const char* keys[2] = {lo, hi};
            for (int k = 0; k < 2; k++) {
                std::string key = keys[k];
                scene::Field* f = fo.field(key);
                editText(cells[k], fieldId(fo, key), f->text, !f->error.empty(),
                         [&sc, kind, oid, key](const std::string& text) {
                             SceneObject* ob = sc.space(kind).find(oid);
                             if (!ob) return false;
                             ob->setField(key, text);
                             sc.commit();
                             return true;
                         });
            }
            ui::textIn({cells[0].x + cells[0].width, row.y, ui::px(26), row.height}, "to", ui::FontRole::Small,
                       t.textMuted, ui::Align::Center);
        };
        rangeRow("xmin", "xmax", "x");
        rangeRow("ymin", "ymax", "y");
        if (fo.volume()) rangeRow("zmin", "zmax", "z");

        // Arrows per unit
        Rectangle row = col.row(30.0f);
        Rectangle label = ui::takeLeft(row, 52.0f, 8.0f);
        Rectangle value = ui::takeRight(row, 64.0f, 8.0f);
        ui::textIn(label, "Detail", ui::FontRole::Body, t.text);
        float density = static_cast<float>(fo.density());
        if (ui::slider(row, density, static_cast<float>(scene::VectorFieldObject::kDensityMin),
                       static_cast<float>(scene::VectorFieldObject::kDensityMax))) {
            fo.setProp("density", std::round(static_cast<double>(density) * 4.0) / 4.0);
            m_sliderDirty = true;
        }
        ui::anchor("field.detail", row);
        char dbuf[48];
        snprintf(dbuf, sizeof(dbuf), "%g/unit", fo.density());
        ui::textIn(value, dbuf, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
        ui::tooltip({label.x, row.y, value.x + value.width - label.x, row.height},
                    "Arrows per unit along each axis. A wider range gets more arrows.", "Detail");
        scene::VectorFieldObject::Counts cn = fo.counts();
        char cbuf[96];
        if (fo.volume()) snprintf(cbuf, sizeof(cbuf), "%d × %d × %d arrows", cn.nx, cn.ny, cn.nz);
        else snprintf(cbuf, sizeof(cbuf), "%d × %d arrows", cn.nx, cn.ny);
        std::string counts = std::string(cbuf) + (cn.capped ? " (capped for speed)" : "");
        note(col, counts, cn.capped ? t.warning : t.textFaint);

        auto toggleProp = [&](const char* text, const char* key, bool current, const char* tip) {
            Rectangle r = col.row(30);
            bool v = current;
            if (ui::toggleRow(r, text, v)) {
                fo.setProp(key, v);
                commit();
            }
            ui::tooltip(r, tip, text);
            return v;
        };
        toggleProp("Color by strength", "heatmap", fo.heatmap(),
                   "Arrows colored by |F| with the heat map (blue weak, red strong); off: the object's color.");
        toggleProp("Same length", "same_length", fo.sameLength(),
                   "All arrows one length: only the direction shows (a direction field). The color still shows |F|.");
        bool lines = toggleProp("Flow lines", "lines", fo.lines(),
                                "Curves that follow F everywhere (streamlines), with an arrowhead showing the way.");
        ui::anchor("field.lines", {col.x(), col.y() - ui::px(34), col.width(), ui::px(30)});
        bool particles = toggleProp("Particles", "particles", fo.particles(),
                                    "Dots drifting along the flow lines as the scene clock plays (press P).");
        ui::anchor("field.particles", {col.x(), col.y() - ui::px(34), col.width(), ui::px(30)});
        if (lines || particles) {
            Rectangle sr = col.row(30.0f);
            Rectangle sl = ui::takeLeft(sr, 92.0f, 8.0f);
            Rectangle sv = ui::takeRight(sr, 40.0f, 8.0f);
            ui::textIn(sl, "Lines per axis", ui::FontRole::Body, t.text);
            float seeds = static_cast<float>(fo.seeds());
            if (ui::slider(sr, seeds, 2.0f, fo.volume() ? 4.0f : 16.0f)) {
                fo.setProp("seeds", std::round(static_cast<double>(seeds)));
                m_sliderDirty = true;
            }
            char sb[16];
            snprintf(sb, sizeof(sb), "%d", field::seedsPerAxis(!fo.volume(), fo.seeds()));
            ui::textIn(sv, sb, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
            ui::tooltip({sl.x, sr.y, sv.x + sv.width - sl.x, sr.height},
                        "How many flow lines start along each axis (evenly spaced).", "Lines per axis");
        }
        if (particles) {
            Rectangle pr = col.row(30.0f);
            Rectangle pl = ui::takeLeft(pr, 92.0f, 8.0f);
            Rectangle pv = ui::takeRight(pr, 40.0f, 8.0f);
            ui::textIn(pl, "Speed", ui::FontRole::Body, t.text);
            float speed = static_cast<float>(fo.speed());
            if (ui::slider(pr, speed, 0.1f, 5.0f)) {
                fo.setProp("speed", std::round(static_cast<double>(speed) * 10.0) / 10.0);
                m_sliderDirty = true;
            }
            char pb[16];
            snprintf(pb, sizeof(pb), "%.1f×", fo.speed());
            ui::textIn(pv, pb, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
            ui::tooltip({pl.x, pr.y, pv.x + pv.width - pl.x, pr.height},
                        "How fast particles move: 1× is the field's own speed.", "Speed");
        }

        if (ui::button(col.row(30), "Next example", ui::ButtonKind::Secondary, true, ui::Icon::Gallery)) {
            m_activeId = -1;
            struct FieldExample {
                const char* f;
                bool volume;
            };
            static const FieldExample kFields[] = {
                {"[-y, x, 0]", false},                        // rotation
                {"[x, y, 0]", false},                         // source
                {"[x, -y, 0]", false},                        // saddle
                {"[-y - x/4, x - y/4, 0]", false},            // vortex sink
                {"[sin(y), sin(x), 0]", false},               // cells
                {"[-y, x, 0.4]", true},                       // helix
                {"[-y, x, -z]/2 + [0, 0, sin(x)]/3", true},   // swirl
            };
            int n = static_cast<int>(sizeof(kFields) / sizeof(kFields[0]));
            m_exampleIndex = (m_exampleIndex + 1) % n;
            fo.setField("F", kFields[m_exampleIndex].f);
            fo.setProp("domain", kFields[m_exampleIndex].volume ? "volume" : "plane");
            commit();
        }
        const FieldRenderer* fr = view.fieldRenderer(fo.id());
        if (!fo.error().empty()) note(col, fo.error(), t.danger);
        else if (fr && !fr->error().empty()) note(col, fr->error(), t.danger);
        else if (fr && fr->arrowCount() > 0) {
            char buf[96];
            snprintf(buf, sizeof(buf), "|F| ranges over [%.2f, %.2f]", fr->minMagnitude(), fr->maxMagnitude());
            note(col, buf, t.textMuted);
        }
        note(col, "F is a vector formula of x, y, z (and t, the scene time); parameters work by name, e.g. "
                  "k*[-y, x, 0].",
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

    if (!o.error().empty() && type != "matrix" && type != "field" && !dynamic_cast<scene::PlotObject*>(&o)) {
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
