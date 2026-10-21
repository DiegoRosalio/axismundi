#pragma once

// One text field bound to something in the scene: a formula field, an object's name, a number property.
// - Enter, or leaving the field, commits what was typed (one undo step); Esc goes back to the saved text.
// - While a formula is being typed, the field turns red when it doesn't parse.
// - The saved text's problem (e.g. "unknown name 'q'") colors the field and is its tooltip.
// It works with any GtkEditable widget (Gtk::Entry, AdwEntryRow) and lives as long as its widget.

#include <functional>
#include <string>

typedef struct _GtkWidget GtkWidget;

namespace linalg::gtk {

class EditBinding {
public:
    using Commit = std::function<bool(const std::string&)>;  // false = rejected: the saved text comes back

    // Attaches to `widget` (deleted together with it); `formula` turns on the live syntax check
    static EditBinding* attach(GtkWidget* widget, Commit commit, bool formula);

    // Show the scene's text and its problem ("" = none), unless the user is typing
    void show(const std::string& text, const std::string& error);
    // Commit what was typed, if anything (before the inspector is rebuilt)
    void flush();
    // Forget what was typed (the big editor took it over): the saved text comes back
    void discard() { revert(); }
    bool editing() const { return m_dirty; }
    // The tooltip shown when there's no problem
    void setTip(const std::string& tip);
    GtkWidget* widget() const { return m_widget; }

private:
    EditBinding(GtkWidget* widget, Commit commit, bool formula);
    void typed();
    void commit();
    void revert();
    void setError(const std::string& error);

    GtkWidget* m_widget;
    Commit m_commit;
    bool m_formula;
    bool m_dirty = false;    // typed since the last commit
    bool m_setting = false;  // the text is being set by show(), not typed
    std::string m_saved;     // the scene's text
    std::string m_error;     // the saved text's problem
    std::string m_tip;
};

} // namespace linalg::gtk
