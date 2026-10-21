#include "gtk/ViewMenu.hpp"
#include "app/Session.hpp"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/popover.h>
#include <gtkmm/scale.h>
#include <gtkmm/separator.h>
#include <gtkmm/switch.h>
#include <gtkmm/togglebutton.h>
#include <cstdio>

namespace linalg::gtk {

ViewMenu::ViewMenu(Session& session) : m_session(session) {
    set_icon_name("view-reveal-symbolic");
    set_tooltip_text("What the view shows, its style and the camera");
    auto* popover = Gtk::make_managed<Gtk::Popover>();
    set_popover(*popover);
    popover->signal_show().connect([this] { rebuild(); });
}

void ViewMenu::changed() {
    if (m_changed) m_changed();
}

void ViewMenu::rebuild() {
    ViewSettings& v = m_session.view();
    bool space3D = m_session.mode() == AppMode::Space3D;
    auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
    box->set_margin(12);
    box->set_size_request(300, -1);

    auto heading = [&](const char* text) {
        auto* l = Gtk::make_managed<Gtk::Label>(text);
        l->set_xalign(0.0f);
        l->add_css_class("heading");
        box->append(*l);
    };
    // A switch row bound to a setting
    auto toggle = [&](const char* text, bool& value, const char* tip) {
        auto* row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
        auto* l = Gtk::make_managed<Gtk::Label>(text);
        l->set_xalign(0.0f);
        l->set_hexpand(true);
        auto* sw = Gtk::make_managed<Gtk::Switch>();
        sw->set_active(value);
        sw->set_valign(Gtk::Align::CENTER);
        row->set_tooltip_text(tip);
        sw->property_active().signal_changed().connect([this, sw, &value] {
            value = sw->get_active();
            changed();
        });
        row->append(*l);
        row->append(*sw);
        box->append(*row);
    };
    // Linked toggle buttons: one of them, or each on its own
    auto linked = [&]() {
        auto* row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
        row->add_css_class("linked");
        row->set_homogeneous(true);
        box->append(*row);
        return row;
    };
    auto choice = [&](Gtk::Box* row, const char* text, const char* tip, bool active, Gtk::ToggleButton* group,
                      std::function<void()> pick) {
        auto* b = Gtk::make_managed<Gtk::ToggleButton>(text);
        b->set_tooltip_text(tip);
        if (group) b->set_group(*group);
        b->set_active(active);
        b->signal_toggled().connect([this, b, pick] {
            if (!b->get_active()) return;
            pick();
            changed();
        });
        row->append(*b);
        return b;
    };

    heading("Show");
    toggle("Grid", v.showGrid, "The endless grid");
    toggle("Axes", v.showAxes, "The x, y and z axes (real and imaginary in the complex plane)");
    toggle("Labels", v.showLabels, "Names and values next to the objects");
    toggle("Numbers on Axes", v.showNumbers, "Tick numbers along the axes");

    heading("Grid");
    Gtk::Box* spacing = linked();
    auto* autoSpacing = choice(spacing, "Auto", "Lines get closer as you zoom in (0.1, 0.01...) and wider as you zoom out",
                               v.gridAuto, nullptr, [&v] { v.gridAuto = true; });
    choice(spacing, "1 Unit", "A line every unit, stronger every 5: easy to count", !v.gridAuto, autoSpacing,
           [&v] { v.gridAuto = false; });
    if (space3D) {
        Gtk::Box* planes = linked();
        struct Plane {
            const char* text;
            bool* value;
            const char* tip;
        };
        for (Plane p : {Plane{"Floor xy", &v.gridFloor, "The grid on the floor (z = 0)"},
                        Plane{"Wall xz", &v.gridWallXZ, "A grid on the wall y = 0"},
                        Plane{"Wall yz", &v.gridWallYZ, "A grid on the wall x = 0"}}) {
            auto* b = Gtk::make_managed<Gtk::ToggleButton>(p.text);
            b->set_tooltip_text(p.tip);
            b->set_active(*p.value);
            bool* value = p.value;
            b->signal_toggled().connect([this, b, value] {
                *value = b->get_active();
                changed();
            });
            planes->append(*b);
        }
    } else {
        Gtk::Box* polar = linked();
        auto* square = choice(polar, "Square", "Lines parallel to the real and imaginary axes", !v.gridPolar, nullptr,
                              [&v] { v.gridPolar = false; });
        choice(polar, "Polar", "Circles around 0 and rays every 15°", v.gridPolar, square, [&v] { v.gridPolar = true; });
    }

    heading("Style");
    auto* styles = Gtk::make_managed<Gtk::Grid>();
    styles->set_column_homogeneous(true);
    styles->set_row_spacing(6);
    styles->set_column_spacing(6);
    Gtk::ToggleButton* first = nullptr;
    for (int i = 0; i < view::kStyleCount; i++) {
        auto id = static_cast<view::StyleId>(i);
        auto* b = Gtk::make_managed<Gtk::ToggleButton>(view::styleName(id));
        b->set_tooltip_text(view::styleDescription(id));
        if (first) b->set_group(*first);
        else first = b;
        b->set_active(v.style == id);
        b->signal_toggled().connect([this, b, id, &v] {
            if (!b->get_active()) return;
            v.style = id;
            changed();
        });
        styles->attach(*b, i % 2, i / 2);
    }
    box->append(*styles);

    auto* lengthRow = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    auto* lengthLabel = Gtk::make_managed<Gtk::Label>("Axis Length");
    auto* length = Gtk::make_managed<Gtk::Scale>(Gtk::Orientation::HORIZONTAL);
    length->set_range(1.0, 20.0);
    length->set_value(v.axisLength);
    length->set_hexpand(true);
    length->set_digits(1);
    length->set_draw_value(true);
    length->set_value_pos(Gtk::PositionType::RIGHT);
    length->signal_value_changed().connect([this, length, &v] {
        v.axisLength = static_cast<float>(length->get_value());
        changed();
    });
    lengthRow->append(*lengthLabel);
    lengthRow->append(*length);
    box->append(*lengthRow);

    box->append(*Gtk::make_managed<Gtk::Separator>());
    heading("Camera");
    auto* camera = Gtk::make_managed<Gtk::Grid>();
    camera->set_column_homogeneous(true);
    camera->set_row_spacing(6);
    camera->set_column_spacing(6);
    struct CameraButton {
        const char* text;
        int view;
        const char* tip;
    };
    std::vector<CameraButton> buttons = {{"Reset", 0, "Back to the default viewpoint for this space"}};
    if (space3D) {
        buttons.push_back({"Front", 1, "Look along the y axis, from the front"});
        buttons.push_back({"Right", 2, "Look along the x axis, from the right"});
        buttons.push_back({"Top", 3, "Look straight down at the floor"});
    }
    buttons.push_back({"Focus", 4, "Turn around the selected object: it becomes the orbit center"});
    for (size_t i = 0; i < buttons.size(); i++) {
        auto* b = Gtk::make_managed<Gtk::Button>(buttons[i].text);
        b->set_tooltip_text(buttons[i].tip);
        int view = buttons[i].view;
        b->signal_clicked().connect([this, view] {
            if (m_camera) m_camera(view);
        });
        camera->attach(*b, static_cast<int>(i % 3), static_cast<int>(i / 3));
    }
    box->append(*camera);

    get_popover()->set_child(*box);
}

} // namespace linalg::gtk
