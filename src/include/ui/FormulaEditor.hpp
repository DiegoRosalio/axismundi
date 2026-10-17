#pragma once

#include "scene/Scene.hpp"
#include "ui/core/TextEditor.hpp"
#include <functional>
#include <string>

namespace linalg {

// Big editor for one formula field: a wide box over the view with the whole
// formula (wrapped), the live result or error, Apply (Enter) and Cancel (Esc).
// Opened from the inspector's expand button or Ctrl+Enter.
class FormulaEditor {
public:
    void open(scene::SpaceKind kind, scene::ObjectId object, const std::string& key, const std::string& text,
              std::function<bool(const std::string&)> commit);
    void close() { m_open = false; }
    bool isOpen() const { return m_open; }
    const std::string& text() const { return m_text; }

    // Draws when open; returns true when the user asked for the formula reference
    bool draw(scene::Scene& scene);

private:
    bool apply();

    bool m_open = false;
    bool m_justOpened = false;
    scene::SpaceKind m_kind = scene::SpaceKind::Euclidean3D;
    scene::ObjectId m_object = scene::kNoObject;
    std::string m_key;
    std::string m_text;
    std::function<bool(const std::string&)> m_commit;
    ui::EditorState m_state;
};

} // namespace linalg
