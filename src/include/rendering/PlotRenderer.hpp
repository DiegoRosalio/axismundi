#pragma once

// Turns a plot (function, curve, surface, implicit curve) into GPU meshes once, then draws them every frame.
// SceneView keeps one PlotRenderer per plot object and rebuilds it only when the object or the style changes.

#include "math/Expression.hpp"
#include "math/expr/Expr.hpp"
#include "rendering/MeshBuilder.hpp"
#include <raylib.h>
#include <string>
#include <vector>

namespace linalg {

enum class PlotType {
    Function2D = 0,  // y = f(x)
    Parametric = 1,  // (x(u), y(u), z(u))
    Surface = 2,     // z = f(x, y)
    Implicit = 3,    // f(x, y) = 0
};

constexpr int kPlotTypeCount = 4;

struct PlotSpec {
    PlotType type = PlotType::Function2D;
    std::string f;                 // y=f(x), z=f(x,y) or f(x,y)=0
    std::string px, py, pz;        // parametric components
    float xMin = -5.0f, xMax = 5.0f;
    float yMin = -5.0f, yMax = 5.0f;
    float uMin = 0.0f, uMax = 12.566f;  // curve variable range
    double time = 0.0;             // scene time: the value of t in every expression
    int samplesX = 300;            // samples (or cells) along x, or along t for curves
    int samplesY = 60;             // cells along y (surfaces, implicit curves)
    bool heatmap = true;
    bool smooth = true;            // surfaces: blended colors + shading, or flat-colored cells
    // Black-and-white print preview (matches the LaTeX export): curves in `color` with a dash
    // pattern (0 solid, 1 dashed, 2 dotted, 3 dash-dotted, 4 loosely dashed); surfaces white
    // (or gray by height) with a black mesh
    bool print = false;
    int dash = 0;
    bool grayShading = false;
    Color color = {0, 0, 0, 0};    // without heat map (alpha 0: the default below)
    // Defaults from the view style (alpha 0: theme colors)
    Color curveDefault = {0, 0, 0, 0};
    Color surfaceDefault = {0, 0, 0, 0};
    Color wire = {0, 0, 0, 0};     // surface mesh lines
    float tubeScale = 1.0f;        // curve tube thickness
    float heatSoften = 0.0f;       // 0..1: blend heat-map colors toward heatSoftenTo (pastel styles)
    Color heatSoftenTo = {255, 255, 255, 255};
};

// Builds plot geometry from a PlotSpec and draws it.
//
// Coordinates follow the 3D space convention (z up, see WorldMapping.hpp):
// 2D plots (y = f(x), implicit curves) lie on the floor, the xy-plane.
// Geometry is uploaded to the GPU once per rebuild (surfaces, heat maps and
// curve tubes are meshes), so dense plots stay cheap to draw every frame.
class PlotRenderer {
public:
    PlotRenderer() = default;
    ~PlotRenderer();
    PlotRenderer(const PlotRenderer&) = delete;
    PlotRenderer& operator=(const PlotRenderer&) = delete;

    // Re-evaluate the spec. Returns false and sets error() if it is invalid.
    // `resolver` supplies other names used in the expressions (parameters).
    bool rebuild(const PlotSpec& spec, const expr::Resolver* resolver = nullptr);
    const std::string& error() const { return m_error; }

    // Range of plotted values (y, z or f) used for the heat map
    float minValue() const { return m_minValue; }
    float maxValue() const { return m_maxValue; }

    bool is3D() const { return m_type == PlotType::Surface || m_type == PlotType::Parametric; }

    // `fade` < 1 draws the plot translucent (other objects while something is selected)
    void draw(float fade = 1.0f) const;
    void drawAxes(float length) const;

    // Map an RGB heat ramp (blue -> cyan -> green -> yellow -> red), t in [0, 1]
    static Color heatColor(float t);
    // The heat color blended toward spec.heatSoftenTo by spec.heatSoften (view styles)
    static Color heatColor(const PlotSpec& spec, float t);

    size_t vertexCount() const;  // GPU vertices of the meshes (for the inspector)

private:
    struct Segment {
        Vector3 a, b;
        Color color;
    };
    using MeshData = mesh::MeshData;
    void upload(MeshData& data) { m_meshSet.upload(data); }
    void releaseMeshes() { m_meshSet.release(); }

    const expr::Resolver* m_resolver = nullptr;
    double m_time = 0.0;
    double eval(const Expression& e, double x, double y, double u = 0.0) const {
        return e.eval(ExprVars{x, y, m_time, u}, m_resolver);
    }
    void buildFunction2D(const PlotSpec& spec, const Expression& f);
    void buildParametric(const PlotSpec& spec, const Expression& x, const Expression& y, const Expression& z);
    void buildSurface(const PlotSpec& spec, const Expression& f);
    void buildImplicit(const PlotSpec& spec, const Expression& f);

    PlotType m_type = PlotType::Function2D;
    std::vector<Segment> m_curve;      // thick lines (curves), built into a tube mesh
    std::vector<Segment> m_wireframe;  // thin lines, drawn as lines
    MeshData m_triangles;              // heat map background (plain triangles)
    size_t m_filled = 0;               // triangles collected (surface + heat map)
    mesh::MeshSet m_meshSet;           // GPU meshes: surface chunks, heat map, tubes
    float m_minValue = 0.0f;
    float m_maxValue = 0.0f;
    std::string m_error;
};

} // namespace linalg
