#pragma once

// The GTK app's screenshot tour (--tour DIR): a scripted walk that changes the view step by step, saves a PNG
// per step and quits, like the raylib app's app/Tour. Each step waits until the 3D view has drawn its change.
// The lesson walk (--lesson-walk DIR) goes through every tutorial step instead: it enters the step (a PNG with the
// card and the pointer), checks that the task isn't done yet, does it the scripted way (edu/TutorialSolutions),
// lets the card check it (another PNG) and prints WALK / SOLVED lines for tools/dev/lesson-walk-gtk.sh.

#include <functional>
#include <gtkmm/application.h>
#include <string>
#include <vector>

namespace linalg::gtk {

class MainWindow;

class Tour {
public:
    Tour(MainWindow& window, Glib::RefPtr<Gtk::Application> app, std::string directory, bool lessonWalk = false);
    void start();

private:
    struct Step {
        Step(std::string n, std::function<void()> a, int w = 0) : name(std::move(n)), action(std::move(a)), wait(w) {}
        std::string name;
        std::function<void()> action;
        int wait;  // frames to wait at least before the PNG (an animation playing to its end)
    };
    bool tick();
    void addLessonWalk();

    MainWindow& m_window;
    Glib::RefPtr<Gtk::Application> m_app;
    std::string m_directory;
    std::vector<Step> m_steps;
    std::size_t m_next = 0;
    bool m_waiting = false;  // the current step's action ran; waiting for its frame
    int m_ticks = 0;
    int m_minTicks = 20;     // frames after the view drew the change before the PNG
    int m_framesAtAction = 0;
};

} // namespace linalg::gtk
