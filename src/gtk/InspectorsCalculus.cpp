// The GTK inspectors of the calculus types: integrals, the Frenet frame, tangents, the tangent plane, velocity,
// Taylor polynomials, Newton's method. They mirror the raylib app's ScenePanelInspectors.cpp.
#include "gtk/Inspectors.hpp"
#include "math/CurveGeometry.hpp"
#include "scene/Summaries.hpp"
#include "scene/Types.hpp"
#include <algorithm>

namespace linalg::gtk::inspectors {

namespace {

template <class T>
const T& as(const Object& o) {
    return static_cast<const T&>(o);
}

} // namespace

void doubleIntegral(InspectorBuilder& b) {
    using D = scene::DoubleIntegralObject;
    b.group("", "More boxes get closer to the exact volume. Volume below the floor counts negative. Use a parameter "
                "in n (e.g. round(2 + 18*s)) to watch the boxes refine.");
    b.formula("f");
    b.anchor("integral.f");
    b.range("xmin", "xmax", "x");
    b.range("ymin", "ymax", "y");
    b.formula("n");
    b.anchor("integral.n");
    b.choice({"Left", "Middle", "Right"}, [](const Object& o) { return static_cast<int>(as<D>(o).rule()); },
             {"Each box takes the height of f at its lower-left corner.",
              "Each box takes the height of f at its center (usually the most accurate).",
              "Each box takes the height of f at its upper-right corner."},
             [](Object& o, int rule) { o.setProp("rule", rule == 0 ? "left" : rule == 2 ? "right" : "middle"); });
    b.group();
    b.anchor("integral.rule");
    b.toggle("Boxes", "boxes", true, "The Riemann boxes under the surface");
    b.toggle("Surface", "surface", true, "The surface z = f(x, y)");
    b.value("Boxes", [](const Object& o) {
        const D& d = as<D>(o);
        if (!d.error().empty()) return std::string();
        int r = static_cast<int>(d.rule());
        int n = d.boxesPerSide();
        return format("%d × %d, %s: %.4f", n, n, r == 0 ? "left" : r == 2 ? "right" : "middle", d.riemann().sum);
    });
    b.value("Volume ∫∫ f dA", [](const Object& o) {
        return o.error().empty() ? format("≈ %.4f", as<D>(o).exact()) : std::string();
    });
    b.value("Error of the Boxes", [](const Object& o) {
        const D& d = as<D>(o);
        return d.error().empty() ? format("%.4f", d.riemann().sum - d.exact()) : std::string();
    });
    b.line([](const Object& o) {
        const D& d = as<D>(o);
        return d.error().empty() && d.riemann().skipped > 0
                   ? format("%d boxes left out: f is undefined there.", d.riemann().skipped)
                   : std::string();
    }, InspectorBuilder::Tone::Warning);
}

void lineIntegral(InspectorBuilder& b) {
    using L = scene::LineIntegralObject;
    b.group("", "Green where F pushes along the motion, red where it pushes against it. Up to: 0 to 1, e.g. a "
                "parameter or mod(t/4, 1) to travel the path.");
    b.formula("F");
    b.anchor("integral.F");
    for (const char* k : {"x", "y", "z"}) b.formula(k);
    b.range("umin", "umax", "u");
    b.formula("upto");
    b.anchor("integral.upto");
    b.value("Work So Far", [](const Object& o) {
        const L& li = as<L>(o);
        return li.error().empty() ? format("%.4f   (%.0f%% of the path)", li.value().re, li.upto() * 100) : std::string();
    });
    b.value("Whole Path", [](const Object& o) {
        return o.error().empty() ? format("%.4f", as<L>(o).total()) : std::string();
    });
}

void frame(InspectorBuilder& b) {
    using F = scene::FrameObject;
    b.group("", "κ: how sharply it turns (1/radius). τ: how fast it twists out of its plane (0 for flat curves). Use t "
                "in \"At u\" to ride along.");
    b.formula("x");
    b.anchor("frame.curve");
    b.formula("y");
    b.formula("z");
    b.range("umin", "umax", "u");
    b.formula("u0");
    b.anchor("frame.u0");
    b.group("Show");
    b.toggle("Frame T, N, B", "frame", true,
             "Tangent T (direction of motion), normal N (towards the center of the turn), binormal B = T × N.");
    b.toggle("Osculating Circle", "circle", true,
             "The circle that hugs the curve best at the point: radius 1/κ, in the plane of T and N.");
    b.toggle("Curvature Comb", "comb", false,
             "Teeth along the curve, as long as the curvature there: where it bends most, they are longest.");
    b.anchor("frame.comb");
    b.toggle("Curve", "curve", true, "Draw the curve itself.");
    b.value("Curvature κ", [](const Object& o) {
        if (!o.error().empty()) return std::string();
        const geometry::Frame& f = as<F>(o).frame();
        return f.kappa > 1e-9 ? format("%.4f   (radius %.4g)", f.kappa, 1.0 / f.kappa)
                              : format("%.4f   (straight)", f.kappa);
    });
    b.value("Torsion τ", [](const Object& o) {
        if (!o.error().empty()) return std::string();
        const geometry::Frame& f = as<F>(o).frame();
        return format("%.4f   speed |r'| = %.3f", std::fabs(f.tau) < 5e-5 ? 0.0 : f.tau, f.speed);
    });
}

void tangent(InspectorBuilder& b) {
    using T = scene::TangentObject;
    b.group("", "The slope f' is the limit of secant slopes as h → 0. Put t or a slider in \"At x\" to slide the point.");
    b.formula("f");
    b.formula("x0");
    b.formula("h");
    b.anchor("tangent.h");
    b.range("xmin", "xmax", "x");
    b.group("Show");
    b.toggle("Graph of f", "function", true, "Draw y = f(x) itself.");
    b.toggle("Graph of f'", "derivative", true,
             "Draw the slope of f at every x as a second curve: where f climbs it is above 0, where f has a peak or a "
             "valley it crosses 0.");
    b.anchor("tangent.derivative");
    b.toggle("Slope Triangle", "triangle", true,
             "One step right along the tangent climbs f'(x0): the slope read off the picture.");
    b.value("Slope", [](const Object& o) {
        const T& tg = as<T>(o);
        return tg.error().empty() ? format("f'(%.4g) = %.4f", tg.x0(), tg.slope()) : std::string();
    });
    b.value("Tangent Line", [](const Object& o) {
        const T& tg = as<T>(o);
        if (!tg.error().empty()) return std::string();
        return format("y = %.4g %s %.4g (x %s %.4g)", tg.y0(), tg.slope() < 0 ? "−" : "+", std::fabs(tg.slope()),
                      tg.x0() < 0 ? "+" : "−", std::fabs(tg.x0()));
    });
    b.value("Secant", [](const Object& o) {
        const T& tg = as<T>(o);
        if (!tg.error().empty() || !tg.hasSecant()) return std::string();
        return format("slope %.4f (h = %.3g): off by %.2g", tg.secantSlope(), tg.secantH(),
                      std::fabs(tg.secantSlope() - tg.slope()));
    });
    b.value("Bending", [](const Object& o) {
        const T& tg = as<T>(o);
        if (!tg.error().empty() || !tg.hasSecond()) return std::string();
        double f2 = tg.second();
        const char* shape = std::fabs(f2) < 1e-4 ? "a straight stretch or an inflection point"
                            : f2 > 0             ? "f bends upward (like a cup) here"
                                                 : "f bends downward (like a cap) here";
        return format("f''(x0) = %.3f: %s", std::fabs(f2) < 1e-4 ? 0.0 : f2, shape);
    });
}

void tangentPlane(InspectorBuilder& b) {
    using G = scene::TangentPlaneObject;
    b.group("", "The gradient is the value: other objects can use it by name, e.g. a vector whose formula is the name "
                "of this object.");
    b.formula("f");
    b.formula("x0");
    b.anchor("tplane.point");
    b.formula("y0");
    b.range("xmin", "xmax", "x");
    b.range("ymin", "ymax", "y");
    b.group("Show");
    b.toggle("Surface", "surface", true, "Draw z = f(x, y) as a wire mesh.");
    b.toggle("Tangent Plane", "plane", true,
             "The flat plane that touches the surface at the point: near it, f looks like this plane.");
    b.toggle("Slopes Along x and y", "slopes", true,
             "One step in x climbs ∂f/∂x (red), one step in y climbs ∂f/∂y (green): the partial derivatives.");
    b.toggle("Gradient", "gradient", true,
             "The arrow (∂f/∂x, ∂f/∂y) on the floor: the direction of steepest climb, as long as the climb rate.");
    b.toggle("Normal", "normal", false, "The unit vector at right angles to the plane.");
    b.value("Partial Derivatives", [](const Object& o) {
        const G& g = as<G>(o);
        return g.error().empty() ? format("∂f/∂x = %.4f    ∂f/∂y = %.4f", g.fx(), g.fy()) : std::string();
    });
    b.value("Steepest Climb", [](const Object& o) {
        const G& g = as<G>(o);
        if (!g.error().empty()) return std::string();
        double steep = std::hypot(g.fx(), g.fy());
        if (steep <= 1e-6) return std::string("flat here: a peak, a valley or a saddle point (gradient 0)");
        return format("%.3f per unit, toward %.0f° from the x axis", steep, std::atan2(g.fy(), g.fx()) * 180.0 / kPi);
    });
    b.value("Plane", [](const Object& o) {
        const G& g = as<G>(o);
        if (!g.error().empty()) return std::string();
        auto p = g.point();
        return format("z = %.3g %s %.3g(x − %.3g) %s %.3g(y − %.3g)", p.z, g.fx() < 0 ? "−" : "+", std::fabs(g.fx()), p.x,
                      g.fy() < 0 ? "−" : "+", std::fabs(g.fy()), p.y);
    });
}

void velocity(InspectorBuilder& b) {
    using V = scene::VelocityObject;
    b.group("", "Constant speed on a circle: all the acceleration turns (it points to the center). Use t in \"At u\" to "
                "ride along.");
    for (const char* k : {"x", "y", "z"}) b.formula(k);
    b.range("umin", "umax", "u");
    b.formula("u0");
    b.group("Show");
    b.toggle("Velocity r'", "velocity", true, "Where the point is heading, as long as its speed.");
    b.toggle("Acceleration r''", "acceleration", true, "How the velocity is changing.");
    b.toggle("Split Acceleration", "split", false,
             "The part along the motion changes the speed; the part across it changes the direction (turning).");
    b.anchor("velocity.split");
    b.toggle("Curve", "curve", true, "Draw the curve itself.");
    b.slider("Arrow Scale", [](const Object&) { return InspectorBuilder::Limits{0.1, 2.0, 0.05}; },
             [](const Object& o) { return as<V>(o).arrowScale(); },
             [](Object& o, double s) { o.setProp("scale", std::round(s * 20.0) / 20.0); },
             [](double s) { return format("%.2f×", s); }, "Draw the arrows shorter or longer (the values don't change).");
    b.value("Speed", [](const Object& o) {
        const V& r = as<V>(o);
        if (!r.error().empty()) return std::string();
        return format("|r'| = %.4f    |r''| = %.4f", geometry::length(r.velocity()), geometry::length(r.acceleration()));
    });
    b.value("Acceleration", [](const Object& o) {
        const V& r = as<V>(o);
        if (!r.error().empty()) return std::string();
        double speed = geometry::length(r.velocity());
        double at = geometry::length(r.tangential()), an = geometry::length(r.normal());
        double along = speed > 1e-12 ? geometry::dot(r.acceleration(), r.velocity()) / speed : 0.0;
        double accel = geometry::length(r.acceleration());
        if (std::fabs(along) < 1e-4 * std::max(1.0, accel)) return format("constant speed here    turning %.3f", an);
        return format("%s %.3f    turning %.3f", along > 0 ? "speeding up" : "slowing down", at, an);
    });
}

void taylor(InspectorBuilder& b) {
    using T = scene::TaylorObject;
    b.group("", "Each term matches one more derivative of f at the center: degree 1 is the tangent line. Far from the "
                "center, or past a singularity, the polynomial can't follow f.");
    b.formula("f");
    b.formula("a");
    b.anchor("taylor.a");
    b.formula("N");
    b.anchor("taylor.N");
    b.range("xmin", "xmax", "x");
    b.group("Show");
    b.toggle("Graph of f", "function", true, "Draw f itself (gray) to compare.");
    b.toggle("Previous Degree", "previous", true,
             "Draw the polynomial one degree lower, faint: the difference is what the newest term adds.");
    b.toggle("Separate Terms", "terms", false, "Draw each term c_k (x − a)^k on its own (the first 6).");
    b.value("Polynomial", [](const Object& o) {
        const T& tp = as<T>(o);
        if (!tp.error().empty()) return std::string();
        const taylor::Series& se = tp.series();
        return format("T%d(x) = %s", static_cast<int>(se.coeffs.size()) - 1, scene::polynomialText(se, 4).c_str());
    });
    b.value("Largest Gap to f", [](const Object& o) {
        const T& tp = as<T>(o);
        return tp.error().empty() && std::isfinite(tp.maxError()) ? format("%.3g on the range", tp.maxError())
                                                                  : std::string();
    });
    // How the coefficients were found, or why the degree is lower (a warning then)
    b.line([](const Object& o) {
        const T& tp = as<T>(o);
        const taylor::Series& se = tp.series();
        bool fine = se.complexMethod && static_cast<int>(se.coeffs.size()) > se.requested;
        return tp.error().empty() && fine ? se.note : std::string();
    });
    b.line([](const Object& o) {
        const T& tp = as<T>(o);
        const taylor::Series& se = tp.series();
        bool fine = se.complexMethod && static_cast<int>(se.coeffs.size()) > se.requested;
        return tp.error().empty() && !fine ? se.note : std::string();
    }, InspectorBuilder::Tone::Warning);
}

void newton(InspectorBuilder& b) {
    using N = scene::NewtonObject;
    b.group("", "Each step follows the tangent at (x, f(x)) down to the x axis: x − f(x)/f'(x).");
    b.formula("f");
    b.formula("x0");
    b.formula("steps");
    b.anchor("newton.steps");
    b.range("xmin", "xmax", "x");
    b.toggle("Graph of f", "function", true, "Draw y = f(x).");
    b.group("Steps");
    b.line([](const Object& o) {
        const N& nw = as<N>(o);
        if (!nw.error().empty()) return std::string();
        const auto& run = nw.run();
        std::string all;
        size_t rows = std::min<size_t>(run.steps.size(), 12);
        for (size_t k = 0; k < rows; k++) {
            const auto& s = run.steps[k];
            if (!all.empty()) all += "\n";
            all += std::isfinite(s.fx) ? format("x%-2zu = %-12.8g f = %.3g", k, s.x, std::fabs(s.fx) < 1e-300 ? 0.0 : s.fx)
                                       : format("x%-2zu = %-12.8g", k, s.x);
        }
        if (run.steps.size() > rows) all += "\n…";
        return all;
    }, InspectorBuilder::Tone::Mono);
    auto why = [](const Object& o) -> std::string {
        if (!o.error().empty()) return "";
        switch (as<N>(o).run().stop) {
            case taylor::NewtonStop::Converged:
                return "Converged: the steps stopped moving. Near a root the number of correct digits about doubles "
                       "each step.";
            case taylor::NewtonStop::FlatTangent:
                return "Stopped: the tangent is flat (f' = 0), so it never meets the x axis. Try another start.";
            case taylor::NewtonStop::Undefined: return "Stopped: f is undefined where the step landed.";
            case taylor::NewtonStop::Diverging: return "Diverging: each step overshoots farther. Start closer to the root.";
            case taylor::NewtonStop::Steps: return "";
        }
        return "";
    };
    b.line([why](const Object& o) {
        return o.error().empty() && as<N>(o).run().stop == taylor::NewtonStop::Converged ? why(o) : std::string();
    });
    b.line([why](const Object& o) {
        return o.error().empty() && as<N>(o).run().stop != taylor::NewtonStop::Converged ? why(o) : std::string();
    }, InspectorBuilder::Tone::Warning);
}

} // namespace linalg::gtk::inspectors
