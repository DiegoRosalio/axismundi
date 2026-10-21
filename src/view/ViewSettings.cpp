#include "view/ViewSettings.hpp"

namespace linalg {

void saveViewSettings(const ViewSettings& s, SceneData& data) {
    putBool(data, "view.grid", s.showGrid);
    putBool(data, "view.axes", s.showAxes);
    putBool(data, "view.labels", s.showLabels);
    putFloat(data, "view.axis_length", s.axisLength);
    putBool(data, "view.grid_auto", s.gridAuto);
    putBool(data, "view.grid_floor", s.gridFloor);
    putBool(data, "view.grid_wall_xz", s.gridWallXZ);
    putBool(data, "view.grid_wall_yz", s.gridWallYZ);
    putBool(data, "view.grid_polar", s.gridPolar);
    putBool(data, "view.numbers", s.showNumbers);
    data["view.style"] = view::styleKey(s.style);
}

void loadViewSettings(ViewSettings& s, const SceneData& data) {
    getBool(data, "view.grid", s.showGrid);
    getBool(data, "view.axes", s.showAxes);
    getBool(data, "view.labels", s.showLabels);
    if (getFloat(data, "view.axis_length", s.axisLength)) {
        if (s.axisLength < 1.0f) s.axisLength = 1.0f;
        if (s.axisLength > 20.0f) s.axisLength = 20.0f;
    }
    // view.grid_size (older files) is ignored: the grid is endless now
    getBool(data, "view.grid_auto", s.gridAuto);
    getBool(data, "view.grid_floor", s.gridFloor);
    getBool(data, "view.grid_wall_xz", s.gridWallXZ);
    getBool(data, "view.grid_wall_yz", s.gridWallYZ);
    getBool(data, "view.grid_polar", s.gridPolar);
    getBool(data, "view.numbers", s.showNumbers);
    // Files without a style (older ones) keep the current style
    std::string style;
    if (getString(data, "view.style", style)) s.style = view::styleFromKey(style, s.style);
}

} // namespace linalg
