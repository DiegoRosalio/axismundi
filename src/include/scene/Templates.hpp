#pragma once

// Ready-made objects for the Add menu: an object type with some fields and properties preset, and the
// "from selection" templates that build derived objects from the selected ones (a + b, cross(a, b), M*v...).
// Scene logic only (no UI), shared by both front ends and tested in tests/test_scene.cpp.

#include "scene/Scene.hpp"
#include <map>
#include <string>
#include <utility>
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

// Templates for the current selection of a space: (menu label, choice)
std::vector<std::pair<std::string, AddChoice>> addTemplates(scene::Scene& scene, scene::SpaceKind kind);

// Create the object described by a choice, select it and record an undo step
scene::SceneObject* applyAddChoice(scene::Scene& scene, scene::SpaceKind kind, const AddChoice& choice);

} // namespace linalg
