#include "scene/Summaries.hpp"
#include "scene/Types.hpp"
#include <cmath>
#include <cstdio>
#include <map>

namespace linalg::scene {

namespace {

std::string fmt(double v) {
    char buf[32];
    if (std::fabs(v) < 5e-5) v = 0;
    snprintf(buf, sizeof(buf), "%.4g", v);
    return buf;
}

using Summary = std::string (*)(const SceneObject&);

// Types without an entry show their value's summary
const std::map<std::string, Summary>& summaries() {
    using O = const SceneObject&;  // a short name for the parameter type below
    // Lambdas with no capture ([]) convert to plain function pointers, so they fit Summary
    static const std::map<std::string, Summary> table = {
        {"vector", [](O o) -> std::string {
             if (!static_cast<const scene::VectorObject&>(o).byFormula()) return o.value().summary();
             return o.field("value")->text + " = " + o.value().summary();
         }},
        {"matrix", [](O o) -> std::string {
             return static_cast<const scene::MatrixObject&>(o).byFormula() ? o.field("value")->text : o.value().summary();
         }},
        {"eigen", [](O o) -> std::string { return "eigenvectors of " + o.field("of")->text; }},
        {"function", [](O o) -> std::string { return "y = " + o.field("f")->text; }},
        {"surface", [](O o) -> std::string { return "z = " + o.field("f")->text; }},
        {"implicit", [](O o) -> std::string { return o.field("f")->text + " = 0"; }},
        {"curve", [](O) -> std::string { return "curve in u"; }},
        {"field", [](O o) -> std::string { return "F = " + o.field("F")->text; }},
        {"parameter", [](O o) -> std::string {
             const auto& p = static_cast<const scene::ParameterObject&>(o);
             const char* how = p.mode() == scene::ParameterObject::Mode::Formula   ? "f(t)"
                               : p.mode() == scene::ParameterObject::Mode::Animate ? "animated"
                                                                                  : "slider";
             return std::string(how) + " = " + fmt(p.number());
         }},
        {"quaternion", [](O o) -> std::string {
             expr::Rotation r;
             if (!expr::rotationOf(o.value(), r)) return o.value().summary();
             char buf[64];
             snprintf(buf, sizeof(buf), "%.0f° rotation", r.angle * 180.0 / 3.14159265358979);
             return buf;
         }},
        {"doubleintegral", [](O o) -> std::string { return "= " + o.value().summary(); }},
        {"lineintegral", [](O o) -> std::string { return "= " + o.value().summary(); }},
        {"frame", [](O o) -> std::string {
             const auto& f = static_cast<const scene::FrameObject&>(o);
             char buf[64];
             snprintf(buf, sizeof(buf), "κ = %.3g at u = %.3g", f.frame().kappa, f.u0());
             return buf;
         }},
        {"tangent", [](O o) -> std::string {
             char buf[64];
             snprintf(buf, sizeof(buf), "slope %.3g at x = %.3g", o.value().re,
                      static_cast<const scene::TangentObject&>(o).x0());
             return buf;
         }},
        {"tangentplane", [](O o) -> std::string {
             const auto& g = static_cast<const scene::TangentPlaneObject&>(o);
             char buf[64];
             snprintf(buf, sizeof(buf), "grad f = (%.3g, %.3g)", g.fx(), g.fy());
             return buf;
         }},
        {"velocity", [](O o) -> std::string {
             char buf[64];
             snprintf(buf, sizeof(buf), "speed %.3g at u = %.3g", o.value().vector.magnitude(),
                      static_cast<const scene::VelocityObject&>(o).u0());
             return buf;
         }},
        {"taylor", [](O o) -> std::string {
             const auto& tp = static_cast<const scene::TaylorObject&>(o);
             char buf[64];
             snprintf(buf, sizeof(buf), "degree %d about %.3g", static_cast<int>(tp.series().coeffs.size()) - 1,
                      tp.series().center);
             return buf;
         }},
        {"newton", [](O o) -> std::string {
             const auto& nw = static_cast<const scene::NewtonObject&>(o);
             char buf[64];
             snprintf(buf, sizeof(buf), "x%d ≈ %.6g", static_cast<int>(nw.run().steps.size()) - 1, o.value().re);
             return buf;
         }},
        {"fourierseries", [](O o) -> std::string {
             return "N = " + std::to_string(static_cast<const scene::FourierSeriesObject&>(o).terms()) + " terms";
         }},
        {"product", [](O o) -> std::string {
             return o.field("a")->text + " · " + o.field("b")->text + " = " + o.value().summary();
         }},
        {"epicycles", [](O o) -> std::string {
             return std::to_string(static_cast<const scene::EpicyclesObject&>(o).circles()) + " circles";
         }},
    };
    return table;
}

} // namespace

std::string summaryOf(const SceneObject& o) {
    if (!o.error().empty()) return o.error();
    auto it = summaries().find(o.typeId());
    return it != summaries().end() ? it->second(o) : o.value().summary();
}

} // namespace linalg::scene
