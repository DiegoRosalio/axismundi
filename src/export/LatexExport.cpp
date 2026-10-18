#include "export/LatexExport.hpp"
#include "math/Contour.hpp"
#include "math/Decompositions.hpp"
#include "math/Expression.hpp"
#include "math/VectorField.hpp"
#include "scene/Types.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <set>
#include <sstream>

namespace linalg::latex {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kOffChart = 50.0;  // plot values beyond this are left out (asymptotes)

std::string num(double v) {
    if (!std::isfinite(v)) return "nan";
    if (std::fabs(v) < 5e-5) v = 0.0;
    char buf[32];
    snprintf(buf, sizeof(buf), "%.4g", v);
    return buf;
}

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

std::string styleOptions(const Style& s, const char* extra = nullptr) {
    std::string o = std::string(s.dash) + ", " + s.gray;
    if (extra) o += std::string(", ") + extra;
    return o;
}

const char* kArrow = "-{Stealth[length=5pt, width=3.5pt]}";

// Label placement so the text sits beyond the tip (2D), above right in 3D
std::string anchorFor(double dx, double dy, bool flat) {
    if (!flat) return "anchor=south west";
    double a = std::atan2(dy, dx) * 180.0 / kPi;
    if (a < 0) a += 360.0;
    const char* names[8] = {"west", "south west", "south", "south east", "east", "north east", "north", "north west"};
    int k = static_cast<int>(std::lround(a / 45.0)) % 8;
    return std::string("anchor=") + names[k];
}

// A label node at p; labels that land on the same point are stacked, one line apart
std::string labelNode(Figure& f, const Point3& p, double dx, double dy, const std::string& label) {
    int same = 0;
    for (const Point3& q : f.labelPoints)
        if (std::fabs(q.x - p.x) + std::fabs(q.y - p.y) + std::fabs(q.z - p.z) < 1e-3) same++;
    f.labelPoints.push_back(p);
    std::string n = " node[" + anchorFor(dx, dy, f.flat) + ", inner sep=2pt";
    if (same) n += ", yshift=" + std::to_string(-11 * same) + "pt";
    return n + "] {$" + label + "$}";
}

void arrow(Figure& f, const Point3& from, const Point3& to, const std::string& options, const std::string& label) {
    f.body << "\\draw[" << kArrow << ", " << options << "] " << f.at(from.x, from.y, from.z) << " -- "
           << f.at(to.x, to.y, to.z);
    if (!label.empty()) f.body << labelNode(f, to, to.x - from.x, to.y - from.y, label);
    f.body << ";\n";
}

void line(Figure& f, const Point3& a, const Point3& b, const std::string& options) {
    f.body << "\\draw[" << options << "] " << f.at(a.x, a.y, a.z) << " -- " << f.at(b.x, b.y, b.z) << ";\n";
}

bool isZ0(const Vector3D& v) {
    return std::fabs(v.z) < 1e-9f;
}

bool isGreek(const std::string& s) {
    static const std::set<std::string> greek = {
        "alpha", "beta", "gamma", "delta", "epsilon", "varepsilon", "zeta", "eta", "theta", "vartheta", "iota",
        "kappa", "lambda", "mu", "nu", "xi", "rho", "sigma", "tau", "upsilon", "phi", "varphi", "chi", "psi",
        "omega", "Gamma", "Delta", "Theta", "Lambda", "Xi", "Pi", "Sigma", "Upsilon", "Phi", "Psi", "Omega"};
    return greek.count(s) > 0;
}

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

bool usable(double v) {
    return std::isfinite(v) && std::fabs(v) <= kOffChart;
}

std::string plotStyle(const Style& s, const char* extra) {
    return std::string(s.dash) + ", " + s.gray + ", " + extra;
}

void legendEntry(Figure& f, const std::string& text, bool legend) {
    if (!legend) {
        return;
    }
    f.body << "\\addlegendentry{$" << text << "$}\n";
    f.legend = true;
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


// Complex plane extras: arcs for angles, dashed projections, circle through z
void arc(Figure& f, double radius, double from, double to, const std::string& options) {
    if (std::fabs(to - from) < 1e-6) return;
    int samples = std::clamp(static_cast<int>(std::fabs(to - from) * 180.0 / kPi / 4.0), 12, 90);
    f.body << "\\addplot[" << options << ", domain=" << num(from * 180.0 / kPi) << ":" << num(to * 180.0 / kPi)
           << ", samples=" << samples << ", forget plot] ({" << num(radius) << "*cos(x)}, {" << num(radius)
           << "*sin(x)});\n";
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

} // namespace

// --- Public helpers -------------------------------------------------------------------

Style styleFor(int index) {
    static const char* dashes[5] = {"solid", "dashed", "densely dotted", "dashdotted", "loosely dashed"};
    static const char* grays[3] = {"black", "black!65", "black!45"};
    static const float levels[3] = {0.0f, 0.35f, 0.55f};
    if (index < 0) index = 0;
    int d = index % 5, g = (index / 5) % 3;
    return {dashes[d], grays[g], d, levels[g]};
}

bool drawable(const scene::SceneObject& o) {
    const std::string& type = o.typeId();
    if (type == "parameter") return false;
    if (!o.error().empty()) return false;
    return true;
}

std::vector<const scene::SceneObject*> exportedObjects(const scene::Space& space, const scene::Selection& sel,
                                                       Scope scope) {
    std::vector<const scene::SceneObject*> out;
    for (const auto& o : space.objects()) {
        if (!drawable(*o)) continue;
        if (scope == Scope::Selection ? !sel.contains(o->id()) : !o->visible) continue;
        out.push_back(o.get());
    }
    return out;
}

int styleIndexOf(const scene::Space& space, const scene::Selection& sel, Scope scope, scene::ObjectId id) {
    auto list = exportedObjects(space, sel, scope);
    for (size_t k = 0; k < list.size(); k++)
        if (list[k]->id() == id) return static_cast<int>(k);
    return -1;
}

void viewFromDirection(const Vector3D& d, double& azimuth, double& elevation) {
    double x = d.x, y = d.y, z = d.z;  // in double precision
    double len = std::sqrt(x * x + y * y + z * z);
    if (len < 1e-9) {
        azimuth = 25.0;
        elevation = 30.0;
        return;
    }
    elevation = std::asin(std::clamp(z / len, -1.0, 1.0)) * 180.0 / kPi;
    azimuth = std::atan2(x, -y) * 180.0 / kPi;
}

std::string nameToLatex(const std::string& name) {
    std::string base = name, sub;
    size_t us = name.find('_');
    if (us != std::string::npos) {
        base = name.substr(0, us);
        sub = name.substr(us + 1);
    } else {
        size_t k = name.size();
        while (k > 1 && std::isdigit(static_cast<unsigned char>(name[k - 1]))) k--;
        if (k < name.size()) {
            base = name.substr(0, k);
            sub = name.substr(k);
        }
    }
    auto word = [](const std::string& w) {
        if (isGreek(w)) return "\\" + w;
        if (w.size() > 1) return "\\mathit{" + w + "}";
        return w;
    };
    std::string out = word(base);
    if (!sub.empty()) out += "_{" + (isGreek(sub) ? "\\" + sub : sub) + "}";
    return out;
}

// --- Export ------------------------------------------------------------------------

Result exportLatex(const scene::Space& space, const scene::Selection& sel, const Options& opt) {
    Result result;
    std::vector<const scene::SceneObject*> objects;
    for (const auto& o : space.objects()) {
        bool inScope = opt.scope == Scope::Selection ? sel.contains(o->id()) : o->visible;
        if (!inScope) continue;
        if (o->typeId() == "parameter") {
            result.skipped.push_back(o->name() + " (parameter: a number, not drawn)");
            continue;
        }
        if (!o->error().empty()) {
            result.skipped.push_back(o->name() + " (has an error)");
            continue;
        }
        objects.push_back(o.get());
    }

    // 2D or 3D
    bool complexPlane = space.kind() == scene::SpaceKind::Complex;
    bool flat = complexPlane;
    if (!complexPlane) {
        if (opt.view == View::Flat) flat = true;
        else if (opt.view == View::Space) flat = false;
        else {
            flat = !objects.empty();
            for (const auto* o : objects) {
                const std::string& t = o->typeId();
                if (t == "function" || t == "implicit" || t == "fourierseries") continue;
                // Plane fields live on the floor (only F's x and y are used)
                if (t == "field" && !static_cast<const scene::VectorFieldObject*>(o)->volume()) continue;
                // A frame on a flat curve
                if (t == "frame") {
                    bool planar = true;
                    for (const auto& p : static_cast<const scene::FrameObject*>(o)->curve()) planar = planar && std::fabs(p.z) < 1e-9;
                    if (planar) continue;
                }
                // A work integral along a flat path in a flat field
                if (t == "lineintegral") {
                    auto* li = static_cast<const scene::LineIntegralObject*>(o);
                    bool planar = true;
                    for (const auto& p : li->path()) planar = planar && std::fabs(p.p.z) < 1e-9;
                    for (const auto& a : li->arrows()) planar = planar && std::fabs(a.f.z) < 1e-9;
                    if (planar) continue;
                }
                // A turn about z with no basis keeps everything on the floor
                if (t == "quaternion") {
                    auto* q = static_cast<const scene::QuaternionObject*>(o);
                    expr::Rotation r;
                    bool aboutZ = expr::rotationOf(o->value(), r) && (!r.hasAxis || std::fabs(std::fabs(r.axis.z) - 1.0f) < 1e-6f);
                    const scene::Field* vf = q->field("v");
                    bool vFlat = !q->actsOnVector() || (vf && vf->value.type == expr::Type::Vector && isZ0(vf->value.vector));
                    if (aboutZ && !q->showBasis() && vFlat) continue;
                }
                if (t == "vector") {
                    auto* v = static_cast<const scene::VectorObject*>(o);
                    if (isZ0(o->value().vector) && isZ0(v->tail())) continue;
                }
                if (t == "matrix") {
                    const Matrix4D& m = o->value().matrix;
                    bool planar = std::fabs(m.m[2][0]) < 1e-9f && std::fabs(m.m[2][1]) < 1e-9f &&
                                  std::fabs(m.m[0][2]) < 1e-9f && std::fabs(m.m[1][2]) < 1e-9f &&
                                  (o->value().size == 3 || std::fabs(m.m[2][3]) < 1e-9f);
                    if (planar) continue;
                }
                flat = false;
                break;
            }
        }
    }
    result.flat = flat;

    Figure f;
    f.flat = flat;
    for (size_t k = 0; k < objects.size(); k++) {
        const scene::SceneObject& o = *objects[k];
        Style s = styleFor(static_cast<int>(k));
        const std::string& t = o.typeId();
        f.body << "% " << o.name() << "\n";
        if (complexPlane && t == "epicycles") {
            exportEpicycles(f, static_cast<const scene::EpicyclesObject&>(o), s);
        } else if (complexPlane) {
            exportComplex(f, o, s);
        } else if (t == "vector") {
            exportVector(f, o, s);
        } else if (t == "matrix") {
            exportMatrix(f, o, s);
        } else if (t == "eigen") {
            exportEigen(f, o, s);
        } else if (auto* p = dynamic_cast<const scene::PlotObject*>(&o)) {
            exportPlot(f, *p, s, space, opt);
        } else if (auto* fsr = dynamic_cast<const scene::FourierSeriesObject*>(&o)) {
            exportFourierSeries(f, *fsr, s, opt);
        } else if (auto* fo = dynamic_cast<const scene::FrameObject*>(&o)) {
            exportFrame(f, *fo, s);
        } else if (auto* di = dynamic_cast<const scene::DoubleIntegralObject*>(&o)) {
            exportDoubleIntegral(f, *di, s);
        } else if (auto* li = dynamic_cast<const scene::LineIntegralObject*>(&o)) {
            exportLineIntegral(f, *li, s);
        } else if (auto* qo = dynamic_cast<const scene::QuaternionObject*>(&o)) {
            exportQuaternion(f, *qo, s);
        } else if (auto* vf = dynamic_cast<const scene::VectorFieldObject*>(&o)) {
            exportField(f, *vf, s, space, opt);
        } else {
            result.skipped.push_back(o.name() + " (not supported yet)");
            continue;
        }
        result.exported++;
    }
    if (complexPlane) {
        f.body << "% unit circle\n";
        arc(f, 1.0, 0.0, 2 * kPi, "black!35, very thin");
        f.extend(1, 1);
        f.extend(-1, -1);
    }

    // Axis limits: what was drawn, plus a margin, never smaller than the unit box
    double lo[3], hi[3];
    for (int k = 0; k < 3; k++) {
        lo[k] = std::min(f.lo[k], k == 2 && flat ? 0.0 : -0.5);
        hi[k] = std::max(f.hi[k], 1.0);
        double pad = 0.12 * (hi[k] - lo[k]);
        lo[k] -= pad;
        hi[k] += pad;
    }
    bool equal = !f.plots;

    std::ostringstream out;
    if (opt.standalone) {
        out << "\\documentclass[border=4pt]{standalone}\n"
               "\\usepackage{pgfplots}\n"
               "\\pgfplotsset{compat=1.18}\n"
               "\\usetikzlibrary{arrows.meta}\n"
               "\\begin{document}\n";
    } else {
        out << "% Exported from axismundi. Needs in the preamble:\n"
               "%   \\usepackage{pgfplots}\n"
               "%   \\pgfplotsset{compat=1.18}\n"
               "%   \\usetikzlibrary{arrows.meta}\n";
    }
    out << "\\begin{tikzpicture}\n\\begin{axis}[\n";
    // In a document the width follows the text; a standalone figure has no text width, so use 14 cm
    double w = std::clamp(opt.width, 0.1, 1.0);
    // Height in proportion: without it PGFPlots keeps a small default height, and equal-aspect or
    // 3D figures shrink to fit it
    bool equalAspect = !f.plots;
    double hRatio = equalAspect ? (flat ? 1.0 : 0.85) : 0.7;
    // PGFPlots sizes a 3D axis by its whole bounding box. With axes through the origin only the
    // axes are drawn, so the picture would come out smaller: enlarge by the ratio of the box's
    // projected size to the axes' projected size for this view.
    double fx = 1.0, fy = 1.0;
    if (!flat && !f.surfaces && opt.axes) {
        double az = opt.azimuth * kPi / 180.0, el = opt.elevation * kPi / 180.0;
        auto extent = [&](const std::vector<Point3>& pts, double& w2, double& h2) {
            double x0 = 1e300, x1 = -1e300, y0 = 1e300, y1 = -1e300;
            for (const Point3& p : pts) {
                double sx = p.x * std::cos(az) + p.y * std::sin(az);
                double sy = (-p.x * std::sin(az) + p.y * std::cos(az)) * std::sin(el) + p.z * std::cos(el);
                x0 = std::min(x0, sx), x1 = std::max(x1, sx), y0 = std::min(y0, sy), y1 = std::max(y1, sy);
            }
            w2 = x1 - x0;
            h2 = y1 - y0;
        };
        std::vector<Point3> box, axes;
        for (int a = 0; a < 2; a++)
            for (int b = 0; b < 2; b++)
                for (int c = 0; c < 2; c++) box.push_back({a ? hi[0] : lo[0], b ? hi[1] : lo[1], c ? hi[2] : lo[2]});
        axes = {{lo[0], 0, 0}, {hi[0], 0, 0}, {0, lo[1], 0}, {0, hi[1], 0}, {0, 0, lo[2]}, {0, 0, hi[2]}};
        double bw, bh, aw, ah;
        extent(box, bw, bh);
        extent(axes, aw, ah);
        if (aw > 1e-9) fx = std::clamp(bw / aw, 1.0, 2.5);
        if (ah > 1e-9) fy = std::clamp(bh / ah, 1.0, 2.5);
    }
    if (fx > 1.0 || fy > 1.0)
        out << "  % larger than the drawing: PGFPlots sizes a 3D axis by its hidden bounding box;\n"
               "  % the drawing itself is about " << num(w) << (opt.standalone ? " x 14cm" : "\\linewidth") << " wide\n";
    if (opt.standalone) {
        out << "  width=" << num(14.0 * w * fx) << "cm, height=" << num(14.0 * w * hRatio * fy) << "cm,\n";
    } else {
        out << "  width=" << num(w * fx) << "\\linewidth, height=" << num(w * hRatio * fy) << "\\linewidth,\n";
    }
    if (!flat) out << "  view={" << num(opt.azimuth) << "}{" << num(opt.elevation) << "},\n";
    if (opt.axes) {
        // Surfaces read better with the axes along the edges, as in textbooks
        out << (f.surfaces ? "  axis lines=left,\n" : "  axis lines=center,\n")
            << "  axis line style={thin, -{Stealth[length=4pt, width=3pt]}},\n"
               "  xlabel={$x$}, ylabel={$y$},"
            << (flat ? "" : " zlabel={$z$},") << "\n"
            << "  ticklabel style={font=\\footnotesize, black!70},\n"
               "  every tick/.style={black!70, very thin},\n";
        if (complexPlane) out << "  xlabel={$\\mathrm{Re}$}, ylabel={$\\mathrm{Im}$},\n";
    } else {
        out << "  hide axis,\n";
    }
    if (opt.grid) out << "  grid=major, grid style={black!12, very thin},\n";
    out << "  xmin=" << num(lo[0]) << ", xmax=" << num(hi[0]) << ", ymin=" << num(lo[1]) << ", ymax=" << num(hi[1]);
    if (!flat) out << ", zmin=" << num(lo[2]) << ", zmax=" << num(hi[2]);
    out << ",\n";
    if (equal && flat) out << "  axis equal image,\n";
    // 3D axes are otherwise stretched to fill the box (a flat field box would look tall)
    if (!flat && f.fields3D) out << "  unit vector ratio=1 1 1,\n";
    out << "  unbounded coords=jump,\n"
           "  clip=false,\n";
    if (f.legend) out << "  legend style={draw=none, fill=none, font=\\footnotesize}, legend cell align=left,\n"
                         "  legend pos=outer north east,\n";
    out << "]\n" << f.body.str() << "\\end{axis}\n\\end{tikzpicture}\n";
    if (opt.standalone) out << "\\end{document}\n";
    result.code = out.str();
    return result;
}

} // namespace linalg::latex
