#pragma once

// The inspector: edits the selected object (its primary, when several are selected), under the objects list.
// - The common part: the type, the name, the color, Duplicate and Delete.
// - The type's own part comes from an inspector function (gtk/Inspectors*.cpp) that describes its rows with an
//   InspectorBuilder: formula fields, ranges, switches, sliders, choices, live values, buttons.
// - refresh() is cheap: it rebuilds the rows only when the selection or the object's layout changed (another
//   mode, another matrix size), and otherwise only updates the values shown.
// Every row finds its object again by id when it's used, so undo (which replaces the objects) is safe.

#include "scene/SceneObject.hpp"
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>
#include <sigc++/connection.h>
#include <functional>
#include <map>
#include <string>
#include <vector>

typedef struct _GtkWidget GtkWidget;

namespace linalg {
class Session;
}

namespace linalg::gtk {

class EditBinding;
class InspectorBuilder;

class Inspector : public Gtk::Box {
public:
    explicit Inspector(Session& session);

    void refresh();
    // The user changed the scene here (after the undo step was recorded), or a slider moved (live, no undo step yet)
    void onChanged(std::function<void()> callback) { m_changed = std::move(callback); }
    void onToast(std::function<void(const std::string&)> callback) { m_toast = std::move(callback); }
    // Commit typing in progress and slider moves waiting for their undo step
    void flush();

    Session& session() { return m_session; }
    // Show the last rows (the tour's screenshots of long inspectors)
    void scrollToEnd();
    // A row the inspector function named with InspectorBuilder::anchor (lessons point at it); nullptr: not shown
    GtkWidget* anchor(const std::string& id) const;
    // Scroll so that `child` (a row inside the inspector) is in sight; false while it has no place yet
    bool scrollTo(GtkWidget* child);

private:
    friend class InspectorBuilder;
    using Update = std::function<void(const scene::SceneObject&)>;

    std::string layoutKey() const;
    void rebuild();
    void update();
    void buildHeader(scene::SceneObject& o);
    void buildFooter();
    scene::SceneObject* current();  // the inspected object, found again by id (nullptr when it's gone)
    // Run `change` on the inspected object, then record one undo step and tell the window
    void edit(const std::function<void(scene::SceneObject&)>& change);
    // A slider moved: the scene changed already; the undo step is recorded when it rests
    void sliderMoved();

    Session& m_session;
    Gtk::ScrolledWindow m_scroll;
    Gtk::Box m_content;
    Gtk::Label m_placeholder;
    scene::ObjectId m_id = scene::kNoObject;
    scene::SpaceKind m_kind = scene::SpaceKind::Euclidean3D;
    std::string m_key;                  // layoutKey() of the rows shown
    std::vector<EditBinding*> m_edits;  // owned by their widgets
    std::vector<Update> m_updates;
    std::map<std::string, GtkWidget*> m_anchors;  // the rows named for lessons (rebuilt with the rows)
    bool m_updating = false;            // values are being set by update(), not by the user
    bool m_rebuildQueued = false;
    sigc::connection m_sliderRest;
    int m_exampleIndex = 0;             // "Next example" buttons
    std::function<void()> m_changed;
    std::function<void(const std::string&)> m_toast;
};

// What an inspector function uses to describe the rows of its object type. Rows go into the current group (a
// titled box of rows, AdwPreferencesGroup); choices and button grids stand between groups. Callbacks get the
// object found again by id; edits record one undo step each.
class InspectorBuilder {
public:
    using Object = scene::SceneObject;
    using Text = std::function<std::string(const Object&)>;

    InspectorBuilder(Inspector& inspector, Object& object);
    Object& object() { return m_object; }
    Session& session() { return m_inspector.m_session; }

    // A new group of rows; `description` is the hint under its title
    void group(const std::string& title = "", const std::string& description = "");
    // A formula field (its label unless one is given), with its problem underneath
    void formula(const std::string& key, const std::string& label = "");
    // Short formula fields side by side: vector components, matrix rows, ranges. `title` on the left ("" = none),
    // `between` between the fields ("to" for ranges), `captions` above them (x, y, z)
    void cells(const std::string& title, const std::vector<std::string>& keys, const std::string& between = "",
               const std::vector<std::string>& tips = {}, const std::vector<std::string>& captions = {});
    void range(const std::string& lo, const std::string& hi, const std::string& label) { cells(label, {lo, hi}, "to"); }
    // Number fields bound to getters/setters (a parameter's range); `set` returns false to reject
    struct Number {
        std::function<double(const Object&)> get;
        std::function<bool(Object&, double)> set;
    };
    void numbers(const std::string& title, const std::vector<Number>& fields, const std::string& between = "",
                 const std::string& tip = "");
    // A switch bound to a true/false property
    void toggle(const std::string& label, const std::string& prop, bool fallback, const std::string& tip = "");
    // Segmented choice (e.g. Components / Formula), between groups; `get` reads it, `pick` changes the object
    void choice(const std::vector<std::string>& labels, std::function<int(const Object&)> get,
                const std::vector<std::string>& tips, std::function<void(Object&, int)> pick);
    // A drop-down list row
    void combo(const std::string& label, const std::vector<std::string>& items, std::function<int(const Object&)> get,
               std::function<void(Object&, int)> set, const std::string& tip = "");
    // A slider row: live while dragging, one undo step when it rests. `limits` may follow the object (a range)
    struct Limits {
        double min, max, step;
    };
    void slider(const std::string& label, std::function<Limits(const Object&)> limits,
                std::function<double(const Object&)> get, std::function<void(Object&, double)> set,
                std::function<std::string(double)> format, const std::string& tip = "");
    // A live value row (title, value); hidden while `text` is ""
    void value(const std::string& title, Text text, const std::string& tip = "");
    // A live line of text: Dim (hints), Warning or Error; hidden while `text` is ""
    enum class Tone { Dim, Warning, Error, Mono };
    void line(Text text, Tone tone = Tone::Dim);
    // A button row; `run` changes the object (one undo step)
    void button(const std::string& label, const std::string& icon, std::function<void(Object&)> run,
                const std::string& tip = "");
    // "Next example": `apply(object, index)` with the index going round `count` examples
    void exampleButton(int count, std::function<void(Object&, int)> apply);
    // Buttons in a grid of `columns`, between groups (matrix presets)
    struct GridButton {
        std::string label, tip;
        std::function<void(Object&)> run;
    };
    void buttonGrid(const std::vector<GridButton>& buttons, int columns);
    // A widget built by hand (a chart), as a row
    void widget(GtkWidget* child);
    // Run `update` on every refresh (for rows built by hand)
    void live(std::function<void(const Object&)> update);
    // Name the row added last ("matrix.grid"), so a lesson step can point at it; `button` names the button with
    // that index of the last button grid instead
    void anchor(const std::string& id, int button = -1);
    void anchorGroup(const std::string& id);  // the current group of rows

private:
    GtkWidget* currentGroup();
    void add(GtkWidget* row);
    EditBinding* bindField(GtkWidget* editable, const std::string& key);

    Inspector& m_inspector;
    Object& m_object;
    GtkWidget* m_group = nullptr;
    GtkWidget* m_last = nullptr;                // the last row, for anchor()
    std::vector<GtkWidget*> m_lastButtons;      // the last button grid's buttons
};

// The type's own part of the inspector: the function that describes its rows (gtk/Inspectors*.cpp), and the
// properties whose value changes which rows there are (a mode, a matrix size, a switch that shows more rows)
struct InspectorType {
    void (*inspect)(InspectorBuilder&) = nullptr;
    std::vector<std::string> layoutProps;
};
const InspectorType* inspectorFor(const std::string& typeId);  // nullptr: the inspector lists the type's fields

// Fallback for types without their own inspector yet: every active field as a formula row
void inspectFields(InspectorBuilder& b);

} // namespace linalg::gtk
