#pragma once

#include "scene/Scene.hpp"
#include "scene/Templates.hpp"
#include <raylib.h>
#include <map>
#include <string>
#include <vector>

namespace linalg {

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

private:
    bool m_open = false;
    bool m_justOpened = false;
    Rectangle m_anchor = {0, 0, 0, 0};
    Rectangle m_bounds = {0, 0, 0, 0};
    float m_scroll = 0.0f;
    float m_contentHeight = 0.0f;
};

} // namespace linalg
