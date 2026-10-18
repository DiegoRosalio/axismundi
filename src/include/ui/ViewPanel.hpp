#pragma once

// ViewSettings (grid, axes, labels, style, saved in scene files) and the "View" section of the side bar
// that edits them, with the camera buttons and the scene file buttons.

#include "ui/AppMode.hpp"
#include "ui/SceneIO.hpp"
#include "view/ViewStyle.hpp"
#include <raylib.h>
#include <string>

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

// "View" group in the side bar: grid/axes/labels toggles, axis length,
// camera reset and scene save/load buttons.
class ViewPanel {
public:
    // Draws at (x, y) with the given width; returns the bottom y.
    float draw(float x, float y, float width, AppMode mode);

    ViewSettings& settings() { return m_settings; }
    const ViewSettings& settings() const { return m_settings; }

    // One-shot requests raised by buttons (cleared when read)
    bool takeResetCameraRequest() { return take(m_resetCamera); }
    // 0 none, 1 front, 2 right, 3 top, 4 focus on the selection
    int takeCameraViewRequest() { int v = m_cameraView; m_cameraView = 0; return v; }
    bool takeSaveRequest() { return take(m_save); }
    bool takeLoadRequest() { return take(m_load); }
    bool takeSaveAsRequest() { return take(m_saveAs); }
    bool takeNewRequest() { return take(m_new); }
    // Shown under the file buttons ("" = not saved yet)
    void setCurrentFile(const std::string& path) { m_currentFile = path; }

    void saveState(SceneData& data) const;
    void loadState(const SceneData& data);

private:
    static bool styleCard(Rectangle r, view::StyleId id, bool selected);
    static bool take(bool& flag) { bool v = flag; flag = false; return v; }

    ViewSettings m_settings;
    bool m_resetCamera = false;
    int m_cameraView = 0;
    bool m_save = false;
    bool m_load = false;
    bool m_saveAs = false;
    bool m_new = false;
    std::string m_currentFile;
    bool m_expanded = true;
};

} // namespace linalg
