#pragma once

#include "math/Expression.hpp"
#include "math/expr/Expr.hpp"
#include <raylib.h>
#include <string>
#include <vector>

namespace linalg {

enum class PlotType {
    Function2D = 0,  // y = f(x)
    Parametric = 1,  // (x(t), y(t), z(t))
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
    float tMin = 0.0f, tMax = 12.566f;
    int resolution = 60;
    bool heatmap = true;
    Color color = {0, 0, 0, 0};    // without heat map (alpha 0: theme color)
};

// Builds plot geometry from a PlotSpec and draws it.
//
// Coordinates follow the 3D space convention (z up, see WorldMapping.hpp):
// 2D plots (y = f(x), implicit curves) lie on the floor, the xy-plane.
class PlotRenderer {
public:
    // Re-evaluate the spec. Returns false and sets error() if it is invalid.
    // `resolver` supplies other names used in the expressions (parameters).
    bool rebuild(const PlotSpec& spec, const expr::Resolver* resolver = nullptr);
    const std::string& error() const { return m_error; }

    // Range of plotted values (y, z or f) used for the heat map
    float minValue() const { return m_minValue; }
    float maxValue() const { return m_maxValue; }

    bool is3D() const { return m_type == PlotType::Surface || m_type == PlotType::Parametric; }

    void draw() const;
    void drawAxes(float length) const;
    void drawLabels(const Camera3D& camera, float axisLength, bool showAxes) const;

    // Map an RGB heat ramp (blue -> cyan -> green -> yellow -> red), t in [0, 1]
    static Color heatColor(float t);

private:
    struct Segment {
        Vector3 a, b;
        Color color;
    };
    struct Triangle {
        Vector3 a, b, c;
        Color color;
    };

    const expr::Resolver* m_resolver = nullptr;
    double eval(const Expression& e, double x, double y, double t) const {
        return e.eval(ExprVars{x, y, t}, m_resolver);
    }
    void buildFunction2D(const PlotSpec& spec, const Expression& f);
    void buildParametric(const PlotSpec& spec, const Expression& x, const Expression& y, const Expression& z);
    void buildSurface(const PlotSpec& spec, const Expression& f);
    void buildImplicit(const PlotSpec& spec, const Expression& f);

    PlotType m_type = PlotType::Function2D;
    std::vector<Segment> m_curve;      // thick lines (curves)
    std::vector<Segment> m_wireframe;  // thin lines
    std::vector<Triangle> m_triangles;
    float m_minValue = 0.0f;
    float m_maxValue = 0.0f;
    std::string m_error;
};

} // namespace linalg
