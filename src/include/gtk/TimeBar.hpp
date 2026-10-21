#pragma once

// The scene clock, as a bar under the 3D view: play/pause, back to 0, the time as a slider you can drag, the
// time read out, loop, and a popover with the speed and the length. Parameters set to Animate or Formula (and
// anything whose formulas use t) follow it. refresh() keeps it in step with the scene.

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>
#include <gtkmm/switch.h>
#include <gtkmm/togglebutton.h>
#include <functional>

namespace linalg {
class Session;
}

namespace linalg::gtk {

class TimeBar : public Gtk::Box {
public:
    explicit TimeBar(Session& session);

    void refresh();
    // The clock changed here: the view must draw again
    void onChanged(std::function<void()> callback) { m_changed = std::move(callback); }

private:
    void changed();

    Session& m_session;
    Gtk::Button m_play;
    Gtk::Button m_restart;
    Gtk::Scale m_time;
    Gtk::Label m_readout;
    Gtk::ToggleButton m_loop;
    Gtk::Scale m_speed;
    Gtk::Label m_speedValue;
    Gtk::Switch m_autoLength;
    Gtk::Scale m_length;
    Gtk::Label m_lengthValue;
    bool m_syncing = false;  // values set by refresh, not by the user
    std::function<void()> m_changed;
};

} // namespace linalg::gtk
