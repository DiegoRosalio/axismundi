#pragma once

#include <utility>
#include <vector>

namespace linalg {

// Level-zero contour of a sampled function f(x, y) (marching squares).
// Values are on an (nx+1) × (ny+1) grid, index j*(nx+1) + i, at
// x = xMin + (xMax - xMin)·i/nx and y = yMin + (yMax - yMin)·j/ny.
// Non-finite samples leave their cells empty.
struct ContourSegment {
    double x0, y0, x1, y1;
};

std::vector<ContourSegment> marchingSquares(const std::vector<double>& values, int nx, int ny, double xMin,
                                            double xMax, double yMin, double yMax);

// Join segments that share end points into polylines (for export)
using Polyline = std::vector<std::pair<double, double>>;
std::vector<Polyline> chainSegments(const std::vector<ContourSegment>& segments, double tolerance);

} // namespace linalg
