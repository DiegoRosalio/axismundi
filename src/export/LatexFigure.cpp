// The LaTeX export's building blocks (see LatexFigure.hpp)
#include "export/LatexFigure.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>

namespace linalg::latex::detail {

std::string num(double v) {
    if (!std::isfinite(v)) return "nan";
    if (std::fabs(v) < 5e-5) v = 0.0;
    char buf[32];
    snprintf(buf, sizeof(buf), "%.4g", v);
    return buf;
}

std::string styleOptions(const Style& s, const char* extra) {
    std::string o = std::string(s.dash) + ", " + s.gray;
    if (extra) o += std::string(", ") + extra;
    return o;
}

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

// Complex plane extras: arcs for angles, dashed projections, circle through z
void arc(Figure& f, double radius, double from, double to, const std::string& options) {
    if (std::fabs(to - from) < 1e-6) return;
    int samples = std::clamp(static_cast<int>(std::fabs(to - from) * 180.0 / kPi / 4.0), 12, 90);
    f.body << "\\addplot[" << options << ", domain=" << num(from * 180.0 / kPi) << ":" << num(to * 180.0 / kPi)
           << ", samples=" << samples << ", forget plot] ({" << num(radius) << "*cos(x)}, {" << num(radius)
           << "*sin(x)});\n";
}

} // namespace linalg::latex::detail
