#pragma once

// The tutorial in the GTK app: edu/Tutorial's lessons, run by a TutorialRunner.
// - The lesson list is a dialog (the ? button above the objects list, or Lessons in the main menu).
// - The current step is a card floating over the 3D view: the lesson, the step's text and its task, checked live
//   against the scene and ticked when done, with Back, Skip and Next.
// - The control the step is about (an anchor, MainWindow::anchorWidget) is outlined, and while its task is open a
//   popover points at it.
// Steps whose words name the raylib app's controls read differently here (gtk/LessonTexts.cpp).

#include "edu/Tutorial.hpp"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <sigc++/connection.h>
#include <functional>
#include <vector>

typedef struct _GtkWidget GtkWidget;

namespace linalg::gtk {

class MainWindow;

// A step's words for the GTK app; nullptr parts keep the lesson's own
struct LessonText {
    const char* body;
    const char* task;
    const char* hint;
};
const LessonText* gtkLessonText(const char* stepTitle);  // nullptr: the lesson's words fit

class LessonCard : public Gtk::Box {
public:
    explicit LessonCard(MainWindow& window);
    ~LessonCard() override;

    void showList();  // the lesson list dialog
    void start(int lesson, int step = 0);
    void leave();     // close the card
    // The step's setup when it is new, the task check, the card and the pointer; a timer runs it while a lesson is
    // open (the lesson walk calls it directly)
    void tick();
    TutorialRunner& runner() { return m_run; }
    // The finished lessons changed (a bit per lesson), to save them
    void onCompleted(std::function<void(unsigned int)> callback) { m_completed = std::move(callback); }

private:
    void showStep();     // fill the card for a new step
    void updateTask();   // done or not: the task box and the buttons
    void updatePointer();
    void hidePointer();
    void setHighlight(GtkWidget* widget);

    MainWindow& m_window;
    TutorialRunner m_run;
    sigc::connection m_timer;
    std::function<void(unsigned int)> m_completed;

    Gtk::Label m_caption, m_count, m_title, m_body, m_task, m_hint;
    Gtk::Box m_dots, m_taskBox, m_buttons;
    Gtk::Image m_taskIcon;
    Gtk::Button m_close, m_back, m_skip, m_next;
    std::vector<GtkWidget*> m_dotWidgets;

    GtkWidget* m_popover = nullptr;       // points at the anchor; its parent is this card
    GtkWidget* m_popoverLabel = nullptr;
    GtkWidget* m_highlighted = nullptr;   // the outlined anchor (a weak pointer: rows come and go)
    bool m_scrolled = false;              // the inspector was scrolled to this step's anchor
};

} // namespace linalg::gtk
