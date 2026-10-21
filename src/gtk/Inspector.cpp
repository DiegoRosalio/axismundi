#include "gtk/Inspector.hpp"
#include "app/Session.hpp"
#include "gtk/EditBinding.hpp"
#include "gtk/Inspectors.hpp"
#include "math/Expression.hpp"
#include "scene/Types.hpp"
#include "view/SceneView.hpp"
#include <adwaita.h>
#include <glibmm/main.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/button.h>
#include <gtkmm/colordialog.h>
#include <gtkmm/colordialogbutton.h>
#include <gtkmm/window.h>
#include <cmath>
#include <cstdio>
#include <map>

namespace linalg::gtk {

using scene::SceneObject;

namespace {

// Connect a plain callback to any GObject signal that returns nothing (the signal's own arguments are ignored)
void onSignal(gpointer object, const char* signal, std::function<void()> callback) {
    g_signal_connect_data(
        object, signal, G_CALLBACK(+[](std::function<void()>* f) { (*f)(); }), new std::function<void()>(std::move(callback)),
        +[](gpointer data, GClosure*) { delete static_cast<std::function<void()>*>(data); }, G_CONNECT_SWAPPED);
}

std::string shortNumber(double v) {
    char buf[32];
    if (std::fabs(v) < 5e-5) v = 0;
    std::snprintf(buf, sizeof(buf), "%.4g", v);
    return buf;
}

// A number typed in a field: any constant formula (2pi, 1/3...)
bool parseNumber(const std::string& text, double& out) {
    Expression e;
    if (!e.parse(text)) return false;
    out = e.eval(0.0);
    return std::isfinite(out);
}

void setTitle(GtkWidget* row, const std::string& title) {
    adw_preferences_row_set_use_markup(ADW_PREFERENCES_ROW(row), FALSE);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title.c_str());
}

// A row that only holds a widget (not clickable)
GtkWidget* plainRow(GtkWidget* child) {
    GtkWidget* row = gtk_list_box_row_new();
    gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), FALSE);
    gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(row), FALSE);
    gtk_widget_set_focusable(row, FALSE);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), child);
    return row;
}

GtkWidget* wrappedLabel(const char* cssClass) {
    GtkWidget* label = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_bottom(label, 8);
    if (cssClass) gtk_widget_add_css_class(label, cssClass);
    return label;
}

void setVisibleText(GtkWidget* row, GtkWidget* label, const std::string& text) {
    if (gtk_label_get_text(GTK_LABEL(label)) != text) gtk_label_set_text(GTK_LABEL(label), text.c_str());
    gtk_widget_set_visible(row, !text.empty());
}

} // namespace

// ---------------------------------------------------------------------------------------------------------------
// Inspector
// ---------------------------------------------------------------------------------------------------------------

Inspector::Inspector(Session& session) : Gtk::Box(Gtk::Orientation::VERTICAL), m_session(session) {
    m_content.set_orientation(Gtk::Orientation::VERTICAL);
    m_content.set_spacing(18);
    m_content.set_margin(12);
    m_scroll.set_child(m_content);
    m_scroll.set_vexpand(true);
    m_scroll.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    m_placeholder.set_text("Select an object to edit it here.");
    m_placeholder.set_wrap(true);
    m_placeholder.set_justify(Gtk::Justification::CENTER);
    m_placeholder.add_css_class("dim-label");
    m_placeholder.set_margin(24);
    m_placeholder.set_vexpand(true);
    m_placeholder.set_valign(Gtk::Align::CENTER);
    append(m_scroll);
    append(m_placeholder);
    m_scroll.set_visible(false);
}

std::string Inspector::layoutKey() const {
    scene::SpaceKind kind = m_session.spaceKind();
    scene::Scene& sc = m_session.scene();
    const SceneObject* o = sc.space(kind).find(sc.selection(kind).primary());
    if (!o) return "none";
    // What decides the rows: the object, its type, its active fields and the properties that switch modes
    std::string key = std::to_string(static_cast<int>(kind)) + ";" + std::to_string(o->id()) + ";" + o->typeId() + ";";
    if (const InspectorType* type = inspectorFor(o->typeId()))
        for (const std::string& p : type->layoutProps) key += o->prop(p) + ";";
    for (const auto& f : o->fields())
        if (o->fieldActive(f)) key += f.key + ",";
    return key;
}

void Inspector::refresh() {
    if (layoutKey() != m_key) {
        // Later: refresh may run inside a signal of a row that the rebuild would delete
        if (!m_rebuildQueued) {
            m_rebuildQueued = true;
            Glib::signal_idle().connect_once([this] { rebuild(); });
        }
        return;
    }
    update();
}

void Inspector::scrollToEnd() {
    auto adj = m_scroll.get_vadjustment();
    adj->set_value(adj->get_upper() - adj->get_page_size());
}

SceneObject* Inspector::current() {
    if (m_id == scene::kNoObject) return nullptr;
    return m_session.scene().space(m_kind).find(m_id);
}

void Inspector::flush() {
    for (EditBinding* e : m_edits) e->flush();
    if (m_sliderRest.connected()) {
        m_sliderRest.disconnect();
        m_session.scene().commit();
    }
}

void Inspector::edit(const std::function<void(SceneObject&)>& change) {
    SceneObject* o = current();
    if (!o) return;
    m_sliderRest.disconnect();
    change(*o);
    m_session.scene().commit();
    if (m_changed) m_changed();
}

void Inspector::sliderMoved() {
    if (m_changed) m_changed();
    m_sliderRest.disconnect();
    m_sliderRest = Glib::signal_timeout().connect([this] {
        m_session.scene().commit();
        if (m_changed) m_changed();
        return false;
    }, 600);
}

void Inspector::update() {
    SceneObject* o = current();
    if (!o) return;
    m_updating = true;
    for (const Update& u : m_updates) u(*o);
    m_updating = false;
}

void Inspector::rebuild() {
    m_rebuildQueued = false;
    flush();
    // Where the keyboard focus was (which of the inspector's parts), so it comes back to the same place instead of
    // jumping into the name field
    int focusIndex = -1;
    auto* window = dynamic_cast<Gtk::Window*>(get_root());
    if (Gtk::Widget* focus = window ? window->get_focus() : nullptr) {
        int i = 0;
        for (Gtk::Widget* child = m_content.get_first_child(); child; child = child->get_next_sibling(), i++)
            if (focus == child || focus->is_ancestor(*child)) focusIndex = i;
    }
    m_edits.clear();
    m_updates.clear();
    while (Gtk::Widget* child = m_content.get_first_child()) m_content.remove(*child);
    m_key = layoutKey();

    scene::SpaceKind kind = m_session.spaceKind();
    scene::Scene& sc = m_session.scene();
    SceneObject* o = sc.space(kind).find(sc.selection(kind).primary());
    m_scroll.set_visible(o != nullptr);
    m_placeholder.set_visible(o == nullptr);
    if (!o) {
        m_id = scene::kNoObject;
        return;
    }
    if (o->id() != m_id || kind != m_kind) {
        m_exampleIndex = 0;
        m_scroll.get_vadjustment()->set_value(0.0);  // another object starts at its top
    }
    m_id = o->id();
    m_kind = kind;

    buildHeader(*o);
    InspectorBuilder b(*this, *o);
    const InspectorType* type = inspectorFor(o->typeId());
    if (type && type->inspect) type->inspect(b);
    else inspectFields(b);
    // The object's problem, when no field shows it
    b.line([](const SceneObject& ob) {
        for (const auto& f : ob.fields())
            if (ob.fieldActive(f) && !f.error.empty()) return std::string();
        return ob.error();
    }, InspectorBuilder::Tone::Error);
    buildFooter();
    update();
    if (focusIndex >= 0) {
        Gtk::Widget* child = m_content.get_first_child();
        for (int i = 0; child && i < focusIndex; i++) child = child->get_next_sibling();
        if (child) child->grab_focus();
    }
}

void Inspector::buildHeader(SceneObject& o) {
    const scene::ObjectTypeInfo* type = scene::findType(o.typeId());
    GtkWidget* group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), type ? type->label : o.typeId().c_str());
    gtk_box_append(m_content.gobj(), group);

    if (o.typeId() != "parameter") {  // parameters aren't drawn
        auto* color = Gtk::make_managed<Gtk::ColorDialogButton>(Gtk::ColorDialog::create());
        color->get_dialog()->set_with_alpha(false);
        color->set_valign(Gtk::Align::CENTER);
        color->set_tooltip_text("Color");
        color->property_rgba().signal_changed().connect([this, color] {
            if (m_updating) return;
            Gdk::RGBA c = color->get_rgba();
            edit([&c](SceneObject& ob) {
                ob.customColor = true;
                ob.color = {static_cast<unsigned char>(std::lround(c.get_red() * 255)),
                            static_cast<unsigned char>(std::lround(c.get_green() * 255)),
                            static_cast<unsigned char>(std::lround(c.get_blue() * 255)), 255};
                ob.touch();
            });
        });
        m_updates.push_back([color](const SceneObject& ob) {
            Color c = view::SceneView::colorOf(ob);
            Gdk::RGBA want(c.r / 255.0, c.g / 255.0, c.b / 255.0, 1.0);
            if (!(color->get_rgba() == want)) color->set_rgba(want);
        });
        adw_preferences_group_set_header_suffix(ADW_PREFERENCES_GROUP(group), GTK_WIDGET(color->gobj()));
    }

    GtkWidget* name = adw_entry_row_new();
    setTitle(name, "Name");
    EditBinding* e = EditBinding::attach(name, [this](const std::string& text) {
        bool ok = false;
        std::string error;
        edit([&](SceneObject& ob) { ok = m_session.scene().space(m_kind).rename(ob.id(), text, error); });
        if (!ok && m_toast) m_toast("Cannot rename: " + error);
        return ok;
    }, false);
    e->setTip("Other objects use it by this name in their formulas; renaming updates them");
    m_edits.push_back(e);
    m_updates.push_back([e](const SceneObject& ob) { e->show(ob.name(), ""); });
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), name);
}

void Inspector::buildFooter() {
    auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    box->set_homogeneous(true);
    auto button = [&](const char* label, const char* icon, const char* tip, bool danger, std::function<void()> run) {
        GtkWidget* content = adw_button_content_new();
        adw_button_content_set_label(ADW_BUTTON_CONTENT(content), label);
        adw_button_content_set_icon_name(ADW_BUTTON_CONTENT(content), icon);
        auto* b = Gtk::make_managed<Gtk::Button>();
        gtk_button_set_child(GTK_BUTTON(b->gobj()), content);
        b->set_tooltip_text(tip);
        if (danger) b->add_css_class("destructive-action");
        b->signal_clicked().connect(std::move(run));
        box->append(*b);
    };
    button("Duplicate", "edit-copy-symbolic", "A copy of this object (Ctrl+D)", false, [this] {
        flush();
        edit([this](SceneObject& ob) {
            scene::Scene& sc = m_session.scene();
            if (SceneObject* copy = sc.space(m_kind).duplicate(ob.id())) sc.selection(m_kind).set(copy->id());
        });
    });
    button("Delete", "user-trash-symbolic", "Delete this object (Delete; Ctrl+Z brings it back)", true, [this] {
        SceneObject* ob = current();
        if (!ob) return;
        scene::ObjectId id = ob->id();
        scene::Scene& sc = m_session.scene();
        sc.space(m_kind).remove(id);
        sc.selection(m_kind).remove(id);
        sc.commit();
        if (m_changed) m_changed();
    });
    m_content.append(*box);
}

// ---------------------------------------------------------------------------------------------------------------
// InspectorBuilder
// ---------------------------------------------------------------------------------------------------------------

InspectorBuilder::InspectorBuilder(Inspector& inspector, Object& object) : m_inspector(inspector), m_object(object) {}

GtkWidget* InspectorBuilder::currentGroup() {
    if (!m_group) {
        m_group = adw_preferences_group_new();
        gtk_box_append(m_inspector.m_content.gobj(), m_group);
    }
    return m_group;
}

void InspectorBuilder::add(GtkWidget* row) { adw_preferences_group_add(ADW_PREFERENCES_GROUP(currentGroup()), row); }

void InspectorBuilder::group(const std::string& title, const std::string& description) {
    m_group = nullptr;
    GtkWidget* g = currentGroup();
    if (!title.empty()) adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(g), title.c_str());
    if (!description.empty()) adw_preferences_group_set_description(ADW_PREFERENCES_GROUP(g), description.c_str());
}

EditBinding* InspectorBuilder::bindField(GtkWidget* editable, const std::string& key) {
    Inspector* insp = &m_inspector;
    EditBinding* e = EditBinding::attach(editable, [insp, key](const std::string& text) {
        bool found = false;
        insp->edit([&](Object& o) {
            found = o.field(key) != nullptr;
            if (found) o.setField(key, text);  // kept even when it doesn't parse: its problem is shown
        });
        return found;
    }, true);
    m_inspector.m_edits.push_back(e);
    m_inspector.m_updates.push_back([e, key](const Object& o) {
        if (const scene::Field* f = o.field(key)) e->show(f->text, f->error);
    });
    return e;
}

void InspectorBuilder::formula(const std::string& key, const std::string& label) {
    const scene::Field* f = m_object.field(key);
    if (!f) return;
    GtkWidget* row = adw_entry_row_new();
    setTitle(row, label.empty() ? f->label : label);
    EditBinding* e = bindField(row, key);
    add(row);
    // The problem in words underneath (while not typing)
    GtkWidget* text = wrappedLabel("error");
    gtk_widget_add_css_class(text, "caption");
    GtkWidget* problem = plainRow(text);
    gtk_widget_set_visible(problem, FALSE);
    add(problem);
    live([e, key, problem, text](const Object& o) {
        const scene::Field* field = o.field(key);
        setVisibleText(problem, text, field && !e->editing() ? field->error : std::string());
    });
}

void InspectorBuilder::cells(const std::string& title, const std::vector<std::string>& keys, const std::string& between,
                             const std::vector<std::string>& tips, const std::vector<std::string>& captions) {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);
    if (!title.empty()) {
        GtkWidget* l = gtk_label_new(title.c_str());
        gtk_label_set_xalign(GTK_LABEL(l), 0.0f);
        gtk_widget_set_size_request(l, 28, -1);
        gtk_box_append(GTK_BOX(box), l);
    }
    std::vector<std::string> shownKeys;
    for (size_t i = 0; i < keys.size(); i++) {
        if (!m_object.field(keys[i])) continue;
        if (i > 0 && !between.empty()) {
            GtkWidget* l = gtk_label_new(between.c_str());
            gtk_widget_add_css_class(l, "dim-label");
            gtk_box_append(GTK_BOX(box), l);
        }
        GtkWidget* cell = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_widget_set_hexpand(cell, TRUE);
        if (i < captions.size()) {
            GtkWidget* c = gtk_label_new(captions[i].c_str());
            gtk_widget_add_css_class(c, "caption");
            gtk_widget_add_css_class(c, "dim-label");
            gtk_box_append(GTK_BOX(cell), c);
        }
        GtkWidget* entry = gtk_entry_new();
        gtk_editable_set_width_chars(GTK_EDITABLE(entry), 3);
        gtk_widget_set_hexpand(entry, TRUE);
        EditBinding* e = bindField(entry, keys[i]);
        if (i < tips.size()) e->setTip(tips[i]);
        gtk_box_append(GTK_BOX(cell), entry);
        gtk_box_append(GTK_BOX(box), cell);
        shownKeys.push_back(keys[i]);
    }
    add(plainRow(box));
    // The fields' problems underneath, "x: unknown name 'q'"
    GtkWidget* text = wrappedLabel("error");
    gtk_widget_add_css_class(text, "caption");
    GtkWidget* problem = plainRow(text);
    gtk_widget_set_visible(problem, FALSE);
    add(problem);
    bool named = !captions.empty() || keys.size() <= 3;
    live([shownKeys, problem, text, named](const Object& o) {
        std::string all;
        for (const std::string& k : shownKeys) {
            const scene::Field* f = o.field(k);
            if (!f || f->error.empty()) continue;
            all += (all.empty() ? "" : "\n") + (named ? f->label + ": " : std::string()) + f->error;
        }
        setVisibleText(problem, text, all);
    });
}

void InspectorBuilder::numbers(const std::string& title, const std::vector<Number>& fields, const std::string& between,
                               const std::string& tip) {
    GtkWidget* row = adw_action_row_new();
    setTitle(row, title);
    if (!tip.empty()) gtk_widget_set_tooltip_text(row, tip.c_str());
    Inspector* insp = &m_inspector;
    for (size_t i = 0; i < fields.size(); i++) {
        if (i > 0 && !between.empty()) {
            GtkWidget* l = gtk_label_new(between.c_str());
            gtk_widget_add_css_class(l, "dim-label");
            adw_action_row_add_suffix(ADW_ACTION_ROW(row), l);
        }
        GtkWidget* entry = gtk_entry_new();
        gtk_editable_set_width_chars(GTK_EDITABLE(entry), 5);
        gtk_widget_set_valign(entry, GTK_ALIGN_CENTER);
        Number n = fields[i];
        EditBinding* e = EditBinding::attach(entry, [insp, n](const std::string& text) {
            double v;
            if (!parseNumber(text, v)) return false;
            bool ok = false;
            insp->edit([&](Object& o) { ok = n.set(o, v); });
            return ok;
        }, false);
        m_inspector.m_edits.push_back(e);
        m_inspector.m_updates.push_back([e, n](const Object& o) { e->show(shortNumber(n.get(o)), ""); });
        adw_action_row_add_suffix(ADW_ACTION_ROW(row), entry);
    }
    add(row);
}

void InspectorBuilder::toggle(const std::string& label, const std::string& prop, bool fallback, const std::string& tip) {
    GtkWidget* row = adw_switch_row_new();
    setTitle(row, label);
    if (!tip.empty()) gtk_widget_set_tooltip_text(row, tip.c_str());
    adw_switch_row_set_active(ADW_SWITCH_ROW(row), m_object.propBool(prop, fallback));
    Inspector* insp = &m_inspector;
    onSignal(row, "notify::active", [insp, row, prop] {
        if (insp->m_updating) return;
        bool on = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
        insp->edit([&](Object& o) { o.setProp(prop, on); });
    });
    live([row, prop, fallback](const Object& o) {
        bool on = o.propBool(prop, fallback);
        if (adw_switch_row_get_active(ADW_SWITCH_ROW(row)) != on) adw_switch_row_set_active(ADW_SWITCH_ROW(row), on);
    });
    add(row);
}

void InspectorBuilder::choice(const std::vector<std::string>& labels, std::function<int(const Object&)> get,
                              const std::vector<std::string>& tips, std::function<void(Object&, int)> pick) {
    AdwToggleGroup* group = ADW_TOGGLE_GROUP(adw_toggle_group_new());
    for (size_t i = 0; i < labels.size(); i++) {
        AdwToggle* t = adw_toggle_new();
        adw_toggle_set_label(t, labels[i].c_str());
        if (i < tips.size()) adw_toggle_set_tooltip(t, tips[i].c_str());
        adw_toggle_group_add(group, t);
    }
    adw_toggle_group_set_active(group, static_cast<guint>(get(m_object)));
    Inspector* insp = &m_inspector;
    onSignal(group, "notify::active", [insp, group, pick] {
        if (insp->m_updating) return;
        int index = static_cast<int>(adw_toggle_group_get_active(group));
        insp->edit([&](Object& o) { pick(o, index); });
    });
    live([group, get](const Object& o) {
        guint want = static_cast<guint>(get(o));
        if (adw_toggle_group_get_active(group) != want) adw_toggle_group_set_active(group, want);
    });
    m_group = nullptr;  // rows after it start a new group
    gtk_box_append(m_inspector.m_content.gobj(), GTK_WIDGET(group));
}

void InspectorBuilder::combo(const std::string& label, const std::vector<std::string>& items,
                             std::function<int(const Object&)> get, std::function<void(Object&, int)> set,
                             const std::string& tip) {
    GtkStringList* model = gtk_string_list_new(nullptr);
    for (const std::string& item : items) gtk_string_list_append(model, item.c_str());
    GtkWidget* row = adw_combo_row_new();
    setTitle(row, label);
    if (!tip.empty()) gtk_widget_set_tooltip_text(row, tip.c_str());
    adw_combo_row_set_model(ADW_COMBO_ROW(row), G_LIST_MODEL(model));
    g_object_unref(model);
    adw_combo_row_set_selected(ADW_COMBO_ROW(row), static_cast<guint>(get(m_object)));
    Inspector* insp = &m_inspector;
    onSignal(row, "notify::selected", [insp, row, set] {
        if (insp->m_updating) return;
        int index = static_cast<int>(adw_combo_row_get_selected(ADW_COMBO_ROW(row)));
        insp->edit([&](Object& o) { set(o, index); });
    });
    live([row, get](const Object& o) {
        guint want = static_cast<guint>(get(o));
        if (adw_combo_row_get_selected(ADW_COMBO_ROW(row)) != want) adw_combo_row_set_selected(ADW_COMBO_ROW(row), want);
    });
    add(row);
}

void InspectorBuilder::slider(const std::string& label, std::function<Limits(const Object&)> limits,
                              std::function<double(const Object&)> get, std::function<void(Object&, double)> set,
                              std::function<std::string(double)> format, const std::string& tip) {
    GtkWidget* row = adw_action_row_new();
    setTitle(row, label);
    if (!tip.empty()) gtk_widget_set_tooltip_text(row, tip.c_str());
    Limits lim = limits(m_object);
    GtkWidget* scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, lim.min, lim.max, lim.step > 0 ? lim.step : 0.01);
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
    gtk_widget_set_hexpand(scale, TRUE);
    gtk_widget_set_size_request(scale, 110, -1);
    gtk_widget_set_valign(scale, GTK_ALIGN_CENTER);
    gtk_range_set_value(GTK_RANGE(scale), get(m_object));
    GtkWidget* value = gtk_label_new("");
    gtk_widget_add_css_class(value, "numeric");
    gtk_widget_add_css_class(value, "dim-label");
    gtk_label_set_width_chars(GTK_LABEL(value), 6);
    gtk_label_set_xalign(GTK_LABEL(value), 1.0f);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), scale);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), value);
    Inspector* insp = &m_inspector;
    onSignal(scale, "value-changed", [insp, scale, value, set, get, format] {
        if (insp->m_updating) return;
        Object* o = insp->current();
        if (!o) return;
        set(*o, gtk_range_get_value(GTK_RANGE(scale)));
        gtk_label_set_text(GTK_LABEL(value), format(get(*o)).c_str());
        insp->sliderMoved();
    });
    live([insp, scale, value, limits, get, format](const Object& o) {
        Limits l = limits(o);
        GtkAdjustment* adj = gtk_range_get_adjustment(GTK_RANGE(scale));
        if (gtk_adjustment_get_lower(adj) != l.min || gtk_adjustment_get_upper(adj) != l.max)
            gtk_range_set_range(GTK_RANGE(scale), l.min, l.max);
        // Don't fight a drag in progress
        if (!insp->m_sliderRest.connected() && std::fabs(gtk_range_get_value(GTK_RANGE(scale)) - get(o)) > 1e-9)
            gtk_range_set_value(GTK_RANGE(scale), get(o));
        std::string text = format(get(o));
        if (gtk_label_get_text(GTK_LABEL(value)) != text) gtk_label_set_text(GTK_LABEL(value), text.c_str());
    });
    add(row);
}

void InspectorBuilder::value(const std::string& title, Text text, const std::string& tip) {
    GtkWidget* row = adw_action_row_new();
    setTitle(row, title);
    gtk_widget_add_css_class(row, "property");
    adw_action_row_set_subtitle_selectable(ADW_ACTION_ROW(row), TRUE);
    if (!tip.empty()) gtk_widget_set_tooltip_text(row, tip.c_str());
    live([row, text](const Object& o) {
        std::string t = text(o);
        const char* now = adw_action_row_get_subtitle(ADW_ACTION_ROW(row));
        if (!now || now != t) adw_action_row_set_subtitle(ADW_ACTION_ROW(row), t.c_str());
        gtk_widget_set_visible(row, !t.empty());
    });
    add(row);
}

void InspectorBuilder::line(Text text, Tone tone) {
    const char* css = tone == Tone::Warning ? "warning" : tone == Tone::Error ? "error" : tone == Tone::Mono ? "monospace" : "dim-label";
    GtkWidget* label = wrappedLabel(css);
    if (tone != Tone::Mono) gtk_widget_add_css_class(label, "caption");
    GtkWidget* row = plainRow(label);
    gtk_widget_set_visible(row, FALSE);
    live([row, label, text](const Object& o) { setVisibleText(row, label, text(o)); });
    add(row);
}

void InspectorBuilder::button(const std::string& label, const std::string& icon, std::function<void(Object&)> run,
                              const std::string& tip) {
    GtkWidget* row = adw_button_row_new();
    setTitle(row, label);
    if (!icon.empty()) adw_button_row_set_start_icon_name(ADW_BUTTON_ROW(row), icon.c_str());
    if (!tip.empty()) gtk_widget_set_tooltip_text(row, tip.c_str());
    Inspector* insp = &m_inspector;
    onSignal(row, "activated", [insp, run] {
        insp->flush();
        insp->edit(run);
    });
    add(row);
}

void InspectorBuilder::exampleButton(int count, std::function<void(Object&, int)> apply) {
    Inspector* insp = &m_inspector;
    button("Next Example", "media-skip-forward-symbolic", [insp, count, apply](Object& o) {
        insp->m_exampleIndex = (insp->m_exampleIndex + 1) % count;
        apply(o, insp->m_exampleIndex);
    }, "Try another formula (Ctrl+Z goes back)");
}

void InspectorBuilder::buttonGrid(const std::vector<GridButton>& buttons, int columns) {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
    Inspector* insp = &m_inspector;
    for (size_t i = 0; i < buttons.size(); i++) {
        GtkWidget* b = gtk_button_new_with_label(buttons[i].label.c_str());
        if (!buttons[i].tip.empty()) gtk_widget_set_tooltip_text(b, buttons[i].tip.c_str());
        auto run = buttons[i].run;
        onSignal(b, "clicked", [insp, run] {
            insp->flush();
            insp->edit(run);
        });
        gtk_grid_attach(GTK_GRID(grid), b, static_cast<int>(i) % columns, static_cast<int>(i) / columns, 1, 1);
    }
    m_group = nullptr;
    gtk_box_append(m_inspector.m_content.gobj(), grid);
}

void InspectorBuilder::widget(GtkWidget* child) { add(plainRow(child)); }

void InspectorBuilder::live(std::function<void(const Object&)> update) { m_inspector.m_updates.push_back(std::move(update)); }

void inspectFields(InspectorBuilder& b) {
    b.group("Fields");
    for (const auto& f : b.object().fields())
        if (b.object().fieldActive(f)) b.formula(f.key);
}

const InspectorType* inspectorFor(const std::string& typeId) {
    using namespace inspectors;
    static const std::map<std::string, InspectorType> table = {
        // InspectorsBasic.cpp
        {"vector", {vector, {"mode"}}},
        {"matrix", {matrix, {"mode", "size"}}},
        {"eigen", {eigen, {}}},
        {"function", {plot, {}}},
        {"surface", {plot, {}}},
        {"implicit", {plot, {}}},
        {"curve", {plot, {}}},
        {"parameter", {parameter, {"mode"}}},
        {"complex", {complexNumber, {}}},
        {"product", {product, {}}},
        // InspectorsCalculus.cpp
        {"doubleintegral", {doubleIntegral, {}}},
        {"lineintegral", {lineIntegral, {}}},
        {"frame", {frame, {}}},
        {"tangent", {tangent, {}}},
        {"tangentplane", {tangentPlane, {}}},
        {"velocity", {velocity, {}}},
        {"taylor", {taylor, {}}},
        {"newton", {newton, {}}},
        // InspectorsMore.cpp
        {"quaternion", {quaternion, {"act"}}},
        {"field", {field, {"domain", "lines", "particles"}}},
        {"epicycles", {epicycles, {}}},
        {"fourierseries", {fourierSeries, {}}},
    };
    auto it = table.find(typeId);
    return it == table.end() ? nullptr : &it->second;
}

} // namespace linalg::gtk
