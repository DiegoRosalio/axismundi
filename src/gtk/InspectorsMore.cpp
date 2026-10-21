// The GTK inspectors of quaternions, vector fields and the Fourier types (epicycles, Fourier series). They mirror
// the raylib app's ScenePanelInspectors.cpp.
#include "app/Session.hpp"
#include "gtk/Inspectors.hpp"
#include "math/VectorField.hpp"
#include "rendering/FieldRenderer.hpp"
#include "scene/Templates.hpp"
#include "scene/Types.hpp"
#include "view/SceneView.hpp"
#include <gtkmm/drawingarea.h>
#include <algorithm>
#include <memory>
#include <vector>

namespace linalg::gtk::inspectors {

namespace {

template <class T>
const T& as(const Object& o) {
    return static_cast<const T&>(o);
}

} // namespace

void quaternion(InspectorBuilder& b) {
    using Q = scene::QuaternionObject;
    b.group("", "qaxis(axis, angle) makes a rotation; q1*q2 does q2 first, then q1.");
    b.formula("value");
    b.value("Value", [](const Object& o) {
        expr::Rotation r;
        return o.error().empty() && expr::rotationOf(o.value(), r) ? o.value().summary() : std::string();
    });
    b.value("Rotation", [](const Object& o) {
        expr::Rotation r;
        if (!o.error().empty() || !expr::rotationOf(o.value(), r)) return std::string();
        if (!r.hasAxis) return std::string("No rotation (angle 0)");
        return format("%.1f° about (%.2f, %.2f, %.2f)", r.angle * 180.0 / kPi, r.axis.x, r.axis.y, r.axis.z);
    });
    b.line([](const Object& o) {
        expr::Rotation r;
        if (!o.error().empty()) return std::string();
        if (!expr::rotationOf(o.value(), r))
            return o.value().type == expr::Type::Quaternion ? std::string("The zero quaternion is not a rotation.")
                                                            : std::string();
        return std::fabs(r.norm - 1.0) > 1e-3
                   ? format("|q| = %.3f, not 1: drawn as the rotation of q/|q|. unit(q) makes it 1.", r.norm)
                   : std::string();
    }, InspectorBuilder::Tone::Warning);
    b.group("Show");
    b.toggle("Axis and Angle", "axis", true,
             "The rotation axis through the origin, and an arc showing how far it turns (right-hand rule).");
    b.toggle("Rotated Basis", "basis", true, "Where the rotation sends the x, y and z axes (the faint ones are the originals).");
    b.toggle("Acts on a Vector", "act", false,
             "Shows a vector v (gray) and the turned vector q v q⁻¹, with the circle its tip follows.");
    if (as<Q>(b.object()).actsOnVector()) b.formula("v");
    b.exampleButton(static_cast<int>(quaternionExamples().size()),
                    [](Object& o, int i) { o.setField("value", quaternionExamples()[i]); });
}

void field(InspectorBuilder& b) {
    using F = scene::VectorFieldObject;
    const F& fo = as<F>(b.object());
    b.group("", "F is a vector formula of x, y, z (and t, the scene time); parameters work by name, e.g. k*[-y, x, 0].");
    b.formula("F");
    b.choice({"Plane", "Volume"}, [](const Object& o) { return as<F>(o).volume() ? 1 : 0; },
             {"On the floor z = 0: F's x and y components, e.g. [-y, x, 0].",
              "A box of arrows in space: F(x, y, z) with all three components."},
             [](Object& o, int domain) { o.setProp("domain", domain == 1 ? "volume" : "plane"); });
    b.group();
    b.range("xmin", "xmax", "x");
    b.range("ymin", "ymax", "y");
    if (fo.volume()) b.range("zmin", "zmax", "z");

    b.group("Arrows");
    b.slider("Detail", [](const Object&) { return InspectorBuilder::Limits{F::kDensityMin, F::kDensityMax, 0.25}; },
             [](const Object& o) { return as<F>(o).density(); },
             [](Object& o, double d) { o.setProp("density", std::round(d * 4.0) / 4.0); },
             [](double d) { return format("%g/unit", d); }, "Arrows per unit along each axis. A wider range gets more arrows.");
    b.value("Arrows", [](const Object& o) {
        const F& f = as<F>(o);
        F::Counts cn = f.counts();
        std::string text = f.volume() ? format("%d × %d × %d", cn.nx, cn.ny, cn.nz) : format("%d × %d", cn.nx, cn.ny);
        return text + (cn.capped ? " (capped for speed)" : "");
    });
    b.toggle("Color by Strength", "heatmap", true,
             "Arrows colored by |F| with the heat map (blue weak, red strong); off: the object's color.");
    b.toggle("Same Length", "same_length", false,
             "All arrows one length: only the direction shows (a direction field). The color still shows |F|.");
    b.toggle("Flow Lines", "lines", false, "Curves that follow F everywhere (streamlines), with an arrowhead showing the way.");
    b.toggle("Particles", "particles", false, "Dots drifting along the flow lines as the scene clock plays.");
    if (fo.lines() || fo.particles()) {
        b.slider("Lines per Axis",
                 [](const Object& o) { return InspectorBuilder::Limits{2.0, as<F>(o).volume() ? 4.0 : 16.0, 1.0}; },
                 [](const Object& o) { return static_cast<double>(as<F>(o).seeds()); },
                 [](Object& o, double s) { o.setProp("seeds", std::round(s)); },
                 [](double s) { return format("%.0f", s); }, "How many flow lines start along each axis (evenly spaced).");
    }
    if (fo.particles()) {
        b.slider("Speed", [](const Object&) { return InspectorBuilder::Limits{0.1, 5.0, 0.1}; },
                 [](const Object& o) { return as<F>(o).speed(); },
                 [](Object& o, double s) { o.setProp("speed", std::round(s * 10.0) / 10.0); },
                 [](double s) { return format("%.1f×", s); }, "How fast particles move: 1× is the field's own speed.");
    }
    Session* session = &b.session();
    b.value("Strength |F|", [session](const Object& o) {
        const FieldRenderer* fr = session->sceneView().fieldRenderer(o.id());
        if (!o.error().empty() || !fr || !fr->error().empty() || fr->arrowCount() == 0) return std::string();
        return format("from %.2f to %.2f", fr->minMagnitude(), fr->maxMagnitude());
    });
    b.line([session](const Object& o) {
        const FieldRenderer* fr = session->sceneView().fieldRenderer(o.id());
        return o.error().empty() && fr ? fr->error() : std::string();
    }, InspectorBuilder::Tone::Error);
    b.exampleButton(static_cast<int>(fieldExamples().size()), [](Object& o, int i) {
        o.setField("F", fieldExamples()[i].f);
        o.setProp("domain", fieldExamples()[i].volume ? "volume" : "plane");
    });
}

void epicycles(InspectorBuilder& b) {
    using E = scene::EpicyclesObject;
    b.group("", "u runs once around the curve (0 to 2π). Each circle turns k times per lap; more circles follow the "
                "corners better.");
    b.formula("z");
    b.formula("N");
    b.formula("speed");
    b.value("Circles", [](const Object& o) {
        const E& ep = as<E>(o);
        if (!ep.error().empty() || ep.terms().empty()) return std::string();
        return format("%d circles of %d frequencies (−%d..%d), biggest first", ep.circles(), 2 * E::kMaxK + 1, E::kMaxK,
                      E::kMaxK);
    });
    b.value("Biggest", [](const Object& o) {
        const E& ep = as<E>(o);
        if (!ep.error().empty()) return std::string();
        std::string list;
        for (size_t i = 1; i < std::min<size_t>(ep.terms().size(), 6); i++) {
            const auto& term = ep.terms()[i];
            list += format("%sk = %d: r %.2f", i > 1 ? ",  " : "", term.k, std::abs(term.c));
        }
        return list;
    });
    b.exampleButton(static_cast<int>(epicycleExamples().size()),
                    [](Object& o, int i) { o.setField("z", epicycleExamples()[i]); });
}

void fourierSeries(InspectorBuilder& b) {
    using S = scene::FourierSeriesObject;
    b.group("", "The series repeats f with period b − a. Near a jump the partial sums overshoot by about 9% (Gibbs), "
                "however many terms.");
    b.formula("f");
    b.range("a", "b", "x");
    b.formula("N");
    b.group("Show");
    b.toggle("Show f", "original", true, "Draw f itself (gray) next to the partial sum.");
    b.toggle("Harmonics", "harmonics", false, "Draw the separate waves a_k cos + b_k sin (the first 8).");
    b.toggle("Repeat Outside", "extend", true, "Draw the sum over three periods: the series repeats f with period b − a.");

    // The spectrum: the amplitude √(a_k² + b_k²) of each wave, as bars
    b.group("Spectrum", "How strong each wave k is. A square wave has only odd k, falling like 1/k.");
    struct Bars {
        std::vector<double> amplitudes;
        Color color{};
    };
    auto bars = std::make_shared<Bars>();
    auto* chart = Gtk::make_managed<Gtk::DrawingArea>();
    chart->set_content_height(90);
    chart->set_margin(10);
    chart->set_draw_func([bars, chart](const Cairo::RefPtr<Cairo::Context>& cr, int w, int h) {
        int n = static_cast<int>(bars->amplitudes.size());
        if (n == 0) return;
        double amax = 1e-12;
        for (double a : bars->amplitudes) amax = std::max(amax, a);
        double labelH = 14.0, bw = static_cast<double>(w) / n;
        Gdk::RGBA text = chart->get_color();
        cr->set_font_size(10.0);
        for (int k = 0; k < n; k++) {
            double bh = bars->amplitudes[k] / amax * (h - labelH - 4);
            cr->set_source_rgb(bars->color.r / 255.0, bars->color.g / 255.0, bars->color.b / 255.0);
            cr->rectangle(k * bw + bw * 0.15, h - labelH - bh, bw * 0.7, std::max(1.0, bh));
            cr->fill();
            if (k == 0 || (k + 1) % 5 == 0) {
                std::string label = std::to_string(k + 1);
                Cairo::TextExtents ext;
                cr->get_text_extents(label, ext);
                cr->set_source_rgba(text.get_red(), text.get_green(), text.get_blue(), 0.6);
                cr->move_to(k * bw + bw / 2 - ext.width / 2, h - 2);
                cr->show_text(label);
            }
        }
    });
    b.widget(GTK_WIDGET(chart->gobj()));
    b.live([bars, chart](const Object& o) {
        const S& fs = as<S>(o);
        std::vector<double> amps;
        if (fs.error().empty()) {
            const auto& se = fs.series();
            int n = std::min(static_cast<int>(se.ak.size()), 40);
            for (int k = 0; k < n; k++) amps.push_back(std::hypot(se.ak[k], se.bk[k]));
        }
        Color c = view::SceneView::colorOf(fs);
        if (amps != bars->amplitudes || c.r != bars->color.r || c.g != bars->color.g || c.b != bars->color.b) {
            bars->amplitudes = amps;
            bars->color = c;
            chart->queue_draw();
        }
        chart->set_visible(!amps.empty());
    });
    b.line([](const Object& o) {
        const S& fs = as<S>(o);
        if (!fs.error().empty()) return std::string();
        const auto& se = fs.series();
        std::string all = format("a₀/2 = %.3f", se.a0 / 2);
        for (int k = 1; k <= std::min(static_cast<int>(se.ak.size()), 4); k++) {
            double a = std::fabs(se.ak[k - 1]) < 5e-4 ? 0.0 : se.ak[k - 1];
            double bk = std::fabs(se.bk[k - 1]) < 5e-4 ? 0.0 : se.bk[k - 1];
            all += format("\na%d = %.3f    b%d = %.3f", k, a, k, bk);
        }
        return all;
    }, InspectorBuilder::Tone::Mono);
    b.line([](const Object& o) {
        const S& fs = as<S>(o);
        return fs.error().empty() && fs.undefinedPoints() > 0
                   ? format("f is undefined at %d sample points (counted as 0).", fs.undefinedPoints())
                   : std::string();
    }, InspectorBuilder::Tone::Warning);
    b.exampleButton(static_cast<int>(fourierExamples().size()), [](Object& o, int i) { o.setField("f", fourierExamples()[i]); });
}

} // namespace linalg::gtk::inspectors
