#include "gtk/SceneArea.hpp"
#include "gtk/SceneRenderer.hpp"
#include <epoxy/gl.h>
#include <gdkmm/frameclock.h>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace linalg::gtk {

namespace {
constexpr double kDragThreshold = 4.0;  // pixels before a press becomes a drag, as in the raylib app
constexpr int kSamples = 4;
}

SceneArea::SceneArea(Session& session) : m_renderer(std::make_unique<SceneRenderer>(session)) {
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
    m_drag->signal_drag_end().connect(sigc::mem_fun(*this, &SceneArea::onDragEnd));
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
    if (!has_error()) {
        m_renderer->stop();
        releaseMultisample();
    }
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
    int w = get_width() * scale, h = get_height() * scale;
    GLint target = 0;  // the GLArea's framebuffer, bound by GTK before this call
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &target);
    bool multisample = prepareMultisample(w, h);
    if (multisample) glBindFramebuffer(GL_FRAMEBUFFER, m_msFramebuffer);
    m_renderer->render(w, h, dt);
    if (multisample) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(target));
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(target));
    }
    // GTK composites the picture with its alpha. Blending (grid lines, labels) leaves alpha below 1, which let the
    // window show through and washed the grid out in the light theme: make the whole picture opaque.
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    m_dirty = false;
    m_frames++;
    return true;
}

bool SceneArea::prepareMultisample(int width, int height) {
    if (m_msFailed || width <= 0 || height <= 0) return false;
    if (m_msFramebuffer && width == m_msWidth && height == m_msHeight) return true;
    releaseMultisample();
    glGenFramebuffers(1, &m_msFramebuffer);
    glGenRenderbuffers(1, &m_msColor);
    glGenRenderbuffers(1, &m_msDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, m_msColor);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, kSamples, GL_RGBA8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, m_msDepth);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, kSamples, GL_DEPTH24_STENCIL8, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, m_msFramebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_msColor);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_msDepth);
    bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (!ok) {
        // Draw straight into the GLArea's framebuffer from now on (no antialiasing)
        std::cerr << "axismundi: no multisampled framebuffer, drawing without antialiasing\n";
        releaseMultisample();
        m_msFailed = true;
        return false;
    }
    m_msWidth = width;
    m_msHeight = height;
    return true;
}

void SceneArea::releaseMultisample() {
    if (m_msFramebuffer) glDeleteFramebuffers(1, &m_msFramebuffer);
    if (m_msColor) glDeleteRenderbuffers(1, &m_msColor);
    if (m_msDepth) glDeleteRenderbuffers(1, &m_msDepth);
    m_msFramebuffer = m_msColor = m_msDepth = 0;
    m_msWidth = m_msHeight = 0;
}

void SceneArea::onDragBegin(double x, double y) {
    m_lastX = m_lastY = 0.0;
    m_pressX = x;
    m_pressY = y;
    m_dragging = false;
    m_button = m_drag->get_current_button();
    Gdk::ModifierType state = m_drag->get_current_event_state();
    bool shift = (state & Gdk::ModifierType::SHIFT_MASK) == Gdk::ModifierType::SHIFT_MASK;
    m_ctrl = (state & Gdk::ModifierType::CONTROL_MASK) == Gdk::ModifierType::CONTROL_MASK;
    m_panning = m_button == 3 || shift;
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
    m_cameraMovedAt = g_get_monotonic_time();
    changed();
}

void SceneArea::onDragEnd(double, double) {
    if (!m_dragging && m_button == 1) pickAt(m_pressX, m_pressY, m_ctrl);
    m_dragging = false;
}

// The view draws at device pixels (get_scale_factor), so the click and the pick radius are scaled the same way
void SceneArea::pickAt(double x, double y, bool add) {
    Session& session = m_renderer->session();
    int scale = get_scale_factor();
    int w = get_width() * scale, h = get_height() * scale;
    Vector2 point{static_cast<float>(x * scale), static_cast<float>(y * scale)};
    scene::ObjectId hit = session.pickAt(point, w, h, 12.0f * static_cast<float>(scale), m_renderer->frameOptions(h));
    scene::Selection& sel = session.scene().selection(session.spaceKind());
    if (hit != scene::kNoObject) {
        if (add) sel.toggle(hit);
        else sel.set(hit);
    } else if (!add) {
        sel.clear();
    }
    changed();
    if (m_sceneChanged) m_sceneChanged();
}

bool SceneArea::onScroll(double, double dy) {
    m_renderer->zoom(static_cast<float>(-dy), static_cast<float>(get_height()));
    m_cameraMovedAt = g_get_monotonic_time();
    changed();
    return true;
}

bool SceneArea::cameraMovedRecently() const {
    return m_cameraMovedAt > 0 && g_get_monotonic_time() - m_cameraMovedAt < 300000;
}

} // namespace linalg::gtk

