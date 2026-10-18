#include "ui/ScenePanel.hpp"
#include "ui/ScenePanelDetail.hpp"
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

namespace panel {


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

void note(ui::Column& col, const std::string& text, Color color, ui::FontRole role) {
    float h = ui::textWrapped({col.x(), col.y(), col.width(), 0}, text.c_str(), role, color);
    col.space(h / ui::scale() + 4.0f);
}

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

} // namespace panel

using namespace panel;

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

    // --- Type-specific content (ScenePanelInspectors.cpp) ------------------------
    InspectorContext ctx{col, o, sc, kind, view};
    if (type == "vector") {
        inspectVector(ctx);
    } else if (type == "matrix") {
        inspectMatrix(ctx);
    } else if (type == "eigen") {
        inspectEigen(ctx);
    } else if (dynamic_cast<scene::PlotObject*>(&o)) {
        inspectPlot(ctx);
    } else if (type == "epicycles") {
        inspectEpicycles(ctx);
    } else if (type == "fourierseries") {
        inspectFourierSeries(ctx);
    } else if (type == "frame") {
        inspectFrame(ctx);
    } else if (type == "doubleintegral" || type == "lineintegral") {
        inspectIntegral(ctx);
    } else if (type == "quaternion") {
        inspectQuaternion(ctx);
    } else if (type == "field") {
        inspectField(ctx);
    } else if (type == "parameter") {
        inspectParameter(ctx);
    } else if (type == "complex") {
        inspectComplex(ctx);
    } else if (type == "product") {
        inspectProduct(ctx);
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
