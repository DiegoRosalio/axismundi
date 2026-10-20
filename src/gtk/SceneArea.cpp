#include "gtk/SceneArea.hpp"
#include "gtk/SceneRenderer.hpp"
#include <gdkmm/frameclock.h>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace linalg::gtk {

namespace {
constexpr double kDragThreshold = 4.0;  // pixels before a press becomes a drag, as in the raylib app
}

SceneArea::SceneArea(scene::Scene& scene) : m_renderer(std::make_unique<SceneRenderer>(scene)) {
    set_required_version(3, 3);
    set_allowed_apis(Gdk::GLApi::GL);  // rlgl is built for desktop OpenGL 3.3, not GLES
    set_has_depth_buffer(true);
    set_hexpand(true);
    set_vexpand(true);
    set_focusable(true);

    m_drag = Gtk::GestureDrag::create();
    m_drag->set_button(0);  // every button: left/middle orbit, right pans
    m_drag->signal_drag_begin().connect(sigc::mem_fun(*this, &SceneArea::onDragBegin));
    m_drag->signal_drag_update().connect(sigc::mem_fun(*this, &SceneArea::onDragUpdate));
    add_controller(m_drag);

    m_scroll = Gtk::EventControllerScroll::create();
    m_scroll->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
    m_scroll->signal_scroll().connect(sigc::mem_fun(*this, &SceneArea::onScroll), false);
    add_controller(m_scroll);

    // Frames only when needed: after a change, or every frame while the scene clock plays
    add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        if (m_dirty || m_renderer->animating()) queue_render();
        return true;
    });
}

SceneArea::~SceneArea() = default;

void SceneArea::changed() {
    m_dirty = true;
    queue_render();
}

void SceneArea::on_realize() {
    Gtk::GLArea::on_realize();
    make_current();
    if (has_error()) {
        m_problem = "OpenGL 3.3 is not available";
        std::cerr << "axismundi: " << m_problem << "\n";
        return;
    }
    int scale = get_scale_factor();
    std::string error;
    if (!m_renderer->start(std::max(1, get_width() * scale), std::max(1, get_height() * scale), error)) {
        m_problem = error;
        std::cerr << "axismundi: the 3D view can't start: " << error << "\n";
    }
}

void SceneArea::on_unrealize() {
    make_current();
    if (!has_error()) m_renderer->stop();
    Gtk::GLArea::on_unrealize();
}

bool SceneArea::on_render(const Glib::RefPtr<Gdk::GLContext>&) {
    if (!m_renderer->started()) return true;
    double dt = m_fixedStep;
    if (dt <= 0.0) {
        std::int64_t now = get_frame_clock() ? get_frame_clock()->get_frame_time() : 0;
        dt = m_lastFrameTime > 0 ? std::clamp((now - m_lastFrameTime) / 1e6, 0.0, 0.1) : 0.0;
        m_lastFrameTime = now;
    }
    int scale = get_scale_factor();
    m_renderer->render(get_width() * scale, get_height() * scale, dt);
    m_dirty = false;
    m_frames++;
    return true;
}

void SceneArea::onDragBegin(double, double) {
    m_lastX = m_lastY = 0.0;
    m_dragging = false;
    unsigned button = m_drag->get_current_button();
    bool shift = (m_drag->get_current_event_state() & Gdk::ModifierType::SHIFT_MASK) == Gdk::ModifierType::SHIFT_MASK;
    m_panning = button == 3 || shift;
    grab_focus();
}

void SceneArea::onDragUpdate(double offsetX, double offsetY) {
    if (!m_dragging) {
        if (std::hypot(offsetX, offsetY) < kDragThreshold) return;
        m_dragging = true;  // the movement so far counts too
    }
    float dx = static_cast<float>(offsetX - m_lastX), dy = static_cast<float>(offsetY - m_lastY);
    m_lastX = offsetX;
    m_lastY = offsetY;
    float h = static_cast<float>(get_height());
    if (m_panning) m_renderer->pan(dx, dy, h);
    else m_renderer->orbit(dx, dy, h);
    changed();
}

bool SceneArea::onScroll(double, double dy) {
    m_renderer->zoom(static_cast<float>(-dy), static_cast<float>(get_height()));
    changed();
    return true;
}

} // namespace linalg::gtk
