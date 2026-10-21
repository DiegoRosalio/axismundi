#pragma once

// The 3D view of the GTK app, without any GTK: raylib's GL layer (rlgl) started inside a GL context that
// someone else made (a Gtk::GLArea), then the session's drawing (grid, objects, labels), as the raylib app does.
// Only raylib headers here: the GTK widget around it is gtk/SceneArea.

#include "app/Session.hpp"
#include <string>

namespace linalg::gtk {

class SceneRenderer {
public:
    explicit SceneRenderer(Session& session) : m_session(session) {}

    // With the GL context current: load the GL functions and start rlgl. False (and why) if that fails.
    bool start(int pixelWidth, int pixelHeight, std::string& error);
    // With the GL context current: free GPU memory (meshes, shaders, font atlases)
    void stop();
    bool started() const { return m_started; }

    // Evaluate the scene (advancing the clock by dt while it plays) and draw a frame into the bound framebuffer
    void render(int pixelWidth, int pixelHeight, double dt);

    // Camera input in the widget's coordinates (logical pixels)
    void orbit(float dx, float dy, float viewHeight);
    void pan(float dx, float dy, float viewHeight);
    void zoom(float wheelSteps, float viewHeight);

    // Something moves on its own (the clock plays), so frames must keep coming
    bool animating() const { return m_session.scene().timeline().playing; }
    Session& session() { return m_session; }

private:
    Session& m_session;
    bool m_started = false;
};

} // namespace linalg::gtk
