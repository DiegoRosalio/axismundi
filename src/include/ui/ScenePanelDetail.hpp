#pragma once

#include "scene/SceneObject.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Ui.hpp"
#include <raylib.h>
#include <string>
#include <vector>

// Helpers shared by ScenePanel.cpp and ScenePanelInspectors.cpp (not part of the panel's interface)
namespace linalg::panel {

constexpr float kPi = 3.14159265f;

std::string fmt(double v);  // a short number: "0.5", "1e-05"
std::string summaryOf(const scene::SceneObject& o);  // the objects list's one-line summary
const char* typeLabel(const scene::SceneObject& o);
// A wrapped note under the controls
void note(ui::Column& col, const std::string& text, Color color, ui::FontRole role = ui::FontRole::Small);

// Sample functions per plot type, for the "Next example" button
struct PlotExample {
    const char* f;
    const char* x;
    const char* y;
    const char* z;
};
const std::vector<PlotExample>& plotExamples(const std::string& type);

} // namespace linalg::panel
