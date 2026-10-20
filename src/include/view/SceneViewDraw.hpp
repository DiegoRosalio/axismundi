#pragma once

#include "math/Integrals.hpp"
#include "math/Matrix3D.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"
#include "view/SceneView.hpp"
#include <raylib.h>
#include <vector>

// Drawing helpers shared by the SceneView source files (not part of SceneView's interface)
namespace linalg::view::draw {

constexpr float kQuatAxisLength = 2.0f;  // a quaternion's axis arrow (its label sits at the tip)

Vector3D toVec(Vector3 v);
Vector3 w3(const integrals::D3& p);  // math (z up) to world
bool isSelected(const DrawOptions& o, const scene::SceneObject& obj);
Color withAlpha(Color c, unsigned char a);
// Blended toward the view's background (unselected objects while something is selected)
Color faded(Color c);
Color mixColor(Color a, Color b, float k);
Color paletteColor(const std::vector<Color>& palette, int index);
// The second color of an object: f', the gradient, the acceleration
Color derivativeColor(const scene::SceneObject& o);

// A point marker: a sphere of the given world radius, kept between about 5 and 14 pixels across on screen
void dot(Vector3 at, float worldRadius, Color color);
// Arrow between two world points; selected arrows are a bit thicker with a narrow outline
void arrow(Vector3 from, Vector3 to, Color color, bool selected);
void dashed(Vector3 a, Vector3 b, Color color, float dash = 0.12f);
// Arc in the complex plane (world XY), radius r, from angle a0 to a1
void arcXY(float r, float a0, float a1, Color color);
// A matrix's moved basis (alpha: transparency, fade: toward the background)
void basis(const Matrix4D& m, float alpha, bool selected, bool fade = false);

// Print preview (the LaTeX export's styles)
Color grayOf(int styleIndex);
int dashOf(int styleIndex);
void printLine(Vector3 a, Vector3 b, Color color, int dash, float radius);
void printArrow(Vector3 from, Vector3 to, Color color, int dash, bool selected);

// Quaternions
Matrix4D embed3(const Matrix3D& m);
Vector3D perpendicular(const Vector3D& a);  // a unit vector at right angles to a
std::vector<Vector3D> arcPoints(const Vector3D& center, const Vector3D& axis, const Vector3D& start, double angle);
void tubeAlong(const std::vector<Vector3D>& pts, float radius, Color color);
void angleArc(const std::vector<Vector3D>& pts, float radius, Color color);  // with an arrowhead

} // namespace linalg::view::draw
