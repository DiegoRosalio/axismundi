#pragma once

// The type-specific inspector functions, one per object type, by family file (gtk/InspectorsBasic.cpp,
// InspectorsCalculus.cpp, InspectorsMore.cpp). The table in Inspector.cpp (`inspectorFor`) maps type ids to them.
// Each mirrors its raylib version in ui/ScenePanelInspectors.cpp, written with an InspectorBuilder.

#include "gtk/Inspector.hpp"
#include <cmath>
#include <cstdio>
#include <string>

namespace linalg::gtk::inspectors {

using Object = scene::SceneObject;
constexpr double kPi = 3.14159265358979;

// printf into a std::string
template <class... Args>
std::string format(const char* pattern, Args... args) {
    char buf[256];
    std::snprintf(buf, sizeof(buf), pattern, args...);
    return buf;
}

// "0.5", "1e-05", never "-0"
inline std::string shortNumber(double v) { return format("%.4g", std::fabs(v) < 5e-5 ? 0.0 : v); }

// InspectorsBasic.cpp
void vector(InspectorBuilder& b);
void matrix(InspectorBuilder& b);
void eigen(InspectorBuilder& b);
void plot(InspectorBuilder& b);  // function, surface, implicit, curve
void parameter(InspectorBuilder& b);
void complexNumber(InspectorBuilder& b);
void product(InspectorBuilder& b);

// InspectorsCalculus.cpp
void doubleIntegral(InspectorBuilder& b);
void lineIntegral(InspectorBuilder& b);
void frame(InspectorBuilder& b);
void tangent(InspectorBuilder& b);
void tangentPlane(InspectorBuilder& b);
void velocity(InspectorBuilder& b);
void taylor(InspectorBuilder& b);
void newton(InspectorBuilder& b);

// InspectorsMore.cpp
void quaternion(InspectorBuilder& b);
void field(InspectorBuilder& b);
void epicycles(InspectorBuilder& b);
void fourierSeries(InspectorBuilder& b);

} // namespace linalg::gtk::inspectors
