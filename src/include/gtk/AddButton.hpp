#pragma once

// The "+" button of the objects list: a popover with what can be added to the current space, the
// "From selection" templates first (scene/Templates: a + b, M·v, morphs...), then every object type by category.

#include "scene/Templates.hpp"
#include <gtkmm/box.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/popover.h>
#include <gtkmm/scrolledwindow.h>
#include <functional>

namespace linalg {
class Session;
}

namespace linalg::gtk {

class AddButton : public Gtk::MenuButton {
public:
    explicit AddButton(Session& session);
    // Called with the chosen entry (the popover is closed by then)
    void onChoice(std::function<void(const AddChoice&)> callback) { m_choice = std::move(callback); }

private:
    void fill();  // rebuilt every time the popover opens: the templates follow the selection
    void addEntry(Gtk::Box& box, const std::string& label, const std::string& tip, const AddChoice& choice);
    void addHeading(Gtk::Box& box, const std::string& text);

    Session& m_session;
    Gtk::Popover m_popover;
    Gtk::ScrolledWindow m_scroll;
    std::function<void(const AddChoice&)> m_choice;
};

} // namespace linalg::gtk
