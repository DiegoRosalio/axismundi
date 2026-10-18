// The LaTeX export: which objects go in, 2D or 3D, the axis (limits, size, view) around the drawing, and
// the public helpers. Each object type's drawing commands are in LatexObjects.cpp.
#include "export/LatexExport.hpp"
#include "export/LatexFigure.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace linalg::latex {

using namespace detail;

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
                // 2D only when every object lies on the floor (each type has its own test)
                const ObjectExport* ex = objectExport(o->typeId());
                if (ex && ex->flat && ex->flat(*o)) continue;
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
        f.body << "% " << o.name() << "\n";
        const ObjectExport* ex = objectExport(o.typeId());
        if (!ex || !ex->write) {
            result.skipped.push_back(o.name() + " (not supported yet)");
            continue;
        }
        ex->write(f, o, s, ExportInputs{space, opt});
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
