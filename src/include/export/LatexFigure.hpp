#pragma once

#include "export/LatexExport.hpp"
#include "math/Expression.hpp"
#include "math/Vector3D.hpp"
#include <sstream>
#include <string>
#include <vector>

// Building blocks of the LaTeX export, shared by LatexExport.cpp (the axis around the figure) and
// LatexObjects.cpp (one function per object type). Internal: only the export's own files include this.
// No raylib here, so the export is unit-tested without a window.
namespace linalg::latex::detail {

constexpr double kPi = 3.14159265358979323846;
constexpr double kOffChart = 50.0;  // plot values beyond this are left out (asymptotes)
inline const char* const kArrow = "-{Stealth[length=5pt, width=3.5pt]}";

// A number for TikZ: up to 4 significant digits, tiny values as 0, "nan" when not finite
std::string num(double v);

struct Point3 {
    double x, y, z;
};

// Collects the figure: body lines plus the extent of everything drawn
struct Figure {
    bool flat = false;
    std::ostringstream body;
    double lo[3] = {0, 0, 0}, hi[3] = {0, 0, 0};
    bool any = false;
    bool plots = false;   // functions/surfaces: no equal scaling
    bool legend = false;  // at least one legend entry
    bool surfaces = false;
    bool fields3D = false;  // a volume vector field: equal unit lengths on all three axes
    std::vector<Point3> labelPoints;

    void extend(double x, double y, double z = 0.0) {
        double p[3] = {x, y, z};
        for (int k = 0; k < 3; k++) {
            if (!std::isfinite(p[k])) return;
        }
        for (int k = 0; k < 3; k++) {
            lo[k] = std::min(lo[k], p[k]);
            hi[k] = std::max(hi[k], p[k]);
        }
        any = true;
    }
    std::string at(double x, double y, double z = 0.0) {
        extend(x, y, z);
        return flat ? "(axis cs:" + num(x) + "," + num(y) + ")"
                    : "(axis cs:" + num(x) + "," + num(y) + "," + num(z) + ")";
    }
    std::string coord(double x, double y, double z = 0.0) const {
        return flat ? "(" + num(x) + "," + num(y) + ")" : "(" + num(x) + "," + num(y) + "," + num(z) + ")";
    }
};

// Line options of an object's style ("dashed, black!65"), plus extras
std::string styleOptions(const Style& s, const char* extra = nullptr);
std::string plotStyle(const Style& s, const char* extra);
// Label placement so the text sits beyond the tip (2D), above right in 3D
std::string anchorFor(double dx, double dy, bool flat);
// A label node at p; labels that land on the same point are stacked, one line apart
std::string labelNode(Figure& f, const Point3& p, double dx, double dy, const std::string& label);
void arrow(Figure& f, const Point3& from, const Point3& to, const std::string& options, const std::string& label);
void line(Figure& f, const Point3& a, const Point3& b, const std::string& options);
// An arc of a circle around the origin (complex plane angles, the unit circle)
void arc(Figure& f, double radius, double from, double to, const std::string& options);
void legendEntry(Figure& f, const std::string& text, bool legend);
bool isZ0(const Vector3D& v);
bool isGreek(const std::string& s);
bool usable(double v);  // finite and not off the chart

// Evaluate a plot field the way the app does (other objects by name, t = scene time)
struct Sampler {
    Expression e;
    const expr::Resolver* resolver;
    double time;
    bool ok;
    Sampler(const std::string& text, const expr::Resolver* r, double t, bool curve) : resolver(r), time(t) {
        ok = e.parse(text, true, curve);
    }
    double at(double x, double y, double u = 0.0) const { return e.eval(ExprVars{x, y, time, u}, resolver); }
};

// What the export needs besides the object: the space (other objects by name) and the options
struct ExportInputs {
    const scene::Space& space;
    const Options& options;
};

// How one object type is written: the drawing commands, and whether it lies on the floor (z = 0), so the
// Auto view can use a 2D axis. flat == nullptr: it needs a 3D axis.
struct ObjectExport {
    void (*write)(Figure& f, const scene::SceneObject& o, const Style& s, const ExportInputs& in) = nullptr;
    bool (*flat)(const scene::SceneObject& o) = nullptr;
};
// The entry for a type id, or nullptr (parameters are not drawn)
const ObjectExport* objectExport(const std::string& typeId);

} // namespace linalg::latex::detail
