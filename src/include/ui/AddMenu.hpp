#pragma once

#include "scene/Scene.hpp"
#include <raylib.h>
#include <map>
#include <string>
#include <vector>

namespace linalg {

// What to create: an object type with some fields and properties preset
struct AddChoice {
    std::string typeId;
    std::string name;                           // "" = the type's default name
    std::map<std::string, std::string> fields;  // field key -> expression
    std::map<std::string, std::string> props;
    // Optional static slider created first (0 to 1); "$s" in fields becomes its name
    std::string helperParameter;
};

// Popover listing what can be added to a space. "From selection" templates
// build derived objects from the selected ones (a + b, cross(a, b), M*v, ...).
class AddMenu {
public:
    void open(Rectangle anchor) { m_open = true; m_anchor = anchor; m_justOpened = true; m_scroll = 0; }
    void close() { m_open = false; }
    bool isOpen() const { return m_open; }
    bool contains(Vector2 p) const { return m_open && CheckCollisionPointRec(p, m_bounds); }

    // Returns true and fills `choice` when an entry was clicked
    bool draw(scene::Scene& scene, scene::SpaceKind kind, AddChoice& choice);

    // Templates for the current selection (also used by tests and the tutorial)
    static std::vector<std::pair<std::string, AddChoice>> templates(scene::Scene& scene, scene::SpaceKind kind);

private:
    bool m_open = false;
    bool m_justOpened = false;
    Rectangle m_anchor = {0, 0, 0, 0};
    Rectangle m_bounds = {0, 0, 0, 0};
    float m_scroll = 0.0f;
    float m_contentHeight = 0.0f;
};

// Create the object described by a choice, select it and record an undo step
scene::SceneObject* applyAddChoice(scene::Scene& scene, scene::SpaceKind kind, const AddChoice& choice);

} // namespace linalg
