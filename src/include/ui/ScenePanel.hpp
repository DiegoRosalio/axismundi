#pragma once

#include "scene/Scene.hpp"
#include "ui/core/Layout.hpp"
#include "view/SceneView.hpp"
#include <raylib.h>
#include <functional>
#include <string>
#include <vector>

namespace linalg {

// Requests the side bar raises for the app to handle
struct ScenePanelRequests {
    bool openAdd = false;        // "+ Add" clicked
    Rectangle addAnchor = {0, 0, 0, 0};
    bool openColor = false;      // color swatch clicked for the selected object
    Rectangle colorAnchor = {0, 0, 0, 0};
    bool openTime = false;       // "Time panel" link in a parameter's inspector
    bool openReference = false;  // formula reference button in the inspector header
    bool loadDemo = false;       // "Demo scene" in the empty objects list
    bool toggleSolo = false;     // solo button: show only the selection
    bool soloOff = false;        // "Show all" turned solo off
    // Open the big formula editor for an object field (the expand button or Ctrl+Enter)
    bool openFormula = false;
    scene::ObjectId formulaObject = scene::kNoObject;
    std::string formulaKey;      // field key
    std::string formulaText;     // text to start from (the unsaved edit, if any)
    std::function<bool(const std::string&)> formulaCommit;
    std::string message;         // e.g. a rejected rename
    bool messageIsError = false;
};

// Side bar for a space: the objects list (select, show/hide, undo/redo, add)
// and the inspector of the selected object (name, color, its fields).
// Fields are expressions; each finished edit is one undo step.
class ScenePanel {
public:
    // Keyboard handling for fields (Tab moves to the next field)
    void update(scene::Scene& scene);
    // Draws at (x, y) with the given width (pixels); returns the bottom y
    float draw(float x, float y, float width, scene::Scene& scene, scene::SpaceKind kind, const view::SceneView& view);

    ScenePanelRequests takeRequests();

    bool isEditing() const { return m_activeId >= 0; }
    void finishEditing(scene::Scene& scene);
    void cancelEditing() { m_activeId = -1; }

    void setEasingOpen(bool open) { m_easingOpen = open; }
    void setSolo(bool solo) { m_solo = solo; }  // show-only-the-selection state, for the list

private:
    // One editable text: the object it belongs to and what committing does
    struct Editable {
        int id;
        std::string text;
        std::function<bool(const std::string&)> commit;  // returns false to reject
    };

    float drawObjects(float x, float y, float width, scene::Scene& scene, scene::SpaceKind kind);
    float drawInspector(float x, float y, float width, scene::Scene& scene, scene::SpaceKind kind,
                        const view::SceneView& view);

    // Text field bound to a string; returns its height used (design units handled inside)
    void editText(Rectangle r, int id, const std::string& current, bool invalid,
                  std::function<bool(const std::string&)> commit);
    // Wrapping multi-line field for formulas (with Ctrl+Enter = big editor)
    void editFormula(Rectangle r, int id, const std::string& current, bool invalid,
                     std::function<bool(const std::string&)> commit, scene::ObjectId object, const std::string& key);
    void editField(Rectangle r, int id, const std::string& current, bool invalid,
                   std::function<bool(const std::string&)> commit, scene::ObjectId object, const std::string& key,
                   bool shortField);
    // "label [field ⤢]" row for an object field, with the field's error underneath; returns the bottom y
    float fieldRow(float x, float y, float width, scene::SceneObject& obj, const std::string& key, scene::Scene& scene,
                   float labelWidth = 74.0f);
    static int fieldId(const scene::SceneObject& obj, const std::string& key);

    // The inspector content of each object type (ScenePanelInspectors.cpp)
    struct InspectorContext {
        ui::Column& col;
        scene::SceneObject& o;
        scene::Scene& sc;
        scene::SpaceKind kind;
        const view::SceneView& view;
    };
    void rangeRow(InspectorContext& c, const char* lo, const char* hi, const char* label);
    bool toggleProp(InspectorContext& c, const char* text, const char* key, bool current, const char* tip);
    void inspectVector(InspectorContext& c);
    void inspectMatrix(InspectorContext& c);
    void inspectEigen(InspectorContext& c);
    void inspectPlot(InspectorContext& c);
    void inspectEpicycles(InspectorContext& c);
    void inspectFourierSeries(InspectorContext& c);
    void inspectFrame(InspectorContext& c);
    void inspectIntegral(InspectorContext& c);
    void inspectQuaternion(InspectorContext& c);
    void inspectField(InspectorContext& c);
    void inspectParameter(InspectorContext& c);
    void inspectComplex(InspectorContext& c);
    void inspectProduct(InspectorContext& c);

    scene::SpaceKind m_kind = scene::SpaceKind::Euclidean3D;
    int m_activeId = -1;
    char m_buffer[1024] = {};  // the text being edited (formulas can be long)
    std::vector<Editable> m_editables;      // drawn this frame, in order (for Tab)
    std::vector<Editable> m_lastEditables;  // from the previous frame
    ScenePanelRequests m_requests;
    bool m_showObjects = true;
    bool m_showInspector = true;
    int m_exampleIndex = 0;
    bool m_sliderDirty = false;  // a slider changed the scene; commit on release
    bool m_easingOpen = false;   // easing grid in the parameter inspector
    bool m_solo = false;
};

} // namespace linalg
