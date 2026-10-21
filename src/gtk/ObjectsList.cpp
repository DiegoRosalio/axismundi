#include "gtk/ObjectsList.hpp"
#include "app/Session.hpp"
#include "scene/Summaries.hpp"
#include "scene/Types.hpp"
#include "view/SceneView.hpp"
#include <glibmm/main.h>
#include <gtkmm/adjustment.h>
#include <glibmm/markup.h>
#include <algorithm>
#include <cstdio>
#include <set>

namespace linalg::gtk {

namespace {

std::string hexOf(Color c) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", c.r, c.g, c.b);
    return buf;
}

std::string escapeMarkup(const std::string& text) { return Glib::Markup::escape_text(text); }

} // namespace

ObjectsList::ObjectsList(Session& session) : Gtk::Box(Gtk::Orientation::VERTICAL), m_session(session) {
    m_list.set_selection_mode(Gtk::SelectionMode::MULTIPLE);
    m_list.add_css_class("navigation-sidebar");
    m_list.signal_selected_rows_changed().connect(sigc::mem_fun(*this, &ObjectsList::onSelectionChanged));
    m_empty.set_text("No objects yet.\nAdd one with the + button.");
    m_empty.set_justify(Gtk::Justification::CENTER);
    m_empty.add_css_class("dim-label");
    m_empty.set_margin(24);
    m_empty.set_vexpand(true);
    m_empty.set_valign(Gtk::Align::CENTER);
    m_scroll.set_child(m_list);
    m_scroll.set_vexpand(true);
    m_scroll.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    append(m_scroll);
    append(m_empty);
    refresh();
}

std::string ObjectsList::structureKey() const {
    std::string key = std::to_string(static_cast<int>(m_session.spaceKind())) + ";";
    for (const auto& o : m_session.scene().space(m_session.spaceKind()).objects()) {
        key += std::to_string(o->id()) + "|" + o->name() + "|" + o->typeId() + "|" + (o->visible ? "1" : "0") + "|" +
               hexOf(view::SceneView::colorOf(*o)) + ";";
    }
    return key;
}

void ObjectsList::refresh() {
    std::string key = structureKey();
    if (key != m_key) rebuild();
    // Values change all the time (the clock, edits elsewhere): only the summaries are updated
    scene::Space& space = m_session.scene().space(m_session.spaceKind());
    for (Row& r : m_rows) {
        const scene::SceneObject* o = space.find(r.id);
        if (!o) continue;
        std::string text = scene::summaryOf(*o);
        if (r.summary->get_text() != text) r.summary->set_text(text);
        bool error = !o->error().empty();
        if (error != r.summary->has_css_class("error")) {
            if (error) r.summary->add_css_class("error");
            else r.summary->remove_css_class("error");
        }
    }
    syncSelection();
}

void ObjectsList::rebuild() {
    m_syncing = true;
    while (Gtk::Widget* child = m_list.get_first_child()) m_list.remove(*child);
    m_rows.clear();
    m_key = structureKey();
    scene::Space& space = m_session.scene().space(m_session.spaceKind());
    for (const auto& obj : space.objects()) {
        const scene::SceneObject& o = *obj;
        auto* row = Gtk::make_managed<Gtk::ListBoxRow>();
        auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 10);
        box->set_margin_start(6);
        box->set_margin_end(4);
        box->set_margin_top(4);
        box->set_margin_bottom(4);

        auto* dot = Gtk::make_managed<Gtk::Label>();
        dot->set_markup("<span foreground=\"" + hexOf(view::SceneView::colorOf(o)) + "\" size=\"x-large\">●</span>");
        box->append(*dot);

        auto* texts = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
        texts->set_hexpand(true);
        auto* name = Gtk::make_managed<Gtk::Label>();
        const scene::ObjectTypeInfo* type = scene::findType(o.typeId());
        name->set_markup("<b>" + escapeMarkup(o.name()) + "</b>  <small>" + escapeMarkup(type ? type->label : o.typeId()) +
                         "</small>");
        name->set_xalign(0.0f);
        name->set_ellipsize(Pango::EllipsizeMode::END);
        auto* summary = Gtk::make_managed<Gtk::Label>(scene::summaryOf(o));
        summary->set_xalign(0.0f);
        summary->set_ellipsize(Pango::EllipsizeMode::END);
        summary->add_css_class("dim-label");
        summary->add_css_class("caption");
        texts->append(*name);
        texts->append(*summary);
        box->append(*texts);

        auto* eye = Gtk::make_managed<Gtk::ToggleButton>();
        eye->set_icon_name(o.visible ? "view-reveal-symbolic" : "view-conceal-symbolic");
        eye->set_active(o.visible);
        eye->add_css_class("flat");
        eye->set_valign(Gtk::Align::CENTER);
        eye->set_tooltip_text("Show or hide");
        scene::ObjectId id = o.id();
        eye->signal_toggled().connect([this, id, eye] {
            scene::SceneObject* target = m_session.scene().space(m_session.spaceKind()).find(id);
            if (!target || target->visible == eye->get_active()) return;
            target->visible = eye->get_active();
            m_session.scene().commit();
            m_key.clear();  // rebuilt on the next refresh (icon, key)
            if (m_changed) m_changed();
        });
        box->append(*eye);

        row->set_child(*box);
        m_list.append(*row);
        m_rows.push_back({id, row, summary, eye});
    }
    m_scroll.set_visible(!m_rows.empty());
    m_empty.set_visible(m_rows.empty());
    m_syncing = false;
}

void ObjectsList::syncSelection() {
    const scene::Selection& sel = m_session.scene().selection(m_session.spaceKind());
    std::set<scene::ObjectId> chosen(sel.ids.begin(), sel.ids.end());
    m_syncing = true;
    for (Row& r : m_rows) {
        bool want = chosen.count(r.id) > 0;
        if (r.row->is_selected() != want) {
            if (want) m_list.select_row(*r.row);
            else m_list.unselect_row(r.row);
        }
    }
    m_syncing = false;
    // A new primary (added, picked in the view) comes into view
    if (sel.primary() != m_primary) {
        m_primary = sel.primary();
        if (m_primary != scene::kNoObject) {
            scene::ObjectId id = m_primary;
            Glib::signal_idle().connect_once([this, id] { scrollTo(id); });  // once the rows have their sizes
        }
    }
}

void ObjectsList::scrollTo(scene::ObjectId id) {
    for (Row& r : m_rows) {
        if (r.id != id) continue;
        graphene_rect_t bounds;
        if (gtk_widget_compute_bounds(GTK_WIDGET(r.row->gobj()), GTK_WIDGET(m_list.gobj()), &bounds))
            m_scroll.get_vadjustment()->clamp_page(bounds.origin.y, bounds.origin.y + bounds.size.height);
    }
}

void ObjectsList::onSelectionChanged() {
    if (m_syncing) return;
    scene::Selection& sel = m_session.scene().selection(m_session.spaceKind());
    std::set<scene::ObjectId> now;
    for (Row& r : m_rows)
        if (r.row->is_selected()) now.insert(r.id);
    // Keep the order of what stays selected; rows just added go last (the last one becomes the primary)
    std::vector<scene::ObjectId> kept, added;
    for (scene::ObjectId id : sel.ids)
        if (now.count(id)) kept.push_back(id);
    for (Row& r : m_rows)
        if (now.count(r.id) && std::find(sel.ids.begin(), sel.ids.end(), r.id) == sel.ids.end()) added.push_back(r.id);
    sel.clear();
    for (scene::ObjectId id : kept) sel.toggle(id);
    for (scene::ObjectId id : added) sel.toggle(id);
    if (m_changed) m_changed();
}

} // namespace linalg::gtk
