#pragma once

#include "math/VectorField.hpp"
#include "math/expr/Expr.hpp"
#include "rendering/MeshBuilder.hpp"
#include <raylib.h>
#include <string>
#include <vector>

namespace linalg {

// What a vector field object asks to draw (filled by SceneView from the object and the view style)
struct FieldSpec {
    std::string formula;          // F(x, y, z), compiled with x, y, z, t
    bool plane = true;            // on the floor z = 0, or in a box
    field::Box box;               // math coordinates
    int nx = 8, ny = 8, nz = 1;   // arrows per axis
    bool sameLength = false;
    bool heatmap = true;          // color arrows by |F|
    Color color = {255, 255, 255, 255};       // arrows without heat map
    Color lineColor = {255, 255, 255, 255};   // flow lines
    Color particleColor = {255, 255, 255, 255};
    bool lines = false;
    int seeds = 6;                // flow lines per axis
    bool particles = false;
    double speed = 1.0;
    double time = 0.0;            // t in the formula (the scene time)
    float lineScale = 1.0f;       // style thickness
    float heatSoften = 0.0f;      // style: heat colors blended toward heatSoftenTo
    Color heatSoftenTo = {255, 255, 255, 255};
};

// Arrows (one mesh), flow lines (tubes) and particles riding the lines. Geometry is built once
// per change; particles are placed every frame from the scene time.
class FieldRenderer {
public:
    // Returns false (and error() says why) when the formula does not compile
    bool rebuild(const FieldSpec& spec, const expr::Resolver* resolver);
    void draw(double time, float fade = 1.0f) const;

    const std::string& error() const { return m_error; }
    size_t arrowCount() const { return m_arrowCount; }
    double minMagnitude() const { return m_minMagnitude; }
    double maxMagnitude() const { return m_maxMagnitude; }
    const std::vector<field::FlowLine>& lines() const { return m_lines; }

    // Flow line seeds per axis actually used (volume fields use fewer, for speed)
    static int seedsPerAxis(const FieldSpec& spec);

private:
    mesh::MeshSet m_meshes;
    std::vector<field::FlowLine> m_lines;  // math coordinates (for particles)
    FieldSpec m_spec;
    std::string m_error;
    size_t m_arrowCount = 0;
    double m_minMagnitude = 0.0, m_maxMagnitude = 0.0;
};

} // namespace linalg
