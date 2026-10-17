#pragma once

#include "ui/AppMode.hpp"
#include "ui/SceneIO.hpp"
#include <raylib.h>

namespace linalg {

struct ViewSettings {
    bool showGrid = true;
    bool showAxes = true;
    bool showLabels = true;
    float axisLength = 5.0f;
    float gridSize = 10.0f;  // the grid reaches from -gridSize to +gridSize
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
    bool takeSaveRequest() { return take(m_save); }
    bool takeLoadRequest() { return take(m_load); }

    void saveState(SceneData& data) const;
    void loadState(const SceneData& data);

private:
    static bool take(bool& flag) { bool v = flag; flag = false; return v; }

    ViewSettings m_settings;
    bool m_resetCamera = false;
    bool m_save = false;
    bool m_load = false;
    bool m_expanded = true;
};

} // namespace linalg
