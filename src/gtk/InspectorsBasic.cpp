// The GTK inspectors of the basic object types: vector, matrix, eigenvectors, plots, parameter, complex number,
// complex product. Each describes its rows with an InspectorBuilder (gtk/Inspector.hpp); the table in
// Inspector.cpp maps type ids to them. They mirror the raylib app's ScenePanelInspectors.cpp.
#include "app/Session.hpp"
#include "gtk/Inspectors.hpp"
#include "rendering/PlotRenderer.hpp"
#include "scene/Templates.hpp"
#include "scene/Types.hpp"
#include "view/SceneView.hpp"
#include <cmath>
#include <cstdio>

namespace linalg::gtk {

namespace inspectors {

void vector(InspectorBuilder& b) {
    auto& v = static_cast<scene::VectorObject&>(b.object());
    b.choice({"Components", "Formula"},
             [](const Object& o) { return static_cast<const scene::VectorObject&>(o).byFormula() ? 1 : 0; },
             {"Type the x, y and z components.",
              "Define it from other objects, e.g. a + b, cross(a, b), M*a, 2*normalize(a)."},
             [](Object& o, int mode) {
                 auto& vo = static_cast<scene::VectorObject&>(o);
                 if ((mode == 1) == vo.byFormula()) return;
                 vo.setByFormula(mode == 1);
                 if (mode == 1)
                     vo.setField("value", "[" + vo.field("x")->text + ", " + vo.field("y")->text + ", " +
                                              vo.field("z")->text + "]");
             });
    if (!v.byFormula()) {
        b.group("", "Numbers or formulas such as cos(theta).");
        const char* tip = " component: a number or a formula such as cos(theta)";
        b.cells("", {"x", "y", "z"}, "", {std::string("x") + tip, std::string("y") + tip, std::string("z") + tip},
                {"x", "y", "z"});
    } else {
        b.group("", "From other objects: a + b, cross(a, b), M*a, 2*normalize(a).");
        b.formula("value");
    }
    b.formula("tail");
    b.value("Length", [](const Object& o) {
        if (o.value().type != expr::Type::Vector) return std::string();
        return format("|%s| = %.3f", o.name().c_str(), o.value().vector.magnitude());
    });
}

void matrix(InspectorBuilder& b) {
    auto& m = static_cast<scene::MatrixObject&>(b.object());
    b.choice({"Cells", "Formula"},
             [](const Object& o) { return static_cast<const scene::MatrixObject&>(o).byFormula() ? 1 : 0; },
             {"Type each entry. Entries may use parameters, e.g. cos(theta).",
              "One formula: rotz(theta), scale(2), inv(A), A*B, [a, b, cross(a, b)] (columns)..."},
             [](Object& o, int mode) { static_cast<scene::MatrixObject&>(o).setByFormula(mode == 1); });
    if (!m.byFormula()) {
        b.choice({"3×3", "4×4"}, [](const Object& o) { return static_cast<const scene::MatrixObject&>(o).size() == 4 ? 1 : 0; },
                 {"A linear map of 3D space.",
                  "Adds a fourth row and column (homogeneous coordinates); the last column translates."},
                 [](Object& o, int size) { static_cast<scene::MatrixObject&>(o).setSize(size == 1 ? 4 : 3); });
        b.group("", "Column j is where the matrix sends the j-th axis vector.");
        static const char* columnTips[4] = {
            "Column 1 is where M sends the x-axis vector (1, 0, 0).",
            "Column 2 is where M sends the y-axis vector (0, 1, 0).",
            "Column 3 is where M sends the z-axis vector (0, 0, 1).",
            "Column 4 is the translation (4×4 only).",
        };
        int n = m.size();
        for (int i = 0; i < n; i++) {
            std::vector<std::string> keys, tips;
            for (int j = 0; j < n; j++) {
                keys.push_back(scene::MatrixObject::cellKey(i, j));
                tips.push_back(columnTips[j]);
            }
            b.cells("", keys, "", tips);
        }
        b.anchorGroup("matrix.grid");
        // Presets act on top of the current matrix; Identity starts over
        auto preset = [](Matrix3D value) {
            return [value](Object& o) { static_cast<scene::MatrixObject&>(o).applyPreset(value); };
        };
        b.buttonGrid({{"Identity", "Start over: the identity leaves every vector where it is.", preset(Matrix3D::identity())},
                      {"Rotate 45°", "Adds a 45° turn about the z-axis on top of the current matrix (det unchanged).",
                       preset(Matrix3D::rotation(static_cast<float>(kPi / 4)))},
                      {"Scale 2×", "Then doubles x and y, keeping z (det × 4).", preset(Matrix3D::scale(2, 2))},
                      {"Shear", "Then x' = x + 0.5 y: squares become parallelograms (det unchanged).",
                       preset(Matrix3D::shear(0.5f, 0))},
                      {"Reflect", "Then x' = −x: a mirror image (det changes sign).", preset(Matrix3D::scale(-1, 1))}},
                     2);
        b.anchor("presets");
        b.anchor("preset.rotate", 1);
        b.anchor("preset.reflect", 4);
    } else {
        b.group("", "Try rotz(theta), rot([1,1,1], pi/3), scale(1, 2, 1), translate(1, 0, 0), inv(M), A*B, or "
                    "[u, v, w] to use vectors as columns.");
        b.formula("value");
    }
    b.group();
    b.value("Determinant", [](const Object& o) {
        const expr::Value& v = o.value();
        if (v.type != expr::Type::Matrix) return std::string();
        return format("det = %.4g", v.size == 4 ? v.matrix.determinant() : v.matrix3().determinant());
    }, "How much the matrix scales volumes; negative when it mirrors");
}

void eigen(InspectorBuilder& b) {
    b.group("", "Dashed lines show the directions this matrix only stretches; the dots mark λv.");
    b.formula("of");
}

void plot(InspectorBuilder& b) {
    auto& p = static_cast<scene::PlotObject&>(b.object());
    const std::string type = p.typeId();
    bool curve = type == "curve";
    bool grid2D = type == "surface" || type == "implicit";
    b.group("", curve ? "u runs along the curve; t is the scene time. Parameters work by name too."
                      : "Use x, y, t (the scene time) and parameters by name, e.g. a*sin(x - t).");
    if (curve) {
        for (const char* k : {"x", "y", "z"}) b.formula(k);
        b.range("umin", "umax", "u");
    } else {
        b.formula("f");
        b.anchor("plot.expr");
        b.range("xmin", "xmax", "x");
        if (type != "function") b.range("ymin", "ymax", "y");
    }

    b.group("Drawing");
    auto dr = scene::PlotObject::densityRange(p.kind());
    b.slider("Detail", [dr](const Object&) { return InspectorBuilder::Limits{dr.min, dr.max, 0.5}; },
             [](const Object& o) { return static_cast<const scene::PlotObject&>(o).density(); },
             [](Object& o, double d) {
                 static_cast<scene::PlotObject&>(o).setDensity(d < 10 ? std::round(d * 2) / 2.0 : std::round(d));
             },
             [](double d) { return format("%g/unit", d); },
             grid2D ? "Grid cells per unit of x and y. A wider range keeps the same smoothness."
                    : (curve ? "Samples per unit of u. A longer range keeps the same smoothness."
                             : "Samples per unit of x. A wider range keeps the same smoothness."));
    b.value("Samples", [grid2D](const Object& o) {
        scene::PlotObject::Samples sm = static_cast<const scene::PlotObject&>(o).samples();
        if (grid2D)
            return sm.capped ? format("%d × %d cells (capped from %d × %d for speed)", sm.nx, sm.ny, sm.wantedX, sm.wantedY)
                             : format("%d × %d cells", sm.nx, sm.ny);
        return sm.capped ? format("%d samples (capped from %d for speed)", sm.nx, sm.wantedX) : format("%d samples", sm.nx);
    });
    b.toggle("Heat Map", "heatmap", true, "Color by value, from low to high");
    Session* session = &b.session();
    b.value("Values", [session](const Object& o) {
        const PlotRenderer* r = session->sceneView().plot(o.id());
        if (!o.error().empty() || !r || !r->error().empty()) return std::string();
        return format("from %.2f to %.2f", r->minValue(), r->maxValue());
    });
    b.line([session](const Object& o) {
        const PlotRenderer* r = session->sceneView().plot(o.id());
        return o.error().empty() && r ? r->error() : std::string();
    }, InspectorBuilder::Tone::Error);
    const auto& examples = plotExamples(type);
    b.exampleButton(static_cast<int>(examples.size()), [type, curve](Object& o, int i) {
        const PlotExample& e = plotExamples(type)[i];
        if (curve) {
            o.setField("x", e.x);
            o.setField("y", e.y);
            o.setField("z", e.z);
        } else {
            o.setField("f", e.f);
        }
    });
}

void parameter(InspectorBuilder& b) {
    using PMode = scene::ParameterObject::Mode;
    auto& p = static_cast<scene::ParameterObject&>(b.object());
    auto param = [](Object& o) -> scene::ParameterObject& { return static_cast<scene::ParameterObject&>(o); };
    auto cparam = [](const Object& o) -> const scene::ParameterObject& {
        return static_cast<const scene::ParameterObject&>(o);
    };
    b.choice({"Slider", "Animate", "Formula"}, [cparam](const Object& o) { return static_cast<int>(cparam(o).mode()); },
             {"Set the value by hand. It stays where you put it.",
              "Follows the scene clock from min to max, with a repeat style and an easing curve.",
              "A formula of t, the scene time in seconds, e.g. 2pi*t/5 or sin(t)."},
             [param](Object& o, int m) { param(o).setMode(static_cast<PMode>(m)); });
    b.anchor("parameter.mode");
    b.group("", "Use " + p.name() + " in any formula, e.g. rotz(" + p.name() + ") or lerp(eye(), M, " + p.name() + ").");
    if (p.mode() == PMode::Formula) b.formula("formula");
    if (p.mode() == PMode::Slider) {
        b.slider("Value",
                 [cparam](const Object& o) {
                     double lo = cparam(o).minimum(), hi = cparam(o).maximum();
                     return InspectorBuilder::Limits{lo, hi, std::fabs(hi - lo) / 1000.0};
                 },
                 [cparam](const Object& o) { return cparam(o).number(); },
                 [param](Object& o, double v) { param(o).setNumber(v); }, shortNumber);
        b.anchor("parameter.slider");
    }
    if (p.mode() != PMode::Formula) {
        auto bound = [param, cparam](const char* key) {
            return InspectorBuilder::Number{[cparam, key](const Object& o) { return cparam(o).propNumber(key); },
                                            [param, key](Object& o, double v) {
                                                param(o).setProp(key, v);
                                                param(o).setNumber(param(o).number());  // clamp into the range
                                                return true;
                                            }};
        };
        b.numbers("Range", {bound("min"), bound("max")}, "to");
    }
    if (p.mode() == PMode::Animate) {
        b.combo("Repeat", {"Once", "Loop", "Ping-pong"},
                [cparam](const Object& o) { return static_cast<int>(cparam(o).playback().repeat); },
                [param](Object& o, int i) {
                    Playback pb = param(o).playback();
                    pb.repeat = static_cast<Repeat>(i);
                    param(o).setPlayback(pb);
                },
                "Once: min to max and stop. Loop: start over. Ping-pong: back and forth.");
        std::vector<std::string> easings;
        for (int i = 0; i < kEasingCount; i++) easings.push_back(easingName(static_cast<Easing>(i)));
        b.combo("Easing", easings, [cparam](const Object& o) { return static_cast<int>(cparam(o).playback().easing); },
                [param](Object& o, int i) {
                    Playback pb = param(o).playback();
                    pb.easing = static_cast<Easing>(i);
                    param(o).setPlayback(pb);
                },
                "How the value speeds up and slows down on its way from min to max");
        b.slider("Duration", [](const Object&) { return InspectorBuilder::Limits{0.5, 30.0, 0.1}; },
                 [cparam](const Object& o) { return static_cast<double>(cparam(o).playback().duration); },
                 [param](Object& o, double d) {
                     Playback pb = param(o).playback();
                     pb.duration = static_cast<float>(std::round(d * 10.0) / 10.0);
                     param(o).setPlayback(pb);
                 },
                 [](double d) { return format("%.1f s", d); }, "Seconds for one run from min to max.");
        b.numbers("Start At (s)",
                  {{[cparam](const Object& o) { return cparam(o).startDelay(); },
                    [param](Object& o, double v) {
                        if (v < 0) return false;
                        param(o).setProp("anim.start", v);
                        return true;
                    }}},
                  "", "Seconds of scene time before this parameter starts moving.");
    }
    if (p.timeDriven()) {
        Session* session = &b.session();
        b.group("Scene Clock");
        b.value("Now", [session, cparam](const Object& o) {
            return format("t = %.2f s     %s = %s", session->scene().timeline().time, o.name().c_str(),
                          shortNumber(cparam(o).number()).c_str());
        });
        b.button("Play or Pause", "media-playback-start-symbolic", [session](Object&) {
            scene::Timeline& tl = session->scene().timeline();
            tl.playing = !tl.playing;
        }, "Time-driven parameters follow the scene clock");
    }
}

void complexNumber(InspectorBuilder& b) {
    b.group();
    b.formula("value");
    b.value("Polar Form", [](const Object& o) {
        const expr::Value& v = o.value();
        if (!v.isNumber()) return std::string();
        double r = std::hypot(v.re, v.im), a = std::atan2(v.im, v.re) * 180.0 / kPi;
        if (std::fabs(a) < 0.05) a = 0.0;  // no "-0.0°"
        return format("|%s| = %.3f   arg = %.1f°", o.name().c_str(), r, a);
    });
    b.group("Show");
    b.toggle("Circle Through It", "circle", false, "The circle of all numbers with the same length");
    b.toggle("cos / sin Projections", "projections", false, "Its shadows on the real and imaginary axes");
    b.toggle("Helix of e^(it)", "helix", false, "The path r·e^(it) for t from 0 to its angle, rising along Z");
}

void product(InspectorBuilder& b) {
    b.group("", "Multiplying complex numbers multiplies their lengths and adds their angles.");
    b.formula("a");
    b.formula("b");
    auto polar = [](const Object& o, bool angle) {
        const scene::Field* fa = o.field("a");
        const scene::Field* fb = o.field("b");
        if (!o.value().isNumber() || !fa->value.isNumber() || !fb->value.isNumber()) return std::string();
        double ra = std::hypot(fa->value.re, fa->value.im), rb = std::hypot(fb->value.re, fb->value.im);
        double aa = std::atan2(fa->value.im, fa->value.re) * 180 / kPi, ab = std::atan2(fb->value.im, fb->value.re) * 180 / kPi;
        return angle ? format("%.1f° + %.1f° = %.1f°", aa, ab, aa + ab) : format("%.2f × %.2f = %.2f", ra, rb, ra * rb);
    };
    b.value("Length", [polar](const Object& o) { return polar(o, false); });
    b.value("Angle", [polar](const Object& o) { return polar(o, true); });
    b.toggle("Show Rotation and Scaling", "angles", true, "The two angles and lengths drawn in the plane");
}

} // namespace inspectors

} // namespace linalg::gtk
