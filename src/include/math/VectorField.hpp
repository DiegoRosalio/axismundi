#pragma once

#include "math/Vector3D.hpp"
#include "math/expr/Expr.hpp"
#include <vector>

namespace linalg::field {

// Vector fields F(x, y, z): arrows on a grid, flow lines and particles riding them (no raylib).

struct Box {
    Vector3D min, max;
    bool contains(const Vector3D& p, double slack = 1e-9) const;
};

// F evaluated from a compiled formula (x, y, z slots; t = the scene time). In a plane field only
// F's x and y components are used, at z = 0.
class FieldFunction {
public:
    FieldFunction(const expr::Program& program, const expr::Resolver* resolver, double time, bool plane);
    // False where F is not a finite vector (errors, division by zero...)
    bool at(const Vector3D& p, Vector3D& out) const;
    bool plane() const { return m_plane; }

private:
    const expr::Program& m_program;
    const expr::Resolver* m_resolver;
    double m_time;
    bool m_plane;
};

struct Arrow {
    Vector3D base;  // grid point
    Vector3D vec;   // scaled for drawing: the longest arrow fits one cell
    double magnitude = 0.0;  // |F| at the grid point
};

struct ArrowSet {
    std::vector<Arrow> arrows;
    double minMagnitude = 0.0, maxMagnitude = 0.0;
    double cell = 1.0;  // grid spacing (smallest of the axes)
};

// Arrows at the centers of an nx × ny (× nz) grid over the box (plane: nz is ignored, z = 0).
// With sameLength every arrow has the same length (a direction field); the color still shows |F|.
ArrowSet sampleArrows(const FieldFunction& f, const Box& box, int nx, int ny, int nz, bool sameLength);

struct FlowLine {
    std::vector<Vector3D> points;
    std::vector<double> times;  // flow time at each point, from 0 (the upstream end)
    bool closed = false;        // came back to its start (an orbit)
    double duration() const { return times.empty() ? 0.0 : times.back(); }
};

// A flow line through `seed`, traced both ways with RK4 in steps of length `step` along the
// direction of F. It stops at the box edge, where F vanishes, after maxSteps per direction, or when
// it closes on itself. Flow time advances by step / |F|, so particles move at the field's speed.
FlowLine traceFlowLine(const FieldFunction& f, const Vector3D& seed, const Box& box, double step, int maxSteps);

// Flow lines from several seeds, kept apart: a line is cut where it comes closer than `separation`
// to a line already kept (its own neighbourhood around the seed excepted), and a seed that is
// already that close is skipped. Spirals into a sink then stay readable.
std::vector<FlowLine> traceFlowLines(const FieldFunction& f, const std::vector<Vector3D>& seeds, const Box& box,
                                     double step, int maxSteps, double separation);

// The flow lines a field object shows: seeds per axis (plane 2..16, volume 2..4), step, length and
// spacing chosen from the box, so the app and the LaTeX export draw the same lines
int seedsPerAxis(bool plane, int requested);
std::vector<FlowLine> fieldFlowLines(const FieldFunction& f, const Box& box, int seedsRequested);

// Evenly spaced seeds: perAxis × perAxis on the floor (plane) or perAxis³ in the box
std::vector<Vector3D> flowSeeds(const Box& box, bool plane, int perAxis);

// The point at flow time tau along a line (clamped to its ends)
Vector3D particleAt(const FlowLine& line, double tau);

// Flow time of particle k of n on a line at scene time t: they move at `speed` and start over
// at the upstream end (closed lines just go round). A pure function of t, so scrubbing works.
double particleTime(const FlowLine& line, int k, int n, double t, double speed);

} // namespace linalg::field
