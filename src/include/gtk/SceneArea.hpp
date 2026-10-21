#pragma once

// The 3D view widget of the GTK app: a Gtk::GLArea that gives SceneRenderer its GL context and turns GTK's
// input events into camera moves (drag = orbit, right-drag or Shift+drag = pan, wheel = zoom). A click (a press
// released before moving 4 px) selects the object under it; Ctrl+click adds to the selection.

#include <gtkmm/eventcontrollerscroll.h>
#include <gtkmm/gesturedrag.h>
#include <gtkmm/glarea.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace linalg { class Session; }

namespace linalg::gtk {

class SceneRenderer;

class SceneArea : public Gtk::GLArea {
public:
    explicit SceneArea(Session& session);
    ~SceneArea() override;

    SceneRenderer& renderer() { return *m_renderer; }
    // Something changed: draw a new frame
    void changed();
    // Called after a click changed the selection
    void onSceneChanged(std::function<void()> callback) { m_sceneChanged = std::move(callback); }
    // Fixed time per frame (screenshots that repeat exactly); 0 = real time
    void setFixedStep(double seconds) { m_fixedStep = seconds; }
    int framesDrawn() const { return m_frames; }
    // The user moved the camera (drag, pan, wheel) in the last moments (the tutorial checks it)
    bool cameraMovedRecently() const;
    // Why the view can't draw ("" when it can)
    const std::string& problem() const { return m_problem; }

protected:
    void on_realize() override;
    void on_unrealize() override;
    bool on_render(const Glib::RefPtr<Gdk::GLContext>& context) override;

private:
    void onDragBegin(double x, double y);
    void onDragUpdate(double offsetX, double offsetY);
    void onDragEnd(double offsetX, double offsetY);
    void pickAt(double x, double y, bool add);
    bool onScroll(double dx, double dy);

    // The scene is drawn into a multisampled framebuffer (4× MSAA, like the raylib app's window) and then copied
    // into the GLArea's own, which has no antialiasing
    bool prepareMultisample(int width, int height);
    void releaseMultisample();
    unsigned m_msFramebuffer = 0, m_msColor = 0, m_msDepth = 0;
    int m_msWidth = 0, m_msHeight = 0;
    bool m_msFailed = false;

    std::unique_ptr<SceneRenderer> m_renderer;
    Glib::RefPtr<Gtk::GestureDrag> m_drag;
    Glib::RefPtr<Gtk::EventControllerScroll> m_scroll;
    double m_lastX = 0.0, m_lastY = 0.0;
    double m_pressX = 0.0, m_pressY = 0.0;
    unsigned m_button = 0;
    bool m_ctrl = false;
    std::function<void()> m_sceneChanged;
    bool m_dragging = false;  // moved far enough to be a drag (not a click)
    long long m_cameraMovedAt = 0;  // g_get_monotonic_time() of the last camera move (µs)
    bool m_panning = false;
    bool m_dirty = true;
    double m_fixedStep = 0.0;
    std::int64_t m_lastFrameTime = 0;  // microseconds, from the frame clock
    int m_frames = 0;
    std::string m_problem;
};

} // namespace linalg::gtk
