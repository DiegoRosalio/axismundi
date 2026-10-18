// One export function per object type (the drawing commands of the figure), and the table that maps a
// type id to its function and to its "lies on the floor" test (see LatexFigure.hpp)
#include "export/LatexFigure.hpp"
#include "math/Contour.hpp"
#include "math/Decompositions.hpp"
#include "math/VectorField.hpp"
#include "scene/Types.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>

namespace linalg::latex {

using namespace detail;

namespace {

// --- Per object --------------------------------------------------------------------

void exportVector(Figure& f, const scene::SceneObject& o, const Style& s) {
    const auto& v = static_cast<const scene::VectorObject&>(o);
    Vector3D tail = v.tail(), tip = tail + o.value().vector;
    arrow(f, {tail.x, tail.y, tail.z}, {tip.x, tip.y, tip.z}, styleOptions(s, "thick"),
          "\\vec{" + nameToLatex(o.name()) + "}");
}

void exportMatrix(Figure& f, const scene::SceneObject& o, const Style& s) {
    const expr::Value& v = o.value();
    const Matrix4D& m = v.matrix;
    Point3 origin = {0, 0, 0};
    if (v.size == 4) origin = {m.m[0][3], m.m[1][3], m.m[2][3]};
    Point3 c[3];
    for (int k = 0; k < 3; k++) c[k] = {origin.x + m.m[0][k], origin.y + m.m[1][k], origin.z + m.m[2][k]};
    std::string name = nameToLatex(o.name());

    // The original basis (dotted) and the image of the unit square / cube (light gray)
    int dims = f.flat ? 2 : 3;
    for (int k = 0; k < dims; k++) {
        Point3 e = {k == 0 ? 1.0 : 0.0, k == 1 ? 1.0 : 0.0, k == 2 ? 1.0 : 0.0};
        line(f, {0, 0, 0}, e, "black!45, dotted");
    }
    auto corner = [&](int a, int b, int cc) {
        return Point3{origin.x + a * (c[0].x - origin.x) + b * (c[1].x - origin.x) + cc * (c[2].x - origin.x),
                      origin.y + a * (c[0].y - origin.y) + b * (c[1].y - origin.y) + cc * (c[2].y - origin.y),
                      origin.z + a * (c[0].z - origin.z) + b * (c[1].z - origin.z) + cc * (c[2].z - origin.z)};
    };
    if (f.flat) {
        f.body << "\\fill[black!8] " << f.at(origin.x, origin.y) << " -- " << f.at(corner(1, 0, 0).x, corner(1, 0, 0).y)
               << " -- " << f.at(corner(1, 1, 0).x, corner(1, 1, 0).y) << " -- "
               << f.at(corner(0, 1, 0).x, corner(0, 1, 0).y) << " -- cycle;\n";
    } else {
        for (int a = 0; a < 2; a++)
            for (int b = 0; b < 2; b++)
                for (int cc = 0; cc < 2; cc++) {
                    if (a == 0) line(f, corner(a, b, cc), corner(1, b, cc), "black!30, very thin");
                    if (b == 0) line(f, corner(a, b, cc), corner(a, 1, cc), "black!30, very thin");
                    if (cc == 0) line(f, corner(a, b, cc), corner(a, b, 1), "black!30, very thin");
                }
    }
    for (int k = 0; k < dims; k++)
        arrow(f, origin, c[k], styleOptions(s, "thick"), name + "\\,e_{" + std::to_string(k + 1) + "}");
}

// A quaternion as its rotation: axis (dashed line + arrow), angle arc, moved basis, and a vector
// before and after when "acts on" is on
void exportQuaternion(Figure& f, const scene::QuaternionObject& o, const Style& s) {
    expr::Rotation r;
    if (!expr::rotationOf(o.value(), r)) return;
    std::string name = nameToLatex(o.name());
    auto P = [](const Vector3D& v) { return Point3{v.x, v.y, v.z}; };
    auto arcDraw = [&](const Vector3D& center, const Vector3D& start, double angle, const std::string& options) {
        int n = std::max(2, static_cast<int>(std::fabs(angle) / 0.1) + 2);
        Vector3D side = r.axis.cross(start);
        f.body << "\\draw[" << options << "] ";
        for (int i = 0; i <= n; i++) {
            float a = static_cast<float>(angle * i / n);
            Vector3D p = center + start * std::cos(a) + side * std::sin(a);
            f.body << (i ? " -- " : "") << f.at(p.x, p.y, p.z);
        }
        f.body << ";\n";
    };
    if (o.showBasis()) {
        int dims = f.flat ? 2 : 3;
        for (int k = 0; k < dims; k++) {
            Vector3D e(k == 0 ? 1.0f : 0.0f, k == 1 ? 1.0f : 0.0f, k == 2 ? 1.0f : 0.0f);
            line(f, {0, 0, 0}, P(e), "black!45, dotted");
            arrow(f, {0, 0, 0}, P(r.matrix.transform(e)), styleOptions(s, "thick"),
                  name + "\\,e_{" + std::to_string(k + 1) + "}");
        }
    }
    if (o.showAxis() && r.hasAxis) {
        Vector3D a = r.axis;
        if (!f.flat) {  // in 2D the axis points out of the page: only the arc is drawn
            line(f, P(a * -2.6f), P(a * 2.6f), std::string("densely dashed, ") + s.gray + ", thin");
            arrow(f, {0, 0, 0}, P(a * 2.0f), styleOptions(s, "semithick"), name);
        }
        Vector3D u = a.cross(std::fabs(a.x) < 0.9f ? Vector3D(1, 0, 0) : Vector3D(0, 1, 0)).normalized();
        arcDraw(Vector3D::zero(), u, r.angle, std::string(kArrow) + ", " + s.gray + ", semithick");
        if (f.flat) {
            Vector3D mid = u * std::cos(static_cast<float>(r.angle / 2)) + a.cross(u) * std::sin(static_cast<float>(r.angle / 2));
            f.body << "\\node[" << s.gray << ", inner sep=2pt] at " << f.at(mid.x * 1.25f, mid.y * 1.25f) << " {$" << name
                   << "$};\n";
        }
    }
    if (o.actsOnVector()) {
        const scene::Field* vf = o.field("v");
        if (vf && vf->value.type == expr::Type::Vector) {
            Vector3D v = vf->value.vector, w = r.matrix.transform(v);
            Vector3D along = r.axis * r.axis.dot(v);
            arcDraw(along, v - along, r.angle, "black!45, densely dotted");
            arrow(f, {0, 0, 0}, P(v), "black!50, thick", "\\vec v");
            arrow(f, {0, 0, 0}, P(w), styleOptions(s, "thick"), name + "\\,\\vec v\\," + name + "^{-1}");
        }
    }
}

void exportEigen(Figure& f, const scene::SceneObject& o, const Style& s) {
    EigenResult er = eigen3(o.value().matrix3());
    std::string name = nameToLatex(o.name());
    for (const Eigenvalue& ev : er.values) {
        if (!ev.isReal() || !ev.hasVector) continue;
        Vector3D d = ev.vector.normalized();
        double len = std::max(2.0, std::fabs(ev.re) * 1.3);
        Point3 a = {-d.x * len, -d.y * len, -d.z * len}, b = {d.x * len, d.y * len, d.z * len};
        line(f, a, b, std::string("densely dashed, ") + s.gray + ", thin");
        Point3 p = {d.x * ev.re, d.y * ev.re, d.z * ev.re};
        f.body << "\\fill[" << s.gray << "] " << f.at(p.x, p.y, p.z) << " circle[radius=1.2pt]"
               << labelNode(f, p, d.x, d.y, "\\lambda = " + num(ev.re)) << ";\n";
    }
    (void)name;
}

void exportField(Figure& f, const scene::VectorFieldObject& o, const Style& s, const scene::Space& space,
                 const Options& opt) {
    expr::Program program;
    const scene::Field* formula = o.field("F");
    if (!formula || !program.compile(formula->text, expr::kVarZ)) return;
    bool plane = !o.volume();
    field::FieldFunction fn(program, &space, o.time(), plane);
    auto range = [&](const char* key, double fb) { return static_cast<float>(o.rangeValue(key, fb)); };
    field::Box box{{range("xmin", -4), range("ymin", -4), plane ? 0.0f : range("zmin", -2)},
                   {range("xmax", 4), range("ymax", 4), plane ? 0.0f : range("zmax", 2)}};
    scene::VectorFieldObject::Counts c = o.counts();
    // Fewer arrows than on screen, so the document compiles quickly
    int cap = plane ? opt.fieldArrows : std::max(2, opt.fieldArrows / 3);
    field::ArrowSet arrows =
        field::sampleArrows(fn, box, std::min(c.nx, cap), std::min(c.ny, cap), std::min(c.nz, cap), o.sameLength());
    std::string name = nameToLatex(o.name());

    // Arrows: a PGFPlots quiver table (x y [z] u v [w]), scaled like on screen
    bool three = !f.flat;
    if (three) f.fields3D = true;
    f.body << (three ? "\\addplot3[" : "\\addplot[") << s.gray << ", thin, -{Stealth[length=3pt, width=2.2pt]}, "
           << "quiver={u=\\thisrow{u}, v=\\thisrow{v}" << (three ? ", w=\\thisrow{w}" : "") << "}]\n"
           << "  table[row sep=crcr] {" << (three ? "x y z u v w" : "x y u v") << "\\\\\n";
    for (const field::Arrow& a : arrows.arrows) {
        f.body << "  " << num(a.base.x) << " " << num(a.base.y);
        if (three) f.body << " " << num(a.base.z);
        f.body << " " << num(a.vec.x) << " " << num(a.vec.y);
        if (three) f.body << " " << num(a.vec.z);
        f.body << "\\\\\n";
        f.extend(a.base.x, a.base.y, three ? a.base.z : 0);
        f.extend(a.base.x + a.vec.x, a.base.y + a.vec.y, three ? a.base.z + a.vec.z : 0);
    }
    f.body << "};\n";
    legendEntry(f, "\\vec{" + name + "}", opt.legend);

    // Flow lines, and particles where they are at the current time
    if (!o.lines() && !o.particles()) return;
    // A volume's lines are hard to read on paper: at most 3 per axis there
    std::vector<field::FlowLine> lines = field::fieldFlowLines(fn, box, plane ? o.seeds() : std::min(o.seeds(), 3));
    if (o.lines()) {
        for (const field::FlowLine& line : lines) {
            size_t stride = std::max<size_t>(1, line.points.size() / 120);
            f.body << (three ? "\\addplot3[" : "\\addplot[") << plotStyle(s, "thin, forget plot") << "] coordinates {";
            int k = 0;
            for (size_t i = 0; i < line.points.size(); i += stride, k++) {
                const Vector3D& p = line.points[i];
                f.body << (k % 6 == 0 ? "\n  " : " ") << f.coord(p.x, p.y, p.z);
            }
            const Vector3D& last = line.points.back();
            f.body << " " << f.coord(last.x, last.y, last.z) << "\n};\n";
        }
    }
    if (o.particles()) {
        f.body << (three ? "\\addplot3[" : "\\addplot[")
               << "only marks, mark=*, mark size=0.9pt, black, forget plot] coordinates {";
        int k = 0;
        const int perLine = plane ? 3 : 2;
        for (const field::FlowLine& line : lines)
            for (int j = 0; j < perLine; j++, k++) {
                Vector3D p = field::particleAt(line, field::particleTime(line, j, perLine, o.time(), o.speed()));
                f.body << (k % 6 == 0 ? "\n  " : " ") << f.coord(p.x, p.y, p.z);
            }
        f.body << "\n};\n";
    }
}

// The volume under z = f(x, y): the boxes (top faces light gray, edges thin) and the surface's grid lines
void exportDoubleIntegral(Figure& f, const scene::DoubleIntegralObject& o, const Style& s) {
    const auto& boxes = o.riemann().boxes;
    std::string name = nameToLatex(o.name());
    double top = 0;
    if (boxes.size() <= 144) {  // at most 12 × 12, so the document stays light
        for (const auto& b : boxes) {
            double h = b.height;
            top = std::max(top, h);
            f.body << "\\fill[black!10] " << f.at(b.x0, b.y0, h) << " -- " << f.at(b.x1, b.y0, h) << " -- "
                   << f.at(b.x1, b.y1, h) << " -- " << f.at(b.x0, b.y1, h) << " -- cycle;\n";
            const double xs[2] = {b.x0, b.x1}, ys[2] = {b.y0, b.y1};
            for (double x : xs)
                for (double y : ys) line(f, {x, y, 0}, {x, y, h}, "black!45, very thin");
            for (double z : {0.0, h}) {
                f.body << "\\draw[black!55, very thin] " << f.at(b.x0, b.y0, z) << " -- " << f.at(b.x1, b.y0, z)
                       << " -- " << f.at(b.x1, b.y1, z) << " -- " << f.at(b.x0, b.y1, z) << " -- cycle;\n";
            }
        }
    } else {
        f.body << "% " << boxes.size() << " boxes: too many to draw, only the surface\n";
    }
    for (const auto& l : o.surfaceLines()) {
        if (l.size() < 2) continue;
        f.body << "\\addplot3[" << plotStyle(s, "thin, forget plot") << "] coordinates {";
        for (size_t i = 0; i < l.size(); i += 2) {
            f.body << (i % 12 == 0 ? "\n  " : " ") << f.coord(l[i].x, l[i].y, l[i].z);
            f.extend(l[i].x, l[i].y, l[i].z);
            top = std::max(top, l[i].z);
        }
        f.body << " " << f.coord(l.back().x, l.back().y, l.back().z) << "\n};\n";
    }
    integrals::Rect r = o.rect();
    // Beside the region (the middle top is where the z axis and its ticks are)
    f.body << "\\node[anchor=west, inner sep=3pt] at " << f.at(r.x1, r.y1, top) << " {$" << name << " \\approx "
           << num(o.exact()) << "$};\n";
}

// The work along a curve: the path, F at a few points along it, the marker and the value
void exportLineIntegral(Figure& f, const scene::LineIntegralObject& o, const Style& s) {
    const auto& path = o.path();
    if (path.size() < 2) return;
    std::string name = nameToLatex(o.name());
    f.body << (f.flat ? "\\addplot[" : "\\addplot3[") << plotStyle(s, "thick, forget plot") << "] coordinates {";
    for (size_t i = 0; i < path.size(); i += 2) {
        f.body << (i % 12 == 0 ? "\n  " : " ") << f.coord(path[i].p.x, path[i].p.y, path[i].p.z);
        f.extend(path[i].p.x, path[i].p.y, f.flat ? 0 : path[i].p.z);
    }
    f.body << " " << f.coord(path.back().p.x, path.back().p.y, path.back().p.z) << "\n};\n";
    double longest = 0;
    for (const auto& a : o.arrows()) longest = std::max(longest, std::sqrt(a.f.x * a.f.x + a.f.y * a.f.y + a.f.z * a.f.z));
    if (longest > 1e-12) {
        double k = 0.7 / longest;
        for (const auto& a : o.arrows())
            arrow(f, {a.p.x, a.p.y, a.p.z}, {a.p.x + a.f.x * k, a.p.y + a.f.y * k, a.p.z + a.f.z * k},
                  "black!60, thin", "");
    }
    integrals::D3 m = o.marker();
    f.body << "\\fill " << f.at(m.x, m.y, m.z) << " circle[radius=1.5pt]"
           << labelNode(f, {m.x, m.y, m.z}, 1, -1, name + " = " + num(o.value().re)) << ";\n";
}

// The moving frame: the curve, T/N/B at the point, the osculating circle
void exportFrame(Figure& f, const scene::FrameObject& o, const Style& s) {
    const auto& curve = o.curve();
    std::string name = nameToLatex(o.name());
    if (o.showCurve() && curve.size() >= 2) {
        f.body << (f.flat ? "\\addplot[" : "\\addplot3[") << plotStyle(s, "thick, forget plot") << "] coordinates {";
        for (size_t i = 0; i < curve.size(); i += 2) {
            f.body << (i % 12 == 0 ? "\n  " : " ") << f.coord(curve[i].x, curve[i].y, curve[i].z);
            f.extend(curve[i].x, curve[i].y, f.flat ? 0 : curve[i].z);
        }
        f.body << " " << f.coord(curve.back().x, curve.back().y, curve.back().z) << "\n};\n";
    }
    if (!o.error().empty()) return;
    const geometry::Frame& fr = o.frame();
    Point3 p = {fr.p.x, fr.p.y, fr.p.z};
    if (o.showCircle() && fr.hasNormal && fr.kappa > 1e-3 && 1.0 / fr.kappa < 60.0) {
        double radius = 1.0 / fr.kappa;
        geometry::D3 c = geometry::add(fr.p, geometry::scale(fr.N, radius));
        f.body << "\\draw[black!45, thin] ";
        for (int i = 0; i <= 72; i++) {
            double a = 2 * kPi * i / 72;
            geometry::D3 q = geometry::add(c, geometry::add(geometry::scale(fr.N, -radius * std::cos(a)),
                                                            geometry::scale(fr.T, radius * std::sin(a))));
            f.body << (i ? " -- " : "") << f.at(q.x, q.y, q.z);
        }
        f.body << ";\n";
    }
    if (o.showFrame()) {
        const geometry::D3 dirs[3] = {fr.T, fr.N, fr.B};
        const char* labels[3] = {"T", "N", "B"};
        for (int i = 0; i < 3; i++) {
            if (i > 0 && !fr.hasNormal) continue;
            if (i == 2 && f.flat) continue;  // B points out of the page
            arrow(f, p, {p.x + dirs[i].x, p.y + dirs[i].y, p.z + dirs[i].z}, "black, semithick", labels[i]);
        }
    }
    f.body << "\\fill " << f.at(p.x, p.y, p.z) << " circle[radius=1.5pt]"
           << labelNode(f, p, -1, -1, name) << ";\n";
}

// The tangent line: f (the object's style), f' (dashed gray), the tangent through the point, the secant
void exportTangent(Figure& f, const scene::TangentObject& o, const Style& s) {
    std::string name = nameToLatex(o.name());
    f.plots = true;
    auto curve = [&](const scene::TangentObject::Samples& pts, const std::string& style, bool grow) {
        f.body << (f.flat ? "\\addplot[" : "\\addplot3[") << style << "] coordinates {";
        int k = 0;
        for (size_t i = 0; i < pts.size(); i += 2, k++) {
            double x = pts[i].first, y = pts[i].second;
            bool ok = usable(y) && std::fabs(y) <= 12;
            f.body << (k % 6 == 0 ? "\n  " : " ") << (ok ? f.coord(x, y, 0) : f.coord(NAN, NAN, NAN));
            if (ok && grow) f.extend(x, y, 0);
        }
        f.body << "\n};\n";
    };
    if (o.showFunction()) curve(o.function(), plotStyle(s, "thick, forget plot"), true);
    if (o.showDerivative()) curve(o.derivative(), "black!50, densely dashed, thin, forget plot", true);
    double x0 = o.x0(), y0 = o.y0(), m = o.slope();
    auto through = [&](double slope, double half, const char* style) {
        double dx = half / std::sqrt(1 + slope * slope);
        line(f, {x0 - dx, y0 - slope * dx, 0}, {x0 + dx, y0 + slope * dx, 0}, style);
    };
    if (o.hasSecant()) {
        through(o.secantSlope(), 3.5, "black!45, thin");
        f.body << "\\fill[black!45] " << f.at(x0 + o.secantH(), y0 + o.secantSlope() * o.secantH(), 0)
               << " circle[radius=1.2pt];\n";
    }
    if (o.showTriangle() && std::fabs(m) < 20) {
        f.body << "\\draw[black!50, thin] " << f.at(x0, y0, 0) << " -- " << f.at(x0 + 1, y0, 0) << " node[midway, below, "
               << "font=\\scriptsize] {$1$} -- " << f.at(x0 + 1, y0 + m, 0) << " node[midway, right, font=\\scriptsize] {$"
               << num(m) << "$};\n";
    }
    through(m, 3.0, "black, semithick");
    f.body << "\\fill " << f.at(x0, y0, 0) << " circle[radius=1.5pt]"
           << labelNode(f, {x0, y0, 0}, 1, -1, name + ": f'(" + num(x0) + ") = " + num(m)) << ";\n";
}

// The tangent plane: the surface mesh, the plane patch, the slopes, the gradient on the floor
void exportTangentPlane(Figure& f, const scene::TangentPlaneObject& o, const Style& s) {
    std::string name = nameToLatex(o.name());
    if (o.showSurface()) {
        for (const auto& l : o.surfaceLines()) {
            if (l.size() < 2) continue;
            f.body << "\\addplot3[" << plotStyle(s, "thin, forget plot") << "] coordinates {";
            for (size_t i = 0; i < l.size(); i += 2) {
                f.body << (i % 12 == 0 ? "\n  " : " ") << f.coord(l[i].x, l[i].y, l[i].z);
                f.extend(l[i].x, l[i].y, l[i].z);
            }
            f.body << " " << f.coord(l.back().x, l.back().y, l.back().z) << "\n};\n";
        }
    }
    auto p = o.point();
    double fx = o.fx(), fy = o.fy();
    auto onPlane = [&](double dx, double dy) { return Point3{p.x + dx, p.y + dy, p.z + fx * dx + fy * dy}; };
    if (o.showPlane()) {
        const double h = scene::TangentPlaneObject::kPatch;
        Point3 c[4] = {onPlane(-h, -h), onPlane(h, -h), onPlane(h, h), onPlane(-h, h)};
        f.body << "\\filldraw[fill=black!10, fill opacity=0.6, draw=black!60, thin] ";
        for (int i = 0; i < 4; i++) {
            f.body << f.at(c[i].x, c[i].y, c[i].z) << " -- ";
            f.extend(c[i].x, c[i].y, c[i].z);
        }
        f.body << "cycle;\n";
    }
    Point3 at = {p.x, p.y, p.z};
    if (o.showSlopes()) {
        arrow(f, at, onPlane(1, 0), "black, semithick", "");
        arrow(f, at, onPlane(0, 1), "black, semithick", "");
    }
    if (o.showNormal()) {
        double len = std::sqrt(fx * fx + fy * fy + 1);
        arrow(f, at, {p.x - fx / len, p.y - fy / len, p.z + 1 / len}, "black!70, thin", "n");
    }
    if (o.showGradient()) {
        line(f, {p.x, p.y, 0}, at, "black!40, densely dashed, thin");
        if (std::hypot(fx, fy) > 1e-6) {
            arrow(f, {p.x, p.y, 0}, {p.x + fx, p.y + fy, 0}, "black, thick", "\\nabla f");
            f.extend(p.x + fx, p.y + fy, 0);
        }
    }
    f.body << "\\fill " << f.at(p.x, p.y, p.z) << " circle[radius=1.5pt]" << labelNode(f, at, -1, 1, name) << ";\n";
}

// Velocity and acceleration: the curve, r' and r'' at the point
void exportVelocity(Figure& f, const scene::VelocityObject& o, const Style& s) {
    const auto& curve = o.curve();
    std::string name = nameToLatex(o.name());
    if (o.showCurve() && curve.size() >= 2) {
        f.body << (f.flat ? "\\addplot[" : "\\addplot3[") << plotStyle(s, "thick, forget plot") << "] coordinates {";
        for (size_t i = 0; i < curve.size(); i += 2) {
            f.body << (i % 12 == 0 ? "\n  " : " ") << f.coord(curve[i].x, curve[i].y, curve[i].z);
            f.extend(curve[i].x, curve[i].y, f.flat ? 0 : curve[i].z);
        }
        f.body << " " << f.coord(curve.back().x, curve.back().y, curve.back().z) << "\n};\n";
    }
    auto p = o.point();
    double k = o.arrowScale();
    Point3 at = {p.x, p.y, p.z};
    auto tip = [&](const integrals::D3& v) { return Point3{p.x + v.x * k, p.y + v.y * k, p.z + v.z * k}; };
    auto grow = [&](const Point3& q) { f.extend(q.x, q.y, f.flat ? 0 : q.z); };
    if (o.showAcceleration()) {
        if (o.splitAcceleration()) {
            arrow(f, at, tip(o.tangential()), "black!45, thin", "");
            arrow(f, at, tip(o.normal()), "black!45, thin", "");
        }
        arrow(f, at, tip(o.acceleration()), "black!60, semithick", "r''");
        grow(tip(o.acceleration()));
    }
    if (o.showVelocity()) {
        arrow(f, at, tip(o.velocity()), "black, thick", "r'");
        grow(tip(o.velocity()));
    }
    f.body << "\\fill " << f.at(p.x, p.y, p.z) << " circle[radius=1.5pt]" << labelNode(f, at, -1, -1, name) << ";\n";
}

void exportPlot(Figure& f, const scene::PlotObject& p, const Style& s, const scene::Space& space,
                const Options& opt) {
    auto text = [&](const char* key) { return p.field(key) ? p.field(key)->text : std::string(); };
    auto range = [&](const char* key, double fb) { return p.rangeValue(key, fb); };
    scene::PlotObject::Samples samples = p.samples();
    double time = p.time();
    std::string name = nameToLatex(p.name());
    f.plots = true;
    auto pt = [&](double x, double y, double z) { return f.coord(x, y, z); };

    switch (p.kind()) {
        case scene::PlotObject::Kind::Function: {
            Sampler fs(text("f"), &space, time, false);
            if (!fs.ok) return;
            double a = range("xmin", -5), b = range("xmax", 5);
            int n = std::max(2, std::min(opt.curveSamples, samples.nx));
            f.body << (f.flat ? "\\addplot[" : "\\addplot3[") << plotStyle(s, "thick") << "] coordinates {";
            double prev = std::numeric_limits<double>::quiet_NaN();
            for (int i = 0; i <= n; i++) {
                double x = a + (b - a) * i / n, y = fs.at(x, 0);
                bool ok = usable(y);
                // A big jump with a sign change is an asymptote (e.g. tan x): break the line
                if (ok && std::isfinite(prev) && y * prev < 0 && std::fabs(y - prev) > 20.0)
                    f.body << " " << pt(NAN, NAN, NAN);
                f.body << (i % 6 == 0 ? "\n  " : " ") << (ok ? pt(x, y, 0) : pt(NAN, NAN, NAN));
                if (ok) f.extend(x, y, 0);
                prev = ok ? y : std::numeric_limits<double>::quiet_NaN();
            }
            f.body << "\n};\n";
            legendEntry(f, "y = " + name + "(x)", opt.legend);
            break;
        }
        case scene::PlotObject::Kind::Curve: {
            Sampler xs(text("x"), &space, time, true), ys(text("y"), &space, time, true),
                zs(text("z"), &space, time, true);
            if (!xs.ok || !ys.ok || !zs.ok) return;
            double a = range("umin", 0), b = range("umax", 6.28);
            int n = std::max(2, std::min(opt.curveSamples, samples.nx));
            f.body << (f.flat ? "\\addplot[" : "\\addplot3[") << plotStyle(s, "thick") << "] coordinates {";
            for (int i = 0; i <= n; i++) {
                double u = a + (b - a) * i / n;
                double x = xs.at(0, 0, u), y = ys.at(0, 0, u), z = zs.at(0, 0, u);
                bool ok = usable(x) && usable(y) && usable(z);
                f.body << (i % 4 == 0 ? "\n  " : " ") << (ok ? pt(x, y, z) : pt(NAN, NAN, NAN));
                if (ok) f.extend(x, y, f.flat ? 0 : z);
            }
            f.body << "\n};\n";
            legendEntry(f, name, opt.legend);
            break;
        }
        case scene::PlotObject::Kind::Surface: {
            Sampler zs(text("f"), &space, time, false);
            if (!zs.ok) return;
            double x0 = range("xmin", -5), x1 = range("xmax", 5), y0 = range("ymin", -5), y1 = range("ymax", 5);
            int nx = std::max(2, std::min(opt.surfaceSamples, samples.nx));
            int ny = std::max(2, std::min(opt.surfaceSamples, samples.ny));
            f.surfaces = true;
            f.body << "\\addplot3[surf, shader=faceted, faceted color=black, line width=0.25pt, "
                   << (opt.grayShading ? "colormap={paper}{gray(0cm)=(0.55); gray(1cm)=(0.97)}"
                                       : "colormap={paper}{gray(0cm)=(1); gray(1cm)=(1)}")
                   << ", z buffer=sort, mesh/rows=" << (ny + 1) << "] coordinates {";
            for (int j = 0; j <= ny; j++) {
                double y = y0 + (y1 - y0) * j / ny;
                f.body << "\n ";
                for (int i = 0; i <= nx; i++) {
                    double x = x0 + (x1 - x0) * i / nx, z = zs.at(x, y);
                    if (usable(z)) {
                        f.body << " " << pt(x, y, z);
                        f.extend(x, y, z);
                    } else {
                        f.body << " " << pt(x, y, NAN);
                    }
                }
            }
            f.body << "\n};\n";
            legendEntry(f, "z = " + name + "(x, y)", opt.legend);
            break;
        }
        case scene::PlotObject::Kind::Implicit: {
            Sampler fs(text("f"), &space, time, false);
            if (!fs.ok) return;
            double x0 = range("xmin", -5), x1 = range("xmax", 5), y0 = range("ymin", -5), y1 = range("ymax", 5);
            int nx = std::max(4, std::min(opt.implicitSamples, samples.nx));
            int ny = std::max(4, std::min(opt.implicitSamples, samples.ny));
            std::vector<double> v((nx + 1) * (ny + 1));
            for (int j = 0; j <= ny; j++)
                for (int i = 0; i <= nx; i++)
                    v[j * (nx + 1) + i] = fs.at(x0 + (x1 - x0) * i / nx, y0 + (y1 - y0) * j / ny);
            double cell = std::min((x1 - x0) / nx, (y1 - y0) / ny);
            std::vector<Polyline> lines = chainSegments(marchingSquares(v, nx, ny, x0, x1, y0, y1), cell * 1e-3);
            if (lines.empty()) return;
            f.body << (f.flat ? "\\addplot[" : "\\addplot3[") << plotStyle(s, "thick") << "] coordinates {";
            int count = 0;
            for (size_t k = 0; k < lines.size(); k++) {
                if (k) f.body << " " << pt(NAN, NAN, NAN);
                for (const auto& [x, y] : lines[k]) {
                    f.body << (count++ % 5 == 0 ? "\n  " : " ") << pt(x, y, 0);
                    f.extend(x, y, 0);
                }
            }
            f.body << "\n};\n";
            legendEntry(f, name + "(x, y) = 0", opt.legend);
            break;
        }
    }
}

// The Fourier series: f (dashed gray) and the partial sum S_N (the object's style)
void exportFourierSeries(Figure& f, const scene::FourierSeriesObject& o, const Style& s, const Options& opt) {
    std::string name = nameToLatex(o.name());
    f.plots = true;
    auto curve = [&](const std::vector<std::pair<double, double>>& pts, const std::string& style, size_t stride) {
        f.body << (f.flat ? "\\addplot[" : "\\addplot3[") << style << "] coordinates {";
        int k = 0;
        for (size_t i = 0; i < pts.size(); i += stride, k++) {
            double x = pts[i].first, y = pts[i].second;
            bool ok = usable(y);
            f.body << (k % 6 == 0 ? "\n  " : " ") << (ok ? f.coord(x, y, 0) : f.coord(NAN, NAN, NAN));
            if (ok) f.extend(x, y, 0);
        }
        f.body << "\n};\n";
    };
    if (o.showOriginal()) curve(o.original(), "black!50, densely dashed, thin, forget plot", 2);
    size_t stride = std::max<size_t>(1, o.sum().size() / static_cast<size_t>(std::max(2, opt.curveSamples * 2)));
    curve(o.sum(), plotStyle(s, "thick"), stride);
    legendEntry(f, name + "_{" + std::to_string(o.terms()) + "}", opt.legend);
}

// Epicycles: the target curve (light), the circles and arms at the current angle, the last lap traced
void exportEpicycles(Figure& f, const scene::EpicyclesObject& o, const Style& s) {
    const auto& target = o.target();
    if (!target.empty()) {
        f.body << "\\draw[black!25, thin] ";
        for (size_t i = 0; i <= target.size(); i += 4) {
            const auto& z = target[i % target.size()];
            f.body << (i ? " -- " : "") << f.at(z.real(), z.imag());
        }
        f.body << " -- cycle;\n";
    }
    if (!o.error().empty() || o.terms().empty()) return;
    int count = o.circles() + 1;
    double u = o.angle();
    double start = std::max(0.0, u - 2 * kPi);
    if (u - start > 1e-6) {
        f.body << "\\draw[" << s.dash << ", " << s.gray << ", thick] ";
        int steps = std::max(2, static_cast<int>((u - start) / 0.03));
        for (int i = 0; i <= steps; i++) {
            auto z = fourier::evaluate(o.terms(), start + (u - start) * i / steps, count);
            f.body << (i ? " -- " : "") << f.at(z.real(), z.imag());
        }
        f.body << ";\n";
    }
    auto centers = fourier::chain(o.terms(), u, count);
    for (size_t i = 1; i < centers.size(); i++) {
        double r = std::abs(o.terms()[i].c);
        const auto& c = centers[i - 1];
        if (r > 0.02) f.body << "\\draw[black!40, very thin] " << f.at(c.real(), c.imag()) << " circle[radius=" << num(r) << "];\n";
        f.body << "\\draw[black!70, thin] " << f.at(c.real(), c.imag()) << " -- " << f.at(centers[i].real(), centers[i].imag())
               << ";\n";
    }
    const auto& tip = centers.back();
    f.body << "\\fill " << f.at(tip.real(), tip.imag()) << " circle[radius=1.5pt]"
           << labelNode(f, {tip.real(), tip.imag(), 0}, 1, 1, nameToLatex(o.name())) << ";\n";
}

void exportComplex(Figure& f, const scene::SceneObject& o, const Style& s) {
    const expr::Value& v = o.value();
    double re = v.re, im = v.type == expr::Type::Complex ? v.im : 0.0;
    std::string name = nameToLatex(o.name());
    if (o.typeId() == "product" && o.propBool("angles", true)) {
        // w = a·b: the angles add up
        const scene::Field* fa = o.field("a");
        const scene::Field* fb = o.field("b");
        if (fa && fb && fa->value.isNumber() && fb->value.isNumber()) {
            double aa = std::atan2(fa->value.type == expr::Type::Complex ? fa->value.im : 0.0, fa->value.re);
            double ab = std::atan2(fb->value.type == expr::Type::Complex ? fb->value.im : 0.0, fb->value.re);
            arc(f, 0.35, 0.0, aa, "black!60, thin");
            arc(f, 0.5, aa, aa + ab, "black!60, thin, densely dotted");
        }
    }
    double r = std::hypot(re, im);
    if (o.propBool("circle") && std::fabs(r - 1.0) > 1e-3) {
        arc(f, r, 0.0, 2 * kPi, std::string(s.gray) + ", very thin, dashed");
        f.extend(r, r);
        f.extend(-r, -r);
    }
    if (o.propBool("projections")) {
        line(f, {re, 0, 0}, {re, im, 0}, std::string(s.gray) + ", thin, dashed");
        line(f, {0, im, 0}, {re, im, 0}, std::string(s.gray) + ", thin, dashed");
    }
    arrow(f, {0, 0, 0}, {re, im, 0}, styleOptions(s, "thick"), name);
}

// The Taylor polynomial: f (dashed gray), T_N (the object's style, in the legend), the center point
void exportTaylor(Figure& f, const scene::TaylorObject& o, const Style& s, const Options& opt) {
    f.plots = true;
    auto curve = [&](const scene::TaylorObject::Samples& pts, const std::string& style) {
        f.body << "\\addplot[" << style << "] coordinates {";
        int k = 0;
        for (size_t i = 0; i < pts.size(); i += 2, k++) {
            double x = pts[i].first, y = pts[i].second;
            bool ok = usable(y) && std::fabs(y) <= 12;
            f.body << (k % 6 == 0 ? "\n  " : " ") << (ok ? f.coord(x, y, 0) : f.coord(NAN, NAN, NAN));
            if (ok) f.extend(x, y, 0);
        }
        f.body << "\n};\n";
    };
    if (o.showFunction()) curve(o.function(), "black!50, densely dashed, thin, forget plot");
    curve(o.polynomial(), plotStyle(s, "thick"));
    legendEntry(f, "T_{" + std::to_string(static_cast<int>(o.series().coeffs.size()) - 1) + "}", opt.legend);
    double a = o.series().center;
    f.body << "\\fill " << f.at(a, o.fCenter(), 0) << " circle[radius=1.5pt]"
           << labelNode(f, {a, o.fCenter(), 0}, 1, -1, "a = " + num(a)) << ";\n";
}

// Newton's method: f, then each step's vertical line and tangent, with the iterates labeled on the axis
void exportNewton(Figure& f, const scene::NewtonObject& o, const Style& s) {
    std::string name = nameToLatex(o.name());
    f.plots = true;
    if (o.showFunction()) {
        f.body << "\\addplot[" << plotStyle(s, "thick, forget plot") << "] coordinates {";
        int k = 0;
        for (size_t i = 0; i < o.function().size(); i += 2, k++) {
            double x = o.function()[i].first, y = o.function()[i].second;
            bool ok = usable(y) && std::fabs(y) <= 12;
            f.body << (k % 6 == 0 ? "\n  " : " ") << (ok ? f.coord(x, y, 0) : f.coord(NAN, NAN, NAN));
            if (ok) f.extend(x, y, 0);
        }
        f.body << "\n};\n";
    }
    const auto& steps = o.run().steps;
    // Iterates bunch up near the root: label only those well apart (and apart from the final estimate)
    double span = o.function().empty() ? 1.0 : o.function().back().first - o.function().front().first;
    double gap = 0.06 * std::fabs(span), lastLabel = NAN;
    for (size_t i = 0; i + 1 < steps.size() && i < 8; i++) {
        const auto& st = steps[i];
        if (!std::isfinite(st.fx)) break;
        line(f, {st.x, 0, 0}, {st.x, st.fx, 0}, "black!45, densely dashed, thin");
        line(f, {st.x, st.fx, 0}, {steps[i + 1].x, 0, 0}, "black, thin");
        f.body << "\\fill " << f.at(st.x, 0, 0) << " circle[radius=1.2pt]";
        bool apart = (std::isnan(lastLabel) || std::fabs(st.x - lastLabel) > gap) && std::fabs(st.x - steps.back().x) > gap;
        if (apart) {
            f.body << labelNode(f, {st.x, 0, 0}, 0, -1, "x_{" + std::to_string(i) + "}");
            lastLabel = st.x;
        }
        f.body << ";\n";
    }
    const auto& last = steps.back();
    f.body << "\\fill " << f.at(last.x, 0, 0) << " circle[radius=1.6pt]"
           << labelNode(f, {last.x, 0, 0}, 1, 1, name + " \\approx " + num(last.x)) << ";\n";
}

// --- Floor tests for the Auto view: true when everything the object draws has z = 0 ---------

bool velocityFlat(const scene::SceneObject& o) {
    auto* r = static_cast<const scene::VelocityObject*>(&o);
    bool planar = std::fabs(r->velocity().z) < 1e-9 && std::fabs(r->acceleration().z) < 1e-9;
    for (const auto& p : r->curve()) planar = planar && std::fabs(p.z) < 1e-9;
    return planar;
}

bool frameFlat(const scene::SceneObject& o) {
    bool planar = true;
    for (const auto& p : static_cast<const scene::FrameObject*>(&o)->curve()) planar = planar && std::fabs(p.z) < 1e-9;
    return planar;
}

// A work integral along a flat path in a flat field
bool workFlat(const scene::SceneObject& o) {
    auto* li = static_cast<const scene::LineIntegralObject*>(&o);
    bool planar = true;
    for (const auto& p : li->path()) planar = planar && std::fabs(p.p.z) < 1e-9;
    for (const auto& a : li->arrows()) planar = planar && std::fabs(a.f.z) < 1e-9;
    return planar;
}

// A turn about z with no basis keeps everything on the floor
bool quaternionFlat(const scene::SceneObject& o) {
    auto* q = static_cast<const scene::QuaternionObject*>(&o);
    expr::Rotation r;
    bool aboutZ = expr::rotationOf(o.value(), r) && (!r.hasAxis || std::fabs(std::fabs(r.axis.z) - 1.0f) < 1e-6f);
    const scene::Field* vf = q->field("v");
    bool vFlat = !q->actsOnVector() || (vf && vf->value.type == expr::Type::Vector && isZ0(vf->value.vector));
    return aboutZ && !q->showBasis() && vFlat;
}

bool vectorFlat(const scene::SceneObject& o) {
    return isZ0(o.value().vector) && isZ0(static_cast<const scene::VectorObject*>(&o)->tail());
}

bool matrixFlat(const scene::SceneObject& o) {
    const Matrix4D& m = o.value().matrix;
    return std::fabs(m.m[2][0]) < 1e-9f && std::fabs(m.m[2][1]) < 1e-9f && std::fabs(m.m[0][2]) < 1e-9f &&
           std::fabs(m.m[1][2]) < 1e-9f && (o.value().size == 3 || std::fabs(m.m[2][3]) < 1e-9f);
}

bool alwaysFlat(const scene::SceneObject&) { return true; }

} // namespace

const ObjectExport* detail::objectExport(const std::string& typeId) {
    using O = const scene::SceneObject&;
    using F = Figure&;
    using I = const ExportInputs&;
    // Lambdas without captures convert to the plain function pointers ObjectExport holds
    static const std::map<std::string, ObjectExport> table = {
        {"vector", {[](F f, O o, const Style& s, I) { exportVector(f, o, s); }, vectorFlat}},
        {"matrix", {[](F f, O o, const Style& s, I) { exportMatrix(f, o, s); }, matrixFlat}},
        {"eigen", {[](F f, O o, const Style& s, I) { exportEigen(f, o, s); }, nullptr}},
        {"quaternion", {[](F f, O o, const Style& s, I) {
             exportQuaternion(f, static_cast<const scene::QuaternionObject&>(o), s);
         }, quaternionFlat}},
        {"function", {[](F f, O o, const Style& s, I in) {
             exportPlot(f, static_cast<const scene::PlotObject&>(o), s, in.space, in.options);
         }, alwaysFlat}},
        {"implicit", {[](F f, O o, const Style& s, I in) {
             exportPlot(f, static_cast<const scene::PlotObject&>(o), s, in.space, in.options);
         }, alwaysFlat}},
        {"curve", {[](F f, O o, const Style& s, I in) {
             exportPlot(f, static_cast<const scene::PlotObject&>(o), s, in.space, in.options);
         }, nullptr}},
        {"surface", {[](F f, O o, const Style& s, I in) {
             exportPlot(f, static_cast<const scene::PlotObject&>(o), s, in.space, in.options);
         }, nullptr}},
        {"field", {[](F f, O o, const Style& s, I in) {
             exportField(f, static_cast<const scene::VectorFieldObject&>(o), s, in.space, in.options);
         }, [](O o) { return !static_cast<const scene::VectorFieldObject&>(o).volume(); }}},  // plane fields
        {"fourierseries", {[](F f, O o, const Style& s, I in) {
             exportFourierSeries(f, static_cast<const scene::FourierSeriesObject&>(o), s, in.options);
         }, alwaysFlat}},
        {"frame", {[](F f, O o, const Style& s, I) { exportFrame(f, static_cast<const scene::FrameObject&>(o), s); },
                   frameFlat}},
        {"tangent", {[](F f, O o, const Style& s, I) {
             exportTangent(f, static_cast<const scene::TangentObject&>(o), s);
         }, alwaysFlat}},
        {"tangentplane", {[](F f, O o, const Style& s, I) {
             exportTangentPlane(f, static_cast<const scene::TangentPlaneObject&>(o), s);
         }, nullptr}},
        {"velocity", {[](F f, O o, const Style& s, I) {
             exportVelocity(f, static_cast<const scene::VelocityObject&>(o), s);
         }, velocityFlat}},
        {"doubleintegral", {[](F f, O o, const Style& s, I) {
             exportDoubleIntegral(f, static_cast<const scene::DoubleIntegralObject&>(o), s);
         }, nullptr}},
        {"lineintegral", {[](F f, O o, const Style& s, I) {
             exportLineIntegral(f, static_cast<const scene::LineIntegralObject&>(o), s);
         }, workFlat}},
        {"taylor", {[](F f, O o, const Style& s, I in) {
             exportTaylor(f, static_cast<const scene::TaylorObject&>(o), s, in.options);
         }, alwaysFlat}},
        {"newton", {[](F f, O o, const Style& s, I) { exportNewton(f, static_cast<const scene::NewtonObject&>(o), s); },
                    alwaysFlat}},
        // The complex plane is always a 2D figure
        {"complex", {[](F f, O o, const Style& s, I) { exportComplex(f, o, s); }, alwaysFlat}},
        {"product", {[](F f, O o, const Style& s, I) { exportComplex(f, o, s); }, alwaysFlat}},
        {"epicycles", {[](F f, O o, const Style& s, I) {
             exportEpicycles(f, static_cast<const scene::EpicyclesObject&>(o), s);
         }, alwaysFlat}},
    };
    auto it = table.find(typeId);
    return it == table.end() ? nullptr : &it->second;
}

} // namespace linalg::latex
