#pragma once

// The pane on the right of the 3D view: three pages behind a view switcher.
// - Analysis: what the selection is and does (scene::analyze), as sections that open and close.
// - Steps: a worked calculation (math/Explain) for a topic: A · B, det M, z₁ · z₂...
// - Breakdown: the target matrix as rotate / stretch / rotate (/ move) steps, played in the 3D view.
// refresh() keeps the visible page up to date; it only rebuilds rows when their structure changed.

#include "math/Breakdown.hpp"
#include "math/Explain.hpp"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>
#include <gtkmm/togglebutton.h>
#include <map>
#include <string>
#include <vector>

typedef struct _GtkWidget GtkWidget;
typedef struct _AdwViewStack AdwViewStack;

namespace linalg {
class Session;
}

namespace linalg::gtk {

class InfoPane : public Gtk::Box {
public:
    explicit InfoPane(Session& session);

    void refresh();
    // "analysis", "steps" or "breakdown"
    void showPage(const std::string& name);
    std::string page() const;

    // The breakdown plays while its page is shown (and the pane is open, which the window tells)
    void setOpen(bool open) { m_open = open; }
    bool isOpen() const { return m_open; }
    bool breakdownActive() const;
    BreakdownPlayer& breakdown() { return m_player; }

private:
    GtkWidget* buildAnalysis();
    GtkWidget* buildSteps();
    GtkWidget* buildBreakdown();
    void refreshAnalysis();
    void refreshSteps();
    void refreshBreakdown();

    Session& m_session;
    AdwViewStack* m_stack = nullptr;
    bool m_open = false;
    bool m_wasActive = false;  // the breakdown was playing in the view at the last refresh

    // Analysis: one expander per section; rebuilt when the sections or their line counts change
    Gtk::Box* m_analysis = nullptr;
    std::string m_analysisKey;
    std::vector<Gtk::Label*> m_analysisLines;
    std::map<std::string, bool> m_expanded;  // the sections the user opened or closed, by key

    // Steps: the topic chips and the explanation's lines (rebuilt when the text changes)
    Gtk::Box* m_topics = nullptr;
    Gtk::Box* m_steps = nullptr;
    std::vector<Gtk::ToggleButton*> m_topicButtons;
    bool m_topicsComplex = false;
    int m_matrixTopic = kDefaultMatrixTopic;
    int m_complexTopic = 0;
    std::string m_stepsText;

    // Breakdown
    BreakdownPlayer m_player;
    Gtk::Box* m_breakdownSteps = nullptr;
    Gtk::Label* m_breakdownNote = nullptr;
    Gtk::Scale* m_breakdownScale = nullptr;
    Gtk::Button* m_playButton = nullptr;
    std::string m_breakdownKey;
    struct StepRow {
        Gtk::Label* badge;
        Gtk::Widget* row;
        Gtk::Widget* bar;  // a Gtk::ProgressBar
    };
    std::vector<StepRow> m_stepRows;
    bool m_seeking = false;  // the scale is being set by refresh, not dragged
};

} // namespace linalg::gtk
