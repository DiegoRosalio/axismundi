#pragma once

// How the 3D view is shown: grid, axes, labels, the view style. Saved in scene files (view.* keys).

#include "ui/SceneIO.hpp"
#include "view/ViewStyle.hpp"

namespace linalg {

struct ViewSettings {
    bool showGrid = true;
    bool showAxes = true;
    bool showLabels = true;
    float axisLength = 5.0f;
    // Grid (endless, drawn by rendering/GridShader): spacing follows the zoom, or lines every unit
    bool gridAuto = true;
    bool gridFloor = true;     // 3D: the floor (math xy)
    bool gridWallXZ = false;   // 3D: the wall y = 0
    bool gridWallYZ = false;   // 3D: the wall x = 0
    bool gridPolar = false;    // complex plane: circles and rays instead of squares
    bool showNumbers = true;   // numbers along the axes
    view::StyleId style = view::StyleId::Colorful;  // how the 3D view is drawn (saved in the scene)
    bool soloSelection = false;  // show only the selected objects (not saved)
};

void saveViewSettings(const ViewSettings& settings, SceneData& data);
// Keys missing from the file keep their current values (older files have no style, for example)
void loadViewSettings(ViewSettings& settings, const SceneData& data);

} // namespace linalg
