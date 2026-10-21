#pragma once

// The sidebar's list of the current space's objects: a color dot, the name, a one-line summary
// (scene/Summaries) and a visibility switch per row. Selecting rows (click, Ctrl+click, Shift+click) selects the
// objects; refresh() brings the list up to date with the scene (new objects, names, values, the selection).

#include "scene/SceneObject.hpp"
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/togglebutton.h>
#include <functional>
#include <string>
#include <vector>

namespace linalg {
class Session;
}

namespace linalg::gtk {

class ObjectsList : public Gtk::Box {
public:
    explicit ObjectsList(Session& session);

    // Rebuild the rows when objects were added, removed, renamed or recolored; update the summaries and the
    // selection otherwise. Cheap when nothing changed: call it often.
    void refresh();
    // Called when the user changed the scene from the list (selection, visibility)
    void onChanged(std::function<void()> callback) { m_changed = std::move(callback); }

private:
    struct Row {
        scene::ObjectId id = scene::kNoObject;
        Gtk::ListBoxRow* row = nullptr;
        Gtk::Label* summary = nullptr;
        Gtk::ToggleButton* eye = nullptr;
    };
    void rebuild();
    void syncSelection();
    void onSelectionChanged();
    void scrollTo(scene::ObjectId id);
    std::string structureKey() const;

    Session& m_session;
    Gtk::ScrolledWindow m_scroll;
    Gtk::ListBox m_list;
    Gtk::Label m_empty;
    std::vector<Row> m_rows;
    std::string m_key;   // structureKey() of the rows shown
    bool m_syncing = false;  // selection changes made by syncSelection, not by the user
    scene::ObjectId m_primary = scene::kNoObject;  // the selection's primary, as last scrolled to
    std::function<void()> m_changed;
};

} // namespace linalg::gtk
