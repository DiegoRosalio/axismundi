#pragma once

#include "math/Vector3D.hpp"
#include "scene/Scene.hpp"
#include <string>
#include <vector>

namespace linalg::latex {

// Export a space as a black-and-white TikZ/PGFPlots figure for a LaTeX
// document: thin lines, line styles instead of colors, math labels, no font
// commands (the document's fonts are used). Graphics-free and unit-tested.

enum class Scope { Visible, Selection };
enum class View { Auto, Flat, Space };  // Flat = 2D axis, Space = 3D axis

struct Options {
    Scope scope = Scope::Visible;
    View view = View::Auto;
    double azimuth = 25.0;     // 3D view (degrees), see viewFromDirection
    double elevation = 30.0;
    double width = 0.8;        // fraction of \linewidth
    bool axes = true;
    bool legend = true;        // legend for plots (vectors and points are labelled at their tips)
    bool grid = false;
    bool grayShading = false;  // surfaces shaded by height (otherwise white with a black mesh)
    bool standalone = false;   // a complete document (for compiling on its own)
    int curveSamples = 200;    // caps, so documents compile quickly
    int surfaceSamples = 30;
    int implicitSamples = 80;
    int fieldArrows = 20;      // vector field arrows per axis (volume: a third)
};

struct Result {
    std::string code;
    int exported = 0;
    std::vector<std::string> skipped;  // "theta (parameter)"
    bool flat = false;                 // a 2D axis was used
};

Result exportLatex(const scene::Space& space, const scene::Selection& selection, const Options& options);

// Line style and gray of the n-th exported object; shared with the in-app print preview
struct Style {
    const char* dash;  // TikZ name: solid, dashed, densely dotted, dashdotted, loosely dashed
    const char* gray;  // TikZ color: black, black!65, black!45
    int dashKind;      // 0 solid, 1 dashed, 2 dotted, 3 dash-dotted, 4 loosely dashed
    float grayLevel;   // 0 = black ... 1 = white
};
Style styleFor(int index);

// Objects that the export draws (and so get a style), in space order
bool drawable(const scene::SceneObject& object);
std::vector<const scene::SceneObject*> exportedObjects(const scene::Space& space, const scene::Selection& selection,
                                                       Scope scope);
// Style index of an object (or -1 when it is not exported)
int styleIndexOf(const scene::Space& space, const scene::Selection& selection, Scope scope, scene::ObjectId id);

// PGFPlots view angles for a viewer looking from `direction` (math coordinates, z up)
void viewFromDirection(const Vector3D& direction, double& azimuth, double& elevation);

// Object name as math: theta -> \theta, z1 -> z_{1}, a_b -> a_{b}
std::string nameToLatex(const std::string& name);

} // namespace linalg::latex
