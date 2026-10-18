#pragma once

#include "view/SceneView.hpp"
#include <map>
#include <raylib.h>
#include <string>
#include <utility>
#include <vector>

// How each object type is drawn, labeled and picked in the view: one ObjectView per type id, kept in a
// table (objectView). SceneView and App loop over the objects and call these functions, so adding a type
// means adding one entry in view/objects/<Family>.cpp instead of a branch in several if-chains.
namespace linalg::view {

// Everything a type's draw function needs for one object, prepared by SceneView
struct DrawContext {
    const SceneView& view;          // the plot and field caches (plot(id), fieldRenderer(id))
    const DrawOptions& options;     // view settings, selection, Paper scope...
    bool selected = false;
    bool dim = false;               // faded toward the background (something else is selected)
    bool print = false;             // Paper: the LaTeX export's grays and dash patterns
    int printStyle = -1;            // Paper: index in the export's style table (-1: not exported)
    Color color{0, 0, 0, 255};      // normal: the palette color (already faded when dim); Paper: its gray
};

// A label to place next to a point (offset in design units from the point)
struct LabelSpot {
    Vector3 at;
    std::string text;
    Vector2 offset;
};

// Which theme palette colors objects of a type (SceneView::colorOf)
enum class Palette { Accent, Vectors, Plots, Complexes, Eigen, Product };

using Segment = std::pair<Vector3, Vector3>;  // two world points

struct ObjectView {
    Palette palette = Palette::Accent;
    // Draw the object (inside BeginMode3D); ctx.print selects the Paper look
    void (*draw)(const scene::SceneObject& o, const DrawContext& ctx) = nullptr;
    // World point of its handle (arrow tip, moving point): its label sits there and picking aims at it
    bool (*anchor)(const scene::SceneObject& o, const DrawOptions& options, Vector3& out) = nullptr;
    // Text of its label in the normal view (nullptr: the name). Paper labels show only the name.
    std::string (*label)(const scene::SceneObject& o) = nullptr;
    // Several labels instead of one at the anchor (eigenvectors: one per λ)
    void (*labels)(const scene::SceneObject& o, bool print, std::vector<LabelSpot>& out) = nullptr;
    bool plainLabel = false;  // label in the text color instead of the object's (matrices)
    // Screen-picking: world segments near which a click selects the object
    void (*pick)(const scene::SceneObject& o, const DrawOptions& options, std::vector<Segment>& out) = nullptr;
};

// The entry for a type id ("vector", "frame", ...), or nullptr for types the view doesn't draw (parameters)
const ObjectView* objectView(const std::string& typeId);

// Each family file fills in its types (called once, when the table is built)
using ObjectViewTable = std::map<std::string, ObjectView>;
void registerBasicViews(ObjectViewTable& table);       // vector, matrix, eigen, plots, field
void registerComplexViews(ObjectViewTable& table);     // complex, product, epicycles
void registerQuaternionViews(ObjectViewTable& table);  // quaternion
void registerCalculusViews(ObjectViewTable& table);    // integrals, frame, tangent, tangent plane, velocity
void registerFourierViews(ObjectViewTable& table);     // Fourier series

} // namespace linalg::view
