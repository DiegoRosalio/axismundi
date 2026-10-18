#pragma once

#include "export/LatexExport.hpp"
#include "math/Matrix4D.hpp"
#include "rendering/FieldRenderer.hpp"
#include "rendering/PlotRenderer.hpp"
#include "scene/Scene.hpp"
#include "ui/ViewPanel.hpp"
#include <map>
#include <vector>

namespace linalg::view {

struct DrawOptions {
    const ViewSettings* view = nullptr;
    const scene::Selection* selection = nullptr;
    // Show this matrix object with a different value (animation, breakdown)
    scene::ObjectId overrideMatrix = scene::kNoObject;
    Matrix4D overrideValue = Matrix4D::identity();
    // Faded bases for completed breakdown steps
    std::vector<Matrix4D> ghosts;
    // Black-and-white print preview: the LaTeX export's styles (gray levels, dash patterns) on
    // white; objects outside the export scope are drawn faint
    bool print = false;
    latex::Scope printScope = latex::Scope::Visible;
    bool printGrayShading = false;
    // Fade the objects that are not selected (while something is selected)
    bool dimUnselected = false;
    // Show only the selected objects (when the selection is not empty)
    bool solo = false;
};

// Draws the objects of a space. 3D space: math coordinates with z up (see
// WorldMapping.hpp). Complex plane: real part along world X, imaginary along Y.
class SceneView {
public:
    // Surfaces: smooth (blended colors, shading) or faceted (flat cells)
    void setSmoothSurfaces(bool smooth) { m_smoothSurfaces = smooth; }

    // Call inside BeginMode3D
    void draw3D(scene::Space& space, const DrawOptions& options);
    // Call after EndMode3D
    void drawLabels(scene::Space& space, const DrawOptions& options, const Camera3D& camera);

    // Theme palette color for an object (or its custom color)
    static Color colorOf(const scene::SceneObject& object);
    // Whether an object is drawn: visible, and selected when solo is on
    static bool shown(const DrawOptions& options, const scene::SceneObject& object);
    // Value a matrix object is drawn with (taking the override into account)
    static Matrix4D displayedMatrix(const scene::SceneObject& object, const DrawOptions& options);

    // World position of an object's handle (arrow tip, point), for labels and picking
    static bool anchorPoint(const scene::SceneObject& object, scene::SpaceKind kind, const DrawOptions& options,
                            Vector3& out);

    // Plot status (error text and value range) for the inspector
    const PlotRenderer* plot(scene::ObjectId id) const;
    // Vector field status (arrow count, |F| range) for the inspector
    const FieldRenderer* fieldRenderer(scene::ObjectId id) const;

private:
    void drawEuclidean(scene::Space& space, const DrawOptions& options);
    void drawComplex(scene::Space& space, const DrawOptions& options);
    void drawEuclideanPrint(scene::Space& space, const DrawOptions& options);
    void drawComplexPrint(scene::Space& space, const DrawOptions& options);
    void drawLabelsPrint(scene::Space& space, const DrawOptions& options, const Camera3D& camera);
    void drawQuaternion(const scene::QuaternionObject& o, bool selected, Color color, bool fade, bool print,
                        int printStyle);
    void drawEpicycles(const scene::EpicyclesObject& o, Color color, bool fade, bool print, Color ink, bool selected);
    void drawFourierSeries(const scene::FourierSeriesObject& o, Color color, bool fade, bool print, Color ink);
    void drawFrame(const scene::FrameObject& o, Color color, bool fade, bool print, Color ink, bool selected);
    void drawDoubleIntegral(const scene::DoubleIntegralObject& o, Color color, bool fade, bool print, Color ink);
    void drawLineIntegral(const scene::LineIntegralObject& o, Color color, bool fade, bool print, Color ink,
                          bool selected);
    void updatePlots(scene::Space& space);
    void updateFields(scene::Space& space);

    struct PlotCache {
        PlotRenderer renderer;
        std::uint64_t builtAt = 0;
        bool dark = true;
        unsigned color = 0;
        bool smooth = true;
        int printKey = -2;  // print style the geometry was built with (-2: normal)
    };
    std::map<scene::ObjectId, PlotCache> m_plots;
    struct FieldCache {
        FieldRenderer renderer;
        std::uint64_t builtAt = 0;
        int styleKey = -1;   // style + theme + print style the geometry was built with
        unsigned color = 0;
    };
    std::map<scene::ObjectId, FieldCache> m_fields;
    // Print preview: style index of each exported object (-1: not exported)
    std::map<scene::ObjectId, int> m_printStyle;
    bool m_print = false;
    bool m_printGray = false;
    int printStyleOf(scene::ObjectId id) const;
    bool m_smoothSurfaces = true;
    bool m_dim = false;  // this frame: fade unselected objects
    bool dimmed(const DrawOptions& options, const scene::SceneObject& object) const;
};

} // namespace linalg::view
