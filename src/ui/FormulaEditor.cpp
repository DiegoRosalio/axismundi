#include "ui/FormulaEditor.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include "ui/core/Ui.hpp"
#include <algorithm>

namespace linalg {

namespace {
constexpr int kEditorId = 900001;  // any id not used by the inspector
}

void FormulaEditor::open(scene::SpaceKind kind, scene::ObjectId object, const std::string& key,
                         const std::string& text, std::function<bool(const std::string&)> commit) {
    m_open = true;
    m_justOpened = true;
    m_kind = kind;
    m_object = object;
    m_key = key;
    m_text = text;
    m_commit = std::move(commit);
    m_state = ui::EditorState{};
    m_state.id = kEditorId;
    m_state.cursor = m_state.anchor = m_text.size();
}

bool FormulaEditor::apply() {
    if (m_commit && !m_commit(m_text)) return false;
    m_open = false;
    return true;
}

bool FormulaEditor::draw(scene::Scene& sc) {
    if (!m_open) return false;
    const ui::Theme& t = ui::theme();
    scene::SceneObject* obj = sc.space(m_kind).find(m_object);
    const scene::Field* field = obj ? obj->field(m_key) : nullptr;
    if (!field) {  // the object was deleted meanwhile
        m_open = false;
        return false;
    }

    // Dim everything behind: the editor is modal
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, t.dark ? 0.45f : 0.25f));
    float w = std::min(ui::px(760.0f), GetScreenWidth() - ui::px(32.0f));
    float editorW = w - ui::px(36.0f);
    float editorH = std::max(ui::px(120.0f), ui::editorHeight(m_text, editorW, 10));
    float h = std::min(editorH + ui::px(172.0f), GetScreenHeight() - ui::px(32.0f));
    Rectangle box = {(GetScreenWidth() - w) / 2, std::max(ui::px(16.0f), (GetScreenHeight() - h) / 3), w, h};
    ui::card(box, true);
    ui::Column col(box.x + ui::px(18), box.y + ui::px(16), box.width - ui::px(36), 8.0f);

    std::string title = obj->name() + " · " + field->label;
    ui::textIn(col.row(26), title.c_str(), ui::FontRole::Title, t.text);

    // The editor
    Rectangle er = {col.x(), col.y(), col.width(), editorH};
    col.setY(er.y + er.height + ui::px(8));
    expr::Program probe;
    bool syntaxOk = probe.compile(m_text, field->variables());
    ui::EditorEvent ev = ui::textEditor(er, kEditorId, m_text, true, !syntaxOk, m_state);
    ui::anchor("formula.editor", er);
    bool enter = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    bool done = false;
    // Only Enter applies here: clicks outside the text (buttons, the dim) must not
    // A syntax error blocks Enter like it disables Apply (unknown names are allowed: they may come later)
    if (ev == ui::EditorEvent::Commit && enter && !m_justOpened && syntaxOk) done = apply();

    // Live result, evaluated like the object does (t = scene time; sample x, y, u for plots)
    std::string result;
    bool ok = false;
    if (!syntaxOk) {
        result = probe.error();
    } else {
        expr::Slots slots;
        slots.t = obj->time();
        slots.hasT = true;
        if (field->expect == scene::Expect::Plot || field->expect == scene::Expect::Field) {
            slots.x = 0.37;
            slots.y = 0.29;
            slots.u = 0.41;
            slots.z = 0.23;
            slots.hasX = slots.hasY = slots.hasU = slots.hasZ = true;
        }
        expr::Value v = probe.eval(&sc.space(m_kind), &slots);
        std::string err = scene::checkExpect(field->expect, v);
        if (!err.empty()) {
            result = err;
        } else if (field->expect == scene::Expect::Plot) {
            result = "plot formula, e.g. " + v.summary() + " at the sample point";
        } else if (field->expect == scene::Expect::Field) {
            result = "field formula, e.g. F = " + v.summary() + " at (0.37, 0.29, 0.23)";
            ok = true;
        } else {
            result = "= " + v.summary() + "   (" + v.typeName() + ")";
            ok = true;
        }
    }
    float rh = ui::textWrapped({col.x(), col.y(), col.width(), 0}, result.c_str(),
                               ok ? ui::FontRole::Mono : ui::FontRole::Small, ok ? t.text : t.danger);
    col.space(rh / ui::scale() + 6.0f);

    // Buttons
    Rectangle bottom = col.row(32.0f);
    Rectangle applyBtn = ui::takeRight(bottom, 136.0f, 0.0f);
    Rectangle cancel = ui::takeRight(bottom, 96.0f, 8.0f);
    Rectangle ref = ui::takeLeft(bottom, 190.0f, 8.0f);
    bool openRef = ui::button(ref, "Formula reference", ui::ButtonKind::Secondary, true, ui::Icon::Formula, "F2");
    if (ui::button(cancel, "Cancel")) m_open = false;
    if (ui::button(applyBtn, "Apply", ui::ButtonKind::Primary, syntaxOk, ui::Icon::Check, "Enter") && !done) apply();
    ui::textIn({col.x(), col.y(), col.width(), ui::px(18)},
               "Enter applies, Esc cancels. Names refer to objects in this space; t is the scene time.",
               ui::FontRole::Small, t.textFaint);
    m_justOpened = false;
    return openRef;
}

} // namespace linalg
