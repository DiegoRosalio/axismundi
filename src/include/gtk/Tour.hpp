#pragma once

// The GTK app's screenshot tour (--tour DIR): a scripted walk that changes the view step by step, saves a PNG
// per step and quits, like the raylib app's app/Tour. Each step waits until the 3D view has drawn its change.

#include <functional>
#include <gtkmm/application.h>
#include <string>
#include <vector>

namespace linalg::gtk {

class MainWindow;

class Tour {
public:
    Tour(MainWindow& window, Glib::RefPtr<Gtk::Application> app, std::string directory);
    void start();

private:
    struct Step {
        std::string name;
        std::function<void()> action;
    };
    bool tick();

    MainWindow& m_window;
    Glib::RefPtr<Gtk::Application> m_app;
    std::string m_directory;
    std::vector<Step> m_steps;
    std::size_t m_next = 0;
    bool m_waiting = false;  // the current step's action ran; waiting for its frame
    int m_ticks = 0;
    int m_framesAtAction = 0;
};

} // namespace linalg::gtk
