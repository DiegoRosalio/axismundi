#include "gtk/AddButton.hpp"
#include "app/Session.hpp"
#include "scene/Types.hpp"
#include <gtkmm/button.h>
#include <gtkmm/label.h>

namespace linalg::gtk {

AddButton::AddButton(Session& session) : m_session(session) {
    set_icon_name("list-add-symbolic");
    set_tooltip_text("Add an object (N)");
    m_scroll.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    m_scroll.set_propagate_natural_height(true);
    m_scroll.set_max_content_height(560);
    m_scroll.set_size_request(300, -1);
    m_popover.set_child(m_scroll);
    m_popover.signal_show().connect(sigc::mem_fun(*this, &AddButton::fill));
    set_popover(m_popover);
}

void AddButton::addHeading(Gtk::Box& box, const std::string& text) {
    auto* label = Gtk::make_managed<Gtk::Label>(text);
    label->set_xalign(0.0f);
    label->add_css_class("heading");
    label->set_margin_start(8);
    label->set_margin_top(box.get_first_child() ? 12 : 4);
    label->set_margin_bottom(2);
    box.append(*label);
}

void AddButton::addEntry(Gtk::Box& box, const std::string& label, const std::string& tip, const AddChoice& choice) {
    auto* button = Gtk::make_managed<Gtk::Button>();
    auto* text = Gtk::make_managed<Gtk::Label>(label);
    text->set_xalign(0.0f);
    text->set_ellipsize(Pango::EllipsizeMode::END);
    button->set_child(*text);
    button->add_css_class("flat");
    if (!tip.empty()) button->set_tooltip_text(tip);
    button->signal_clicked().connect([this, choice] {
        m_popover.popdown();
        if (m_choice) m_choice(choice);
    });
    box.append(*button);
}

void AddButton::fill() {
    auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
    box->set_margin(4);
    scene::SpaceKind kind = m_session.spaceKind();
    auto templates = addTemplates(m_session.scene(), kind);
    if (!templates.empty()) {
        addHeading(*box, "From selection");
        for (const auto& [label, choice] : templates) addEntry(*box, label, "", choice);
    }
    // Types in registry order, grouped by consecutive category
    std::string category;
    for (const scene::ObjectTypeInfo& t : scene::objectTypes()) {
        if (!(t.spaces & scene::spaceBit(kind))) continue;
        if (category != t.category) {
            category = t.category;
            addHeading(*box, category);
        }
        AddChoice c;
        c.typeId = t.id;
        addEntry(*box, t.label, t.description, c);
    }
    m_scroll.set_child(*box);
}

} // namespace linalg::gtk
