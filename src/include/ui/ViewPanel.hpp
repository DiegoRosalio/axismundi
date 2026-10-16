#pragma once

#include "ui/AppMode.hpp"
#include "ui/SceneIO.hpp"
#include <raylib.h>

namespace linalg {

// Vectors whose color can be customized
enum VectorSlot { kVecA, kVecB, kVecSum, kVecCross, kVecMA, kVecMB, kVectorSlotCount };
const char* vectorSlotName(int slot);  // "A", "B", "A+B", ...

struct ViewSettings {
    bool showGrid = true;
    bool showAxes = true;
    bool showLabels = true;
    bool showEigenvectors = true;
    bool showTransformedVectors = true;
    float axisLength = 5.0f;

    // Custom vector colors (unset slots follow the theme)
    bool customColor[kVectorSlotCount] = {};
    Color vectorColor[kVectorSlotCount] = {};

    Color colorFor(int slot) const;  // custom color or the theme default
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
    // A swatch was clicked: returns its slot (or -1) and the swatch rectangle
    int takeColorRequest(Rectangle& anchor);

    void saveState(SceneData& data) const;
    void loadState(const SceneData& data);
    // Vector colors live in the preferences, not in scenes
    void saveColors(SceneData& prefs) const;
    void loadColors(const SceneData& prefs);

private:
    static bool take(bool& flag) { bool v = flag; flag = false; return v; }

    ViewSettings m_settings;
    bool m_resetCamera = false;
    bool m_save = false;
    bool m_load = false;
    bool m_expanded = true;
    int m_colorRequest = -1;
    Rectangle m_colorAnchor = {0, 0, 0, 0};
};

} // namespace linalg
