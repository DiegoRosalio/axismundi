#include "gtk/TimeBar.hpp"
#include "app/Session.hpp"
#include <gtkmm/adjustment.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/popover.h>
#include <cmath>
#include <cstdio>

namespace linalg::gtk {

namespace {

std::string format(const char* pattern, double a, double b = 0.0) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), pattern, a, b);
    return buf;
}

// "Label  [scale] value" as one row of the popover
Gtk::Box* popoverRow(const char* label, Gtk::Widget& control, Gtk::Label& value) {
    auto* row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    auto* l = Gtk::make_managed<Gtk::Label>(label);
    l->set_xalign(0.0f);
    l->set_width_chars(7);
    row->append(*l);
    control.set_hexpand(true);
    row->append(control);
    value.set_width_chars(5);
    value.set_xalign(1.0f);
    value.add_css_class("numeric");
    value.add_css_class("dim-label");
    row->append(value);
    return row;
}

} // namespace

TimeBar::TimeBar(Session& session) : Gtk::Box(Gtk::Orientation::HORIZONTAL, 6), m_session(session) {
    set_margin_start(6);
    set_margin_end(6);
    set_margin_top(4);
    set_margin_bottom(4);

    m_play.set_icon_name("media-playback-start-symbolic");
    m_play.set_tooltip_text("Play or pause the scene clock (Space in the view). Parameters set to Animate or Formula "
                            "follow it; sliders stay where they are.");
    m_play.add_css_class("flat");
    m_play.signal_clicked().connect([this] {
        m_session.scene().togglePlay();
        changed();
    });
    m_restart.set_icon_name("media-skip-backward-symbolic");
    m_restart.set_tooltip_text("Back to t = 0");
    m_restart.add_css_class("flat");
    m_restart.signal_clicked().connect([this] {
        m_session.scene().timeline().time = 0.0;
        changed();
    });

    m_time.set_draw_value(false);
    m_time.set_hexpand(true);
    m_time.set_range(0.0, 10.0);
    m_time.set_tooltip_text("Drag to see any moment: time-driven parameters jump to their value at that time");
    m_time.signal_value_changed().connect([this] {
        if (m_syncing) return;
        m_session.scene().timeline().time = m_time.get_value();
        changed();
    });
    m_readout.add_css_class("numeric");
    m_readout.set_width_chars(13);
    m_readout.set_xalign(1.0f);

    m_loop.set_icon_name("media-playlist-repeat-symbolic");
    m_loop.add_css_class("flat");
    m_loop.set_tooltip_text("Loop: at the end, start again from 0");
    m_loop.signal_toggled().connect([this] {
        if (m_syncing) return;
        m_session.scene().timeline().loop = m_loop.get_active();
        changed();
    });

    // Speed and length in a popover
    auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10);
    box->set_margin(12);
    box->set_size_request(280, -1);
    m_speed.set_range(0.1, 4.0);
    m_speed.set_increments(0.1, 0.5);
    m_speed.set_draw_value(false);
    m_speed.signal_value_changed().connect([this] {
        if (m_syncing) return;
        m_session.scene().timeline().speed = static_cast<float>(std::round(m_speed.get_value() * 10.0) / 10.0);
        changed();
    });
    box->append(*popoverRow("Speed", m_speed, m_speedValue));
    auto* autoRow = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    auto* autoLabel = Gtk::make_managed<Gtk::Label>("Automatic length");
    autoLabel->set_xalign(0.0f);
    autoLabel->set_hexpand(true);
    autoLabel->set_tooltip_text("As long as the longest Once or Ping-pong animation (10 s if there is none)");
    autoRow->append(*autoLabel);
    m_autoLength.set_valign(Gtk::Align::CENTER);
    autoRow->append(m_autoLength);
    box->append(*autoRow);
    m_autoLength.property_active().signal_changed().connect([this] {
        if (m_syncing) return;
        scene::Scene& sc = m_session.scene();
        sc.timeline().length = m_autoLength.get_active() ? 0.0 : std::ceil(sc.autoLength());
        changed();
    });
    m_length.set_range(1.0, 120.0);
    m_length.set_increments(1.0, 10.0);
    m_length.set_draw_value(false);
    m_length.signal_value_changed().connect([this] {
        if (m_syncing) return;
        m_session.scene().timeline().length = std::round(m_length.get_value());
        changed();
    });
    box->append(*popoverRow("Length", m_length, m_lengthValue));
    auto* popover = Gtk::make_managed<Gtk::Popover>();
    popover->set_child(*box);
    auto* more = Gtk::make_managed<Gtk::MenuButton>();
    more->set_icon_name("preferences-system-time-symbolic");
    more->set_tooltip_text("Speed and length of the scene clock");
    more->add_css_class("flat");
    more->set_popover(*popover);

    append(m_play);
    append(m_restart);
    append(m_time);
    append(m_readout);
    append(m_loop);
    append(*more);
    refresh();
}

void TimeBar::changed() {
    refresh();
    if (m_changed) m_changed();
}

void TimeBar::refresh() {
    scene::Scene& sc = m_session.scene();
    scene::Timeline& tl = sc.timeline();
    double length = sc.timelineLength();
    m_syncing = true;
    m_play.set_icon_name(tl.playing ? "media-playback-pause-symbolic" : "media-playback-start-symbolic");
    if (m_time.get_adjustment()->get_upper() != length) m_time.set_range(0.0, length);
    double shown = std::min(tl.time, length);
    if (std::fabs(m_time.get_value() - shown) > 1e-6) m_time.set_value(shown);
    std::string readout = format("t %.2f / %.3g s", tl.time, length);
    if (m_readout.get_text() != readout) m_readout.set_text(readout);
    if (m_loop.get_active() != tl.loop) m_loop.set_active(tl.loop);
    if (std::fabs(m_speed.get_value() - tl.speed) > 1e-6) m_speed.set_value(tl.speed);
    m_speedValue.set_text(format("%.1f×", tl.speed));
    bool automatic = tl.length <= 0.0;
    if (m_autoLength.get_active() != automatic) m_autoLength.set_active(automatic);
    m_length.set_sensitive(!automatic);
    if (std::fabs(m_length.get_value() - length) > 1e-6) m_length.set_value(length);
    m_lengthValue.set_text(format("%.3g s", length));
    m_syncing = false;
}

} // namespace linalg::gtk
