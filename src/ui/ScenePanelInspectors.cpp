// The inspector content of each object type (ScenePanel::drawInspector dispatches here).
#include "ui/ScenePanel.hpp"
#include "scene/Templates.hpp"
#include "math/Decompositions.hpp"
#include "math/VectorField.hpp"
#include "ui/ScenePanelDetail.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace linalg {

using scene::SceneObject;
using scene::SpaceKind;
using namespace panel;

// "label [lo] to [hi]": two short formula fields side by side (ranges)
void ScenePanel::rangeRow(InspectorContext& c, const char* lo, const char* hi, const char* label) {
    const ui::Theme& t = ui::theme();
    scene::Scene& sc = c.sc;
    SpaceKind kind = c.kind;
    scene::ObjectId oid = c.o.id();
    Rectangle row = c.col.row(34.0f);
    Rectangle l = ui::takeLeft(row, 34.0f, 6.0f);
    ui::textIn(l, label, ui::FontRole::Body, t.textMuted);
    auto cells = ui::splitRow(row, {1, 1}, 26.0f);
    const char* keys[2] = {lo, hi};
    for (int k = 0; k < 2; k++) {
        std::string key = keys[k];
        scene::Field* f = c.o.field(key);
        if (!f) continue;
        editText(cells[k], fieldId(c.o, key), f->text, !f->error.empty(), [&sc, kind, oid, key](const std::string& text) {
            SceneObject* ob = sc.space(kind).find(oid);
            if (!ob) return false;
            ob->setField(key, text);
            sc.commit();
            return true;
        });
    }
    ui::textIn({cells[0].x + cells[0].width, row.y, ui::px(26), row.height}, "to", ui::FontRole::Small, t.textMuted,
               ui::Align::Center);
}

// An on/off switch bound to a property of the object (one undo step per change); returns the new value
bool ScenePanel::toggleProp(InspectorContext& c, const char* text, const char* key, bool current, const char* tip) {
    Rectangle r = c.col.row(30);
    bool v = current;
    if (ui::toggleRow(r, text, v)) {
        c.o.setProp(key, v);
        c.sc.commit();
    }
    ui::tooltip(r, tip, text);
    return v;
}

void ScenePanel::inspectVector(InspectorContext& c) {
    ui::Column& col = c.col;
    SceneObject& o = c.o;
    scene::Scene& sc = c.sc;
    SpaceKind kind = c.kind;
    scene::ObjectId oid = c.o.id();
    const ui::Theme& t = ui::theme();
    auto commit = [&sc]() { sc.commit(); };
    auto& v = static_cast<scene::VectorObject&>(o);
    int mode = v.byFormula() ? 1 : 0;
    if (ui::segmented(col.row(32), {"Components", "Formula"}, mode,
                      {"Type the x, y and z components.",
                       "Define it from other objects, e.g. a + b, cross(a, b), M*a, 2*normalize(a)."})) {
        m_activeId = -1;
        v.setByFormula(mode == 1);
        if (mode == 1) v.setField("value", "[" + v.field("x")->text + ", " + v.field("y")->text + ", " +
                                                v.field("z")->text + "]");
        commit();
    }
    if (!v.byFormula()) {
        Rectangle row = col.row(34.0f);
        auto cells = ui::splitRow(row, {1, 1, 1}, 6.0f);
        const char* keys[3] = {"x", "y", "z"};
        for (int k = 0; k < 3; k++) {
            scene::Field* f = v.field(keys[k]);
            int id = fieldId(v, keys[k]);
            std::string key = keys[k];
            editText(cells[k], id, f->text, !f->error.empty(), [&sc, kind, oid, key](const std::string& text) {
                SceneObject* ob = sc.space(kind).find(oid);
                if (!ob) return false;
                ob->setField(key, text);
                sc.commit();
                return true;
            });
            ui::tooltip(cells[k], (std::string(keys[k]) + " component: a number or a formula such as cos(theta)").c_str());
        }
        for (int k = 0; k < 3; k++) {
            if (!v.field(keys[k])->error.empty()) note(col, std::string(keys[k]) + ": " + v.field(keys[k])->error, t.danger);
        }
    } else {
        col.setY(fieldRow(col.x(), col.y(), col.width(), v, "value", sc, 0.0f));
    }
    col.setY(fieldRow(col.x(), col.y(), col.width(), v, "tail", sc));
    if (v.value().type == expr::Type::Vector) {
        Vector3D val = v.value().vector;
        char buf[96];
        snprintf(buf, sizeof(buf), "|%s| = %.3f", v.name().c_str(), val.magnitude());
        ui::textIn(col.row(18), buf, ui::FontRole::Mono, t.textMuted);
    }
}

void ScenePanel::inspectMatrix(InspectorContext& c) {
    ui::Column& col = c.col;
    SceneObject& o = c.o;
    scene::Scene& sc = c.sc;
    SpaceKind kind = c.kind;
    scene::ObjectId oid = c.o.id();
    const ui::Theme& t = ui::theme();
    auto commit = [&sc]() { sc.commit(); };
    auto& m = static_cast<scene::MatrixObject&>(o);
    int mode = m.byFormula() ? 1 : 0;
    if (ui::segmented(col.row(32), {"Cells", "Formula"}, mode,
                      {"Type each entry. Entries may use parameters, e.g. cos(theta).",
                       "One formula: rotz(theta), scale(2), inv(A), A*B, [a, b, cross(a, b)] (columns)..."})) {
        m_activeId = -1;
        m.setByFormula(mode == 1);
        commit();
    }
    if (!m.byFormula()) {
        Rectangle top = col.row(32.0f);
        top.width = std::min(top.width, ui::px(150.0f));
        int size = m.size() == 4 ? 1 : 0;
        if (ui::segmented(top, {"3×3", "4×4"}, size,
                          {"A linear map of 3D space.",
                           "Adds a fourth row and column (homogeneous coordinates); the last column translates."})) {
            m_activeId = -1;
            m.setSize(size == 1 ? 4 : 3);
            commit();
        }
        int n = m.size();
        Rectangle grid = {col.x(), col.y(), col.width(), 0};
        for (int i = 0; i < n; i++) {
            Rectangle row = col.row(n == 4 ? 32.0f : 36.0f);
            grid = ui::unite(grid, row);
            auto cells = n == 4 ? ui::splitRow(row, {1, 1, 1, 1}, 6.0f) : ui::splitRow(row, {1, 1, 1}, 6.0f);
            for (int j = 0; j < n; j++) {
                std::string key = scene::MatrixObject::cellKey(i, j);
                scene::Field* f = m.field(key);
                editText(cells[j], fieldId(m, key), f->text, !f->error.empty(), [&sc, kind, oid, key](const std::string& text) {
                    SceneObject* ob = sc.space(kind).find(oid);
                    if (!ob) return false;
                    ob->setField(key, text);
                    sc.commit();
                    return true;
                });
                static const char* columnTips[4] = {
                    "Column 1 is where M sends the x-axis vector (1, 0, 0).",
                    "Column 2 is where M sends the y-axis vector (0, 1, 0).",
                    "Column 3 is where M sends the z-axis vector (0, 0, 1).",
                    "Column 4 is the translation (4×4 only).",
                };
                if (m_activeId != fieldId(m, key)) ui::tooltip(cells[j], columnTips[j]);
            }
        }
        ui::anchor("matrix.grid", grid);
        if (!m.error().empty()) note(col, m.error(), t.danger);

        // Presets
        struct Preset { const char* label; const char* key; Matrix3D value; const char* tip; };
        const Preset presets[] = {
            {"Identity", "I", Matrix3D::identity(), "Start over: back to the identity, which leaves every vector "
                                                    "where it is."},
            {"Rotate 45°", "R", Matrix3D::rotation(kPi / 4), "Adds a 45° turn about the z-axis on top of the "
                                                             "current matrix (det unchanged)."},
            {"Scale 2×", "S", Matrix3D::scale(2, 2), "Then doubles x and y, keeping z (det × 4)."},
            {"Shear", "X", Matrix3D::shear(0.5f, 0), "Then x' = x + 0.5 y: squares become parallelograms (det "
                                                    "unchanged)."},
            {"Reflect", "F", Matrix3D::scale(-1, 1), "Then x' = −x: a mirror image (det changes sign)."},
        };
        col.space(2.0f);
        Rectangle presetArea = {col.x(), col.y(), col.width(), 0};
        for (int i = 0; i < 5; i += 2) {
            Rectangle row = col.row(30.0f);
            presetArea = ui::unite(presetArea, row);
            auto cells = ui::splitRow(row, {1, 1}, 6.0f);
            for (int k = 0; k < 2 && i + k < 5; k++) {
                const Preset& p = presets[i + k];
                if (ui::button(cells[k], p.label, ui::ButtonKind::Secondary, true, ui::Icon::None, p.key)) {
                    m_activeId = -1;
                    m.applyPreset(p.value);  // on top of the current matrix; Identity starts over
                    commit();
                }
                ui::tooltip(cells[k], p.tip, p.label);
                if (i + k == 1) ui::anchor("preset.rotate", cells[k]);
                if (i + k == 4) ui::anchor("preset.reflect", cells[k]);
            }
        }
        ui::anchor("presets", presetArea);
    } else {
        col.setY(fieldRow(col.x(), col.y(), col.width(), m, "value", sc, 0.0f));
        note(col, "Try rotz(theta), rot([1,1,1], pi/3), scale(1, 2, 1), translate(1, 0, 0), inv(M), A*B, "
                  "or [u, v, w] to use vectors as columns.", t.textFaint);
    }
    if (m.value().type == expr::Type::Matrix) {
        float det = m.value().size == 4 ? m.value().matrix.determinant() : m.value().matrix3().determinant();
        char buf[64];
        snprintf(buf, sizeof(buf), "det = %.4g", det);
        ui::textIn(col.row(18), buf, ui::FontRole::Mono, t.textMuted);
    }
}

void ScenePanel::inspectEigen(InspectorContext& c) {
    ui::Column& col = c.col;
    SceneObject& o = c.o;
    scene::Scene& sc = c.sc;
    const ui::Theme& t = ui::theme();
    col.setY(fieldRow(col.x(), col.y(), col.width(), o, "of", sc));
    note(col, "Dashed lines show the directions this matrix only stretches; the dots mark λv.", t.textMuted);
}

void ScenePanel::inspectPlot(InspectorContext& c) {
    ui::Column& col = c.col;
    SceneObject& o = c.o;
    scene::Scene& sc = c.sc;
    const view::SceneView& view = c.view;
    const ui::Theme& t = ui::theme();
    const std::string& type = c.o.typeId();
    auto commit = [&sc]() { sc.commit(); };
    auto& p = static_cast<scene::PlotObject&>(o);
    bool curve = (type == "curve");
    if (curve) {
        for (const char* k : {"x", "y", "z"}) col.setY(fieldRow(col.x(), col.y(), col.width(), p, k, sc));
    } else {
        col.setY(fieldRow(col.x(), col.y(), col.width(), p, "f", sc));
        ui::anchor("plot.expr", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    }
    if (curve) rangeRow(c, "umin", "umax", "u");
    else {
        rangeRow(c, "xmin", "xmax", "x");
        if (type != "function") rangeRow(c, "ymin", "ymax", "y");
    }

    Rectangle row = col.row(30.0f);
    Rectangle label = ui::takeLeft(row, 52.0f, 8.0f);
    Rectangle value = ui::takeRight(row, 64.0f, 8.0f);
    ui::textIn(label, "Detail", ui::FontRole::Body, t.text);
    auto dr = scene::PlotObject::densityRange(p.kind());
    float density = static_cast<float>(p.density());
    if (ui::slider(row, density, static_cast<float>(dr.min), static_cast<float>(dr.max))) {
        p.setDensity(density < 10 ? std::round(density * 2) / 2.0 : std::round(static_cast<double>(density)));
        m_sliderDirty = true;
    }
    ui::anchor("plot.detail", row);
    bool grid2D = type == "surface" || type == "implicit";
    char dbuf[48];
    snprintf(dbuf, sizeof(dbuf), "%g/unit", p.density());
    ui::textIn(value, dbuf, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
    ui::tooltip({label.x, row.y, value.x + value.width - label.x, row.height},
                grid2D ? "Grid cells per unit of x and y. A wider range keeps the same smoothness."
                       : (curve ? "Samples per unit of t. A longer range keeps the same smoothness."
                                : "Samples per unit of x. A wider range keeps the same smoothness."),
                "Detail");
    scene::PlotObject::Samples sm = p.samples();
    char sbuf[128];
    if (grid2D) {
        if (sm.capped)
            snprintf(sbuf, sizeof(sbuf), "%d × %d cells (capped from %d × %d for speed)", sm.nx, sm.ny, sm.wantedX,
                     sm.wantedY);
        else
            snprintf(sbuf, sizeof(sbuf), "%d × %d cells", sm.nx, sm.ny);
    } else {
        if (sm.capped) snprintf(sbuf, sizeof(sbuf), "%d samples (capped from %d for speed)", sm.nx, sm.wantedX);
        else snprintf(sbuf, sizeof(sbuf), "%d samples", sm.nx);
    }
    note(col, sbuf, sm.capped ? t.warning : t.textFaint);
    bool heat = p.heatmap();
    if (ui::toggleRow(col.row(30), "Heat map", heat)) {
        p.setProp("heatmap", heat);
        commit();
    }
    if (ui::button(col.row(30), "Next example", ui::ButtonKind::Secondary, true, ui::Icon::Gallery)) {
        m_activeId = -1;
        const auto& list = plotExamples(type);
        m_exampleIndex = (m_exampleIndex + 1) % static_cast<int>(list.size());
        const PlotExample& e = list[m_exampleIndex];
        if (curve) {
            p.setField("x", e.x);
            p.setField("y", e.y);
            p.setField("z", e.z);
        } else {
            p.setField("f", e.f);
        }
        commit();
    }
    const PlotRenderer* r = view.plot(p.id());
    if (!p.error().empty()) note(col, p.error(), t.danger);
    else if (r && !r->error().empty()) note(col, r->error(), t.danger);
    else if (r) {
        char buf[96];
        snprintf(buf, sizeof(buf), "Values range over [%.2f, %.2f]", r->minValue(), r->maxValue());
        note(col, buf, t.textMuted);
    }
    note(col, curve ? "u runs along the curve; t is the scene time. Parameters work by name too."
                    : "Use x, y, t (the scene time) and parameters by name, e.g. a*sin(x - t).",
         t.textFaint);
}

void ScenePanel::inspectEpicycles(InspectorContext& c) {
    ui::Column& col = c.col;
    SceneObject& o = c.o;
    scene::Scene& sc = c.sc;
    const ui::Theme& t = ui::theme();
    auto commit = [&sc]() { sc.commit(); };
    auto& ep = static_cast<scene::EpicyclesObject&>(o);
    col.setY(fieldRow(col.x(), col.y(), col.width(), ep, "z", sc, 46.0f));
    ui::anchor("epicycles.z", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    col.setY(fieldRow(col.x(), col.y(), col.width(), ep, "N", sc, 110.0f));
    ui::anchor("epicycles.N", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    col.setY(fieldRow(col.x(), col.y(), col.width(), ep, "speed", sc, 110.0f));
    if (ep.error().empty() && !ep.terms().empty()) {
        char buf[160];
        snprintf(buf, sizeof(buf), "%d circles of %d frequencies (−%d..%d), biggest first", ep.circles(),
                 2 * scene::EpicyclesObject::kMaxK + 1, scene::EpicyclesObject::kMaxK, scene::EpicyclesObject::kMaxK);
        note(col, buf, t.text);
        std::string list;
        for (size_t i = 1; i < std::min<size_t>(ep.terms().size(), 6); i++) {
            const auto& term = ep.terms()[i];
            char one[48];
            snprintf(one, sizeof(one), "%sk = %d: r %.2f", i > 1 ? ",  " : "", term.k, std::abs(term.c));
            list += one;
        }
        note(col, list, t.textMuted);
    }
    if (ui::button(col.row(30), "Next example", ui::ButtonKind::Secondary, true, ui::Icon::Gallery)) {
        m_activeId = -1;
        const auto& shapes = epicycleExamples();
        m_exampleIndex = (m_exampleIndex + 1) % static_cast<int>(shapes.size());
        ep.setField("z", shapes[m_exampleIndex]);
        commit();
    }
    note(col, "u runs once around the curve (0 to 2π). Each circle turns k times per lap; more circles follow "
              "the corners better. Press P to play.",
         t.textFaint);
}

void ScenePanel::inspectFourierSeries(InspectorContext& c) {
    ui::Column& col = c.col;
    SceneObject& o = c.o;
    scene::Scene& sc = c.sc;
    const ui::Theme& t = ui::theme();
    auto commit = [&sc]() { sc.commit(); };
    auto& fs = static_cast<scene::FourierSeriesObject&>(o);
    col.setY(fieldRow(col.x(), col.y(), col.width(), fs, "f", sc, 52.0f));
    ui::anchor("series.f", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    rangeRow(c, "a", "b", "x");
    col.setY(fieldRow(col.x(), col.y(), col.width(), fs, "N", sc, 52.0f));
    ui::anchor("series.N", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    toggleProp(c, "Show f", "original", fs.showOriginal(), "Draw f itself (gray) next to the partial sum.");
    toggleProp(c, "Harmonics", "harmonics", fs.showHarmonics(), "Draw the separate waves a_k cos + b_k sin (the first 8).");
    ui::anchor("series.harmonics", {col.x(), col.y() - ui::px(34), col.width(), ui::px(30)});
    toggleProp(c, "Repeat outside", "extend", fs.extended(),
               "Draw the sum over three periods: the series repeats f with period b − a.");
    if (fs.error().empty()) {
        // Spectrum: the amplitude √(a_k² + b_k²) of each wave
        const auto& se = fs.series();
        int n = static_cast<int>(se.ak.size());
        int bars = std::min(n, 40);
        if (bars > 0) {
            Rectangle chart = col.row(78.0f);
            ui::fillRounded(chart, ui::px(6), t.panelAlt);
            double amax = 1e-12;
            for (int k = 0; k < bars; k++) amax = std::max(amax, std::hypot(se.ak[k], se.bk[k]));
            float pad = ui::px(8), bw = (chart.width - 2 * pad) / bars;
            float base = chart.y + chart.height - ui::px(16);
            for (int k = 0; k < bars; k++) {
                double amp = std::hypot(se.ak[k], se.bk[k]);
                float h = static_cast<float>(amp / amax) * (chart.height - ui::px(26));
                Rectangle bar = {chart.x + pad + k * bw + bw * 0.15f, base - h, bw * 0.7f, std::max(1.0f, h)};
                DrawRectangleRec(bar, view::SceneView::colorOf(fs));
                if ((k + 1) % 5 == 0 || k == 0) {
                    char kb[8];
                    snprintf(kb, sizeof(kb), "%d", k + 1);
                    ui::textIn({chart.x + pad + k * bw - ui::px(6), base + ui::px(1), bw + ui::px(12), ui::px(14)}, kb,
                               ui::FontRole::Small, t.textFaint, ui::Align::Center);
                }
            }
            ui::anchor("series.spectrum", chart);
            ui::tooltip(chart, "How strong each wave k is: √(a_k² + b_k²). A square wave has only odd k, falling "
                               "like 1/k.",
                        "Spectrum");
        }
        char buf[160];
        snprintf(buf, sizeof(buf), "a₀/2 = %.3f", se.a0 / 2);
        note(col, buf, t.text);
        for (int k = 1; k <= std::min(n, 4); k++) {
            char one[64];
            snprintf(one, sizeof(one), "a%d = %.3f    b%d = %.3f", k, std::fabs(se.ak[k - 1]) < 5e-4 ? 0.0 : se.ak[k - 1],
                     k, std::fabs(se.bk[k - 1]) < 5e-4 ? 0.0 : se.bk[k - 1]);
            note(col, one, t.textMuted);
        }
        if (fs.undefinedPoints() > 0) {
            snprintf(buf, sizeof(buf), "f is undefined at %d sample points (counted as 0).", fs.undefinedPoints());
            note(col, buf, t.warning);
        }
    }
    if (ui::button(col.row(30), "Next example", ui::ButtonKind::Secondary, true, ui::Icon::Gallery)) {
        m_activeId = -1;
        const auto& fns = fourierExamples();
        m_exampleIndex = (m_exampleIndex + 1) % static_cast<int>(fns.size());
        fs.setField("f", fns[m_exampleIndex]);
        commit();
    }
    note(col, "The series repeats f with period b − a. Near a jump the partial sums overshoot by about 9% "
              "(Gibbs), however many terms.",
         t.textFaint);
}

void ScenePanel::inspectFrame(InspectorContext& c) {
    ui::Column& col = c.col;
    SceneObject& o = c.o;
    scene::Scene& sc = c.sc;
    const ui::Theme& t = ui::theme();
    auto& fr = static_cast<scene::FrameObject&>(o);
    for (const char* k : {"x", "y", "z"}) col.setY(fieldRow(col.x(), col.y(), col.width(), fr, k, sc, 64.0f));
    ui::anchor("frame.curve", {col.x(), col.y() - ui::px(120), col.width(), ui::px(116)});
    rangeRow(c, "umin", "umax", "u");
    col.setY(fieldRow(col.x(), col.y(), col.width(), fr, "u0", sc, 64.0f));
    ui::anchor("frame.u0", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    toggleProp(c, "Frame T, N, B", "frame", fr.showFrame(),
               "Tangent T (direction of motion), normal N (towards the center of the turn), binormal B = T × N.");
    toggleProp(c, "Osculating circle", "circle", fr.showCircle(),
               "The circle that hugs the curve best at the point: radius 1/κ, in the plane of T and N.");
    toggleProp(c, "Curvature comb", "comb", fr.showComb(),
               "Teeth along the curve, as long as the curvature there: where it bends most, they are longest.");
    ui::anchor("frame.comb", {col.x(), col.y() - ui::px(34), col.width(), ui::px(30)});
    toggleProp(c, "Curve", "curve", fr.showCurve(), "Draw the curve itself.");
    if (fr.error().empty()) {
        const geometry::Frame& f = fr.frame();
        char buf[160];
        snprintf(buf, sizeof(buf), "κ = %.4f   (radius %s)", f.kappa,
                 f.kappa > 1e-9 ? std::to_string(1.0 / f.kappa).substr(0, 6).c_str() : "∞, straight");
        note(col, buf, t.text);
        snprintf(buf, sizeof(buf), "τ = %.4f   speed |r'| = %.3f", std::fabs(f.tau) < 5e-5 ? 0.0 : f.tau, f.speed);
        note(col, buf, t.text);
        note(col, "κ: how sharply it turns (1/radius). τ: how fast it twists out of its plane (0 for flat curves). "
                  "Use t in \"At u\" to ride along.",
             t.textFaint);
    }
}

void ScenePanel::inspectIntegral(InspectorContext& c) {
    ui::Column& col = c.col;
    SceneObject& o = c.o;
    scene::Scene& sc = c.sc;
    const ui::Theme& t = ui::theme();
    const std::string& type = c.o.typeId();
    auto commit = [&sc]() { sc.commit(); };
    bool dbl = type == "doubleintegral";
    char buf[160];
    if (dbl) {
        auto& d = static_cast<scene::DoubleIntegralObject&>(o);
        col.setY(fieldRow(col.x(), col.y(), col.width(), d, "f", sc, 34.0f));
        ui::anchor("integral.f", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
        rangeRow(c, "xmin", "xmax", "x");
        rangeRow(c, "ymin", "ymax", "y");
        col.setY(fieldRow(col.x(), col.y(), col.width(), d, "n", sc, 110.0f));
        ui::anchor("integral.n", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
        Rectangle ruleRow = col.row(30.0f);
        int rule = static_cast<int>(d.rule());
        ui::anchor("integral.rule", ruleRow);
        if (ui::segmented(ruleRow, {"Left", "Middle", "Right"}, rule,
                          {"Each box takes the height of f at its lower-left corner.",
                           "Each box takes the height of f at its center (usually the most accurate).",
                           "Each box takes the height of f at its upper-right corner."})) {
            d.setProp("rule", rule == 0 ? "left" : rule == 2 ? "right" : "middle");
            commit();
        }
        bool boxes = d.showBoxes(), surface = d.showSurface();
        if (ui::toggleRow(col.row(30), "Boxes", boxes)) {
            d.setProp("boxes", boxes);
            commit();
        }
        if (ui::toggleRow(col.row(30), "Surface", surface)) {
            d.setProp("surface", surface);
            commit();
        }
        if (d.error().empty()) {
            int n = d.boxesPerSide();
            const char* ruleName = rule == 0 ? "left" : rule == 2 ? "right" : "middle";
            snprintf(buf, sizeof(buf), "Boxes (%d × %d, %s): %.4f", n, n, ruleName, d.riemann().sum);
            note(col, buf, t.text);
            snprintf(buf, sizeof(buf), "Volume ∫∫ f dA ≈ %.4f", d.exact());
            note(col, buf, t.text);
            snprintf(buf, sizeof(buf), "Error of the boxes: %.4f", d.riemann().sum - d.exact());
            note(col, buf, t.textMuted);
            if (d.riemann().skipped > 0) {
                snprintf(buf, sizeof(buf), "%d boxes left out: f is undefined there.", d.riemann().skipped);
                note(col, buf, t.warning);
            }
        }
        note(col, "More boxes get closer to the exact volume. Volume below the floor counts negative. Use a "
                  "parameter in n (e.g. round(2 + 18*s)) to watch the boxes refine.",
             t.textFaint);
    } else {
        auto& li = static_cast<scene::LineIntegralObject&>(o);
        col.setY(fieldRow(col.x(), col.y(), col.width(), li, "F", sc, 92.0f));
        ui::anchor("integral.F", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
        for (const char* k : {"x", "y", "z"}) col.setY(fieldRow(col.x(), col.y(), col.width(), li, k, sc, 64.0f));
        rangeRow(c, "umin", "umax", "u");
        col.setY(fieldRow(col.x(), col.y(), col.width(), li, "upto", sc, 64.0f));
        ui::anchor("integral.upto", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
        if (li.error().empty()) {
            snprintf(buf, sizeof(buf), "Work so far (%.0f%% of the path): %.4f", li.upto() * 100, li.value().re);
            note(col, buf, t.text);
            snprintf(buf, sizeof(buf), "Whole path: %.4f", li.total());
            note(col, buf, t.textMuted);
        }
        note(col, "Green where F pushes along the motion, red where it pushes against it. Up to: 0 to 1, e.g. a "
                  "parameter or mod(t/4, 1) to travel the path.",
             t.textFaint);
    }
}

void ScenePanel::inspectQuaternion(InspectorContext& c) {
    ui::Column& col = c.col;
    SceneObject& o = c.o;
    scene::Scene& sc = c.sc;
    const ui::Theme& t = ui::theme();
    auto commit = [&sc]() { sc.commit(); };
    auto& qo = static_cast<scene::QuaternionObject&>(o);
    col.setY(fieldRow(col.x(), col.y(), col.width(), qo, "value", sc, 34.0f));
    ui::anchor("quaternion.value", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    expr::Rotation r;
    if (qo.error().empty() && expr::rotationOf(qo.value(), r)) {
        note(col, "= " + qo.value().summary(), t.textMuted);
        char buf[128];
        if (r.hasAxis)
            snprintf(buf, sizeof(buf), "%.1f° about (%.2f, %.2f, %.2f)", r.angle * 180.0 / 3.14159265358979,
                     r.axis.x, r.axis.y, r.axis.z);
        else
            snprintf(buf, sizeof(buf), "No rotation (angle 0)");
        note(col, buf, t.text);
        if (std::fabs(r.norm - 1.0) > 1e-3) {
            snprintf(buf, sizeof(buf), "|q| = %.3f, not 1: drawn as the rotation of q/|q|. unit(q) makes it 1.", r.norm);
            note(col, buf, t.warning);
        } else {
            note(col, "|q| = 1", t.textFaint);
        }
    } else if (qo.error().empty() && qo.value().type == expr::Type::Quaternion) {
        note(col, "The zero quaternion is not a rotation.", t.warning);
    }
    toggleProp(c, "Axis and angle", "axis", qo.showAxis(),
               "The rotation axis through the origin, and an arc showing how far it turns (right-hand rule).");
    toggleProp(c, "Rotated basis", "basis", qo.showBasis(),
               "Where the rotation sends the x, y and z axes (the faint ones are the originals).");
    bool acts = toggleProp(c, "Acts on a vector", "act", qo.actsOnVector(),
                           "Shows a vector v (gray) and the turned vector q v q⁻¹, with the circle its tip follows.");
    ui::anchor("quaternion.act", {col.x(), col.y() - ui::px(34), col.width(), ui::px(30)});
    if (acts) col.setY(fieldRow(col.x(), col.y(), col.width(), qo, "v", sc, 64.0f));
    if (ui::button(col.row(30), "Next example", ui::ButtonKind::Secondary, true, ui::Icon::Gallery)) {
        m_activeId = -1;
        static const char* kQuats[] = {
            "qaxis([0, 0, 1], pi/2)",      // a quarter turn about z
            "qaxis([1, 0, 0], pi)",        // a half turn about x
            "qaxis([1, 1, 1], 2pi/3)",     // 120° about the diagonal: x -> y -> z
            "quat(1, 0, 0, 0)",            // no rotation
            "quat(1, 1, 0, 0)",            // not unit: still a rotation (90° about x)
            "qaxis([0, 0, 1], t)",         // turning with the clock
        };
        int n = static_cast<int>(sizeof(kQuats) / sizeof(kQuats[0]));
        m_exampleIndex = (m_exampleIndex + 1) % n;
        qo.setField("value", kQuats[m_exampleIndex]);
        commit();
    }
    note(col, "qaxis(axis, angle) makes a rotation; q1*q2 does q2 first, then q1. The formula reference (F2) has "
              "the rest.",
         t.textFaint);
}

void ScenePanel::inspectField(InspectorContext& c) {
    ui::Column& col = c.col;
    SceneObject& o = c.o;
    scene::Scene& sc = c.sc;
    const view::SceneView& view = c.view;
    const ui::Theme& t = ui::theme();
    auto commit = [&sc]() { sc.commit(); };
    auto& fo = static_cast<scene::VectorFieldObject&>(o);
    col.setY(fieldRow(col.x(), col.y(), col.width(), fo, "F", sc, 92.0f));
    ui::anchor("field.formula", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});

    Rectangle domainRow = col.row(30.0f);
    int domain = fo.volume() ? 1 : 0;
    if (ui::segmented(domainRow, {"Plane", "Volume"}, domain,
                      {"On the floor z = 0: F's x and y components, e.g. [-y, x, 0].",
                       "A box of arrows in space: F(x, y, z) with all three components."})) {
        fo.setProp("domain", domain == 1 ? "volume" : "plane");
        commit();
    }
    ui::anchor("field.domain", domainRow);

    rangeRow(c, "xmin", "xmax", "x");
    rangeRow(c, "ymin", "ymax", "y");
    if (fo.volume()) rangeRow(c, "zmin", "zmax", "z");

    // Arrows per unit
    Rectangle row = col.row(30.0f);
    Rectangle label = ui::takeLeft(row, 52.0f, 8.0f);
    Rectangle value = ui::takeRight(row, 64.0f, 8.0f);
    ui::textIn(label, "Detail", ui::FontRole::Body, t.text);
    float density = static_cast<float>(fo.density());
    if (ui::slider(row, density, static_cast<float>(scene::VectorFieldObject::kDensityMin),
                   static_cast<float>(scene::VectorFieldObject::kDensityMax))) {
        fo.setProp("density", std::round(static_cast<double>(density) * 4.0) / 4.0);
        m_sliderDirty = true;
    }
    ui::anchor("field.detail", row);
    char dbuf[48];
    snprintf(dbuf, sizeof(dbuf), "%g/unit", fo.density());
    ui::textIn(value, dbuf, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
    ui::tooltip({label.x, row.y, value.x + value.width - label.x, row.height},
                "Arrows per unit along each axis. A wider range gets more arrows.", "Detail");
    scene::VectorFieldObject::Counts cn = fo.counts();
    char cbuf[96];
    if (fo.volume()) snprintf(cbuf, sizeof(cbuf), "%d × %d × %d arrows", cn.nx, cn.ny, cn.nz);
    else snprintf(cbuf, sizeof(cbuf), "%d × %d arrows", cn.nx, cn.ny);
    std::string counts = std::string(cbuf) + (cn.capped ? " (capped for speed)" : "");
    note(col, counts, cn.capped ? t.warning : t.textFaint);

    toggleProp(c, "Color by strength", "heatmap", fo.heatmap(),
               "Arrows colored by |F| with the heat map (blue weak, red strong); off: the object's color.");
    toggleProp(c, "Same length", "same_length", fo.sameLength(),
               "All arrows one length: only the direction shows (a direction field). The color still shows |F|.");
    bool lines = toggleProp(c, "Flow lines", "lines", fo.lines(),
                            "Curves that follow F everywhere (streamlines), with an arrowhead showing the way.");
    ui::anchor("field.lines", {col.x(), col.y() - ui::px(34), col.width(), ui::px(30)});
    bool particles = toggleProp(c, "Particles", "particles", fo.particles(),
                                "Dots drifting along the flow lines as the scene clock plays (press P).");
    ui::anchor("field.particles", {col.x(), col.y() - ui::px(34), col.width(), ui::px(30)});
    if (lines || particles) {
        Rectangle sr = col.row(30.0f);
        Rectangle sl = ui::takeLeft(sr, 92.0f, 8.0f);
        Rectangle sv = ui::takeRight(sr, 40.0f, 8.0f);
        ui::textIn(sl, "Lines per axis", ui::FontRole::Body, t.text);
        float seeds = static_cast<float>(fo.seeds());
        if (ui::slider(sr, seeds, 2.0f, fo.volume() ? 4.0f : 16.0f)) {
            fo.setProp("seeds", std::round(static_cast<double>(seeds)));
            m_sliderDirty = true;
        }
        char sb[16];
        snprintf(sb, sizeof(sb), "%d", field::seedsPerAxis(!fo.volume(), fo.seeds()));
        ui::textIn(sv, sb, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
        ui::tooltip({sl.x, sr.y, sv.x + sv.width - sl.x, sr.height},
                    "How many flow lines start along each axis (evenly spaced).", "Lines per axis");
    }
    if (particles) {
        Rectangle pr = col.row(30.0f);
        Rectangle pl = ui::takeLeft(pr, 92.0f, 8.0f);
        Rectangle pv = ui::takeRight(pr, 40.0f, 8.0f);
        ui::textIn(pl, "Speed", ui::FontRole::Body, t.text);
        float speed = static_cast<float>(fo.speed());
        if (ui::slider(pr, speed, 0.1f, 5.0f)) {
            fo.setProp("speed", std::round(static_cast<double>(speed) * 10.0) / 10.0);
            m_sliderDirty = true;
        }
        char pb[16];
        snprintf(pb, sizeof(pb), "%.1f×", fo.speed());
        ui::textIn(pv, pb, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
        ui::tooltip({pl.x, pr.y, pv.x + pv.width - pl.x, pr.height},
                    "How fast particles move: 1× is the field's own speed.", "Speed");
    }

    if (ui::button(col.row(30), "Next example", ui::ButtonKind::Secondary, true, ui::Icon::Gallery)) {
        m_activeId = -1;
        struct FieldExample {
            const char* f;
            bool volume;
        };
        static const FieldExample kFields[] = {
            {"[-y, x, 0]", false},                        // rotation
            {"[x, y, 0]", false},                         // source
            {"[x, -y, 0]", false},                        // saddle
            {"[-y - x/4, x - y/4, 0]", false},            // vortex sink
            {"[sin(y), sin(x), 0]", false},               // cells
            {"[-y, x, 0.4]", true},                       // helix
            {"[-y, x, -z]/2 + [0, 0, sin(x)]/3", true},   // swirl
        };
        int n = static_cast<int>(sizeof(kFields) / sizeof(kFields[0]));
        m_exampleIndex = (m_exampleIndex + 1) % n;
        fo.setField("F", kFields[m_exampleIndex].f);
        fo.setProp("domain", kFields[m_exampleIndex].volume ? "volume" : "plane");
        commit();
    }
    const FieldRenderer* fr = view.fieldRenderer(fo.id());
    if (!fo.error().empty()) note(col, fo.error(), t.danger);
    else if (fr && !fr->error().empty()) note(col, fr->error(), t.danger);
    else if (fr && fr->arrowCount() > 0) {
        char buf[96];
        snprintf(buf, sizeof(buf), "|F| ranges over [%.2f, %.2f]", fr->minMagnitude(), fr->maxMagnitude());
        note(col, buf, t.textMuted);
    }
    note(col, "F is a vector formula of x, y, z (and t, the scene time); parameters work by name, e.g. "
              "k*[-y, x, 0].",
         t.textFaint);
}

void ScenePanel::inspectParameter(InspectorContext& c) {
    ui::Column& col = c.col;
    SceneObject& o = c.o;
    scene::Scene& sc = c.sc;
    SpaceKind kind = c.kind;
    scene::ObjectId oid = c.o.id();
    const ui::Theme& t = ui::theme();
    auto commit = [&sc]() { sc.commit(); };
    auto& p = static_cast<scene::ParameterObject&>(o);
    using PMode = scene::ParameterObject::Mode;
    Rectangle modeRow = col.row(30.0f);
    int m = static_cast<int>(p.mode());
    if (ui::segmented(modeRow, {"Slider", "Animate", "Formula"}, m,
                      {"Set the value by hand. It stays where you put it.",
                       "Follows the scene clock from min to max, with a repeat style and an easing curve.",
                       "A formula of t, the scene time in seconds, e.g. 2pi*t/5 or sin(t)."})) {
        p.setMode(static_cast<PMode>(m));
        commit();
    }
    ui::anchor("parameter.mode", modeRow);

    if (p.mode() == PMode::Formula) {
        col.setY(fieldRow(col.x(), col.y(), col.width(), o, "formula", sc, 64.0f));
        ui::anchor("parameter.formula", {col.x(), col.y() - ui::px(40), col.width(), ui::px(36)});
    }
    if (p.mode() == PMode::Slider) {
        Rectangle row = col.row(30.0f);
        Rectangle value = ui::takeRight(row, 56.0f, 8.0f);
        float v = static_cast<float>(p.number());
        if (ui::slider(row, v, static_cast<float>(p.minimum()), static_cast<float>(p.maximum()))) {
            p.setNumber(v);
            m_sliderDirty = true;
        }
        ui::anchor("parameter.slider", row);
        ui::textIn(value, fmt(p.number()).c_str(), ui::FontRole::Mono, t.text, ui::Align::Right);
    }
    if (p.mode() != PMode::Formula) {
        Rectangle range = col.row(34.0f);
        Rectangle l = ui::takeLeft(range, 52.0f, 6.0f);
        ui::textIn(l, "Range", ui::FontRole::Body, t.textMuted);
        auto cells = ui::splitRow(range, {1, 1}, 26.0f);
        const char* keys[2] = {"min", "max"};
        for (int k = 0; k < 2; k++) {
            std::string key = keys[k];
            editText(cells[k], fieldId(p, std::string("#") + key), fmt(p.propNumber(key)), false,
                     [&sc, kind, oid, key](const std::string& text) {
                         float val;
                         auto* ob = dynamic_cast<scene::ParameterObject*>(sc.space(kind).find(oid));
                         if (!ob || !parseNumber(text.c_str(), val)) return false;
                         ob->setProp(key, static_cast<double>(val));
                         ob->setNumber(ob->number());
                         sc.commit();
                         return true;
                     });
        }
        ui::textIn({cells[0].x + cells[0].width, range.y, ui::px(26), range.height}, "to", ui::FontRole::Small,
                   t.textMuted, ui::Align::Center);
    }

    if (p.mode() == PMode::Animate) {
        Playback pb = p.playback();
        bool changed = false;
        changed |= ui::repeatControl(col.row(30.0f), pb.repeat);
        Rectangle row = col.row(32.0f);
        Rectangle easeRect = ui::takeLeft(row, 150.0f, 10.0f);
        if (ui::easingButton(easeRect, pb.easing, m_easingOpen)) m_easingOpen = !m_easingOpen;
        ui::anchor("parameter.easing", easeRect);
        Rectangle durValue = ui::takeRight(row, 44.0f, 6.0f);
        float dur = pb.duration;
        if (ui::slider(row, dur, 0.5f, 30.0f)) {
            pb.duration = std::round(dur * 10.0f) / 10.0f;
            changed = true;
            m_sliderDirty = true;
        }
        ui::tooltip(row, "Seconds for one run from min to max.", "Duration");
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1fs", pb.duration);
        ui::textIn(durValue, buf, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
        if (m_easingOpen) {
            bool picked = false;
            float h = ui::easingGrid(col.x(), col.y(), col.width(), pb.easing, picked);
            col.space(h / ui::scale() + 6.0f);
            changed |= picked;
        }
        if (changed) {
            p.setPlayback(pb);
            if (!m_sliderDirty) commit();
        }
        // Start delay, so several parameters can run one after another
        Rectangle startRow = col.row(34.0f);
        Rectangle sl = ui::takeLeft(startRow, 64.0f, 6.0f);
        ui::textIn(sl, "Start at", ui::FontRole::Body, t.textMuted);
        Rectangle sf = ui::takeLeft(startRow, 80.0f, 6.0f);
        editText(sf, fieldId(p, "#start"), fmt(p.startDelay()), false, [&sc, kind, oid](const std::string& text) {
            float val;
            auto* ob = dynamic_cast<scene::ParameterObject*>(sc.space(kind).find(oid));
            if (!ob || !parseNumber(text.c_str(), val) || val < 0.0f) return false;
            ob->setProp("anim.start", static_cast<double>(val));
            sc.commit();
            return true;
        });
        ui::tooltip(sf, "Seconds of scene time before this parameter starts moving.", "Start delay");
        ui::textIn(startRow, "s", ui::FontRole::Small, t.textMuted);
    }

    if (p.timeDriven()) {
        // Current value and the link to the clock that drives it
        const scene::Timeline& tl = sc.timeline();
        char buf[96];
        snprintf(buf, sizeof(buf), "t = %.2f s     %s = %s", tl.time, p.name().c_str(), fmt(p.number()).c_str());
        ui::textIn(col.row(18.0f), buf, ui::FontRole::Mono, t.textMuted);
        Rectangle link = col.row(30.0f);
        if (ui::button(link, tl.playing ? "Follows the Time panel (playing)" : "Follows the Time panel (paused)",
                       ui::ButtonKind::Secondary, true, tl.playing ? ui::Icon::Pause : ui::Icon::Play)) {
            m_requests.openTime = true;
        }
        ui::anchor("parameter.time", link);
        ui::tooltip(link, "Time-driven parameters follow the scene clock. Play, pause and scrub it in the Time "
                          "panel (P plays or pauses).", "Scene clock");
    }
    note(col, "Use " + p.name() + " in any formula, e.g. rotz(" + p.name() + ") or lerp(eye(), M, " + p.name() +
                  ").",
         t.textFaint);
}

void ScenePanel::inspectComplex(InspectorContext& c) {
    ui::Column& col = c.col;
    SceneObject& o = c.o;
    scene::Scene& sc = c.sc;
    const ui::Theme& t = ui::theme();
    auto commit = [&sc]() { sc.commit(); };
    col.setY(fieldRow(col.x(), col.y(), col.width(), o, "value", sc, 52.0f));
    const expr::Value& v = o.value();
    if (v.isNumber()) {
        double r = std::hypot(v.re, v.im), a = std::atan2(v.im, v.re) * 180.0 / kPi;
        if (std::fabs(a) < 0.05) a = 0.0;  // no "-0.0°"
        char buf[96];
        snprintf(buf, sizeof(buf), "|%s| = %.3f   arg = %.1f°", o.name().c_str(), r, a);
        ui::textIn(col.row(18), buf, ui::FontRole::Mono, t.textMuted);
    }
    struct Toggle { const char* label; const char* key; const char* tip; };
    const Toggle toggles[] = {
        {"Circle through it", "circle", "The circle of all numbers with the same length"},
        {"cos / sin projections", "projections", "Its shadows on the real and imaginary axes"},
        {"Helix of e^(it)", "helix", "The path r·e^(it) for t from 0 to its angle, rising along Z"},
    };
    for (const auto& tg : toggles) {
        Rectangle r = col.row(30.0f);
        bool on = o.propBool(tg.key);
        if (ui::toggleRow(r, tg.label, on)) {
            o.setProp(tg.key, on);
            commit();
        }
        ui::tooltip(r, tg.tip);
    }
}

void ScenePanel::inspectProduct(InspectorContext& c) {
    ui::Column& col = c.col;
    SceneObject& o = c.o;
    scene::Scene& sc = c.sc;
    const ui::Theme& t = ui::theme();
    auto commit = [&sc]() { sc.commit(); };
    col.setY(fieldRow(col.x(), col.y(), col.width(), o, "a", sc, 104.0f));
    col.setY(fieldRow(col.x(), col.y(), col.width(), o, "b", sc, 104.0f));
    const scene::Field* fa = o.field("a");
    const scene::Field* fb = o.field("b");
    if (o.value().isNumber() && fa->value.isNumber() && fb->value.isNumber()) {
        double ra = std::hypot(fa->value.re, fa->value.im), rb = std::hypot(fb->value.re, fb->value.im);
        double aa = std::atan2(fa->value.im, fa->value.re) * 180 / kPi, ab = std::atan2(fb->value.im, fb->value.re) * 180 / kPi;
        char buf[128];
        snprintf(buf, sizeof(buf), "length %.2f × %.2f = %.2f", ra, rb, ra * rb);
        ui::textIn(col.row(18), buf, ui::FontRole::Mono, t.textMuted);
        snprintf(buf, sizeof(buf), "angle %.1f° + %.1f° = %.1f°", aa, ab, aa + ab);
        ui::textIn(col.row(18), buf, ui::FontRole::Mono, t.textMuted);
    }
    bool angles = o.propBool("angles", true);
    if (ui::toggleRow(col.row(30), "Show rotation and scaling", angles)) {
        o.setProp("angles", angles);
        commit();
    }
}

void ScenePanel::inspectTangent(InspectorContext& c) {
    ui::Column& col = c.col;
    scene::Scene& sc = c.sc;
    const ui::Theme& t = ui::theme();
    auto& tg = static_cast<scene::TangentObject&>(c.o);
    col.setY(fieldRow(col.x(), col.y(), col.width(), tg, "f", sc, 52.0f));
    ui::anchor("tangent.f", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    col.setY(fieldRow(col.x(), col.y(), col.width(), tg, "x0", sc, 64.0f));
    ui::anchor("tangent.x0", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    col.setY(fieldRow(col.x(), col.y(), col.width(), tg, "h", sc, 64.0f));
    ui::anchor("tangent.h", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    rangeRow(c, "xmin", "xmax", "x");
    toggleProp(c, "Graph of f", "function", tg.showFunction(), "Draw y = f(x) itself.");
    toggleProp(c, "Graph of f'", "derivative", tg.showDerivative(),
               "Draw the slope of f at every x as a second curve: where f climbs it is above 0, where f has a "
               "peak or a valley it crosses 0.");
    ui::anchor("tangent.derivative", {col.x(), col.y() - ui::px(34), col.width(), ui::px(30)});
    toggleProp(c, "Slope triangle", "triangle", tg.showTriangle(),
               "One step right along the tangent climbs f'(x0): the slope read off the picture.");
    if (!tg.error().empty()) return;
    char buf[160];
    snprintf(buf, sizeof(buf), "f'(%.4g) = %.4f", tg.x0(), tg.slope());
    note(c.col, buf, t.text);
    snprintf(buf, sizeof(buf), "tangent: y = %.4g %s %.4g (x %s %.4g)", tg.y0(), tg.slope() < 0 ? "−" : "+",
             std::fabs(tg.slope()), tg.x0() < 0 ? "+" : "−", std::fabs(tg.x0()));
    note(c.col, buf, t.textMuted);
    if (tg.hasSecant()) {
        snprintf(buf, sizeof(buf), "secant slope %.4f (h = %.3g): off by %.2g", tg.secantSlope(), tg.secantH(),
                 std::fabs(tg.secantSlope() - tg.slope()));
        note(c.col, buf, t.textMuted);
    }
    if (tg.hasSecond()) {
        double f2 = tg.second();
        const char* shape = std::fabs(f2) < 1e-4 ? "f'' ≈ 0: a straight stretch or an inflection point"
                            : f2 > 0             ? "f'' > 0: f bends upward (like a cup) here"
                                                 : "f'' < 0: f bends downward (like a cap) here";
        snprintf(buf, sizeof(buf), "f''(x0) = %.3f   %s", std::fabs(f2) < 1e-4 ? 0.0 : f2, shape);
        note(c.col, buf, t.textMuted);
    }
    note(c.col, "The slope f' is the limit of secant slopes as h → 0. Put t or a slider in \"At x\" to slide "
                "the point.",
         t.textFaint);
}

void ScenePanel::inspectTangentPlane(InspectorContext& c) {
    ui::Column& col = c.col;
    scene::Scene& sc = c.sc;
    const ui::Theme& t = ui::theme();
    auto& g = static_cast<scene::TangentPlaneObject&>(c.o);
    col.setY(fieldRow(col.x(), col.y(), col.width(), g, "f", sc, 34.0f));
    ui::anchor("tplane.f", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    col.setY(fieldRow(col.x(), col.y(), col.width(), g, "x0", sc, 64.0f));
    col.setY(fieldRow(col.x(), col.y(), col.width(), g, "y0", sc, 64.0f));
    ui::anchor("tplane.point", {col.x(), col.y() - ui::px(80), col.width(), ui::px(74)});
    rangeRow(c, "xmin", "xmax", "x");
    rangeRow(c, "ymin", "ymax", "y");
    toggleProp(c, "Surface", "surface", g.showSurface(), "Draw z = f(x, y) as a wire mesh.");
    toggleProp(c, "Tangent plane", "plane", g.showPlane(),
               "The flat plane that touches the surface at the point: near it, f looks like this plane.");
    toggleProp(c, "Slopes along x and y", "slopes", g.showSlopes(),
               "One step in x climbs ∂f/∂x (red), one step in y climbs ∂f/∂y (green): the partial derivatives.");
    toggleProp(c, "Gradient", "gradient", g.showGradient(),
               "The arrow (∂f/∂x, ∂f/∂y) on the floor: the direction of steepest climb, as long as the climb rate.");
    ui::anchor("tplane.gradient", {col.x(), col.y() - ui::px(34), col.width(), ui::px(30)});
    toggleProp(c, "Normal", "normal", g.showNormal(), "The unit vector at right angles to the plane.");
    if (!g.error().empty()) return;
    char buf[160];
    snprintf(buf, sizeof(buf), "∂f/∂x = %.4f    ∂f/∂y = %.4f", g.fx(), g.fy());
    note(c.col, buf, t.text);
    double steep = std::hypot(g.fx(), g.fy());
    if (steep > 1e-6) {
        double deg = std::atan2(g.fy(), g.fx()) * 180.0 / panel::kPi;
        snprintf(buf, sizeof(buf), "steepest climb %.3f per unit, toward %.0f° from the x axis", steep, deg);
    } else {
        snprintf(buf, sizeof(buf), "flat here: a peak, a valley or a saddle point (gradient 0)");
    }
    note(c.col, buf, t.textMuted);
    auto p = g.point();
    snprintf(buf, sizeof(buf), "plane: z = %.3g %s %.3g(x − %.3g) %s %.3g(y − %.3g)", p.z, g.fx() < 0 ? "−" : "+",
             std::fabs(g.fx()), p.x, g.fy() < 0 ? "−" : "+", std::fabs(g.fy()), p.y);
    note(c.col, buf, t.textMuted);
    note(c.col, "The gradient is the value: other objects can use it by name, e.g. a vector whose formula is "
                "the name of this object.",
         t.textFaint);
}

void ScenePanel::inspectVelocity(InspectorContext& c) {
    ui::Column& col = c.col;
    scene::Scene& sc = c.sc;
    const ui::Theme& t = ui::theme();
    auto& r = static_cast<scene::VelocityObject&>(c.o);
    for (const char* k : {"x", "y", "z"}) col.setY(fieldRow(col.x(), col.y(), col.width(), r, k, sc, 64.0f));
    ui::anchor("velocity.curve", {col.x(), col.y() - ui::px(120), col.width(), ui::px(116)});
    rangeRow(c, "umin", "umax", "u");
    col.setY(fieldRow(col.x(), col.y(), col.width(), r, "u0", sc, 64.0f));
    ui::anchor("velocity.u0", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    toggleProp(c, "Velocity r'", "velocity", r.showVelocity(), "Where the point is heading, as long as its speed.");
    toggleProp(c, "Acceleration r''", "acceleration", r.showAcceleration(), "How the velocity is changing.");
    toggleProp(c, "Split acceleration", "split", r.splitAcceleration(),
               "The part along the motion changes the speed; the part across it changes the direction (turning).");
    ui::anchor("velocity.split", {col.x(), col.y() - ui::px(34), col.width(), ui::px(30)});
    toggleProp(c, "Curve", "curve", r.showCurve(), "Draw the curve itself.");
    Rectangle row = col.row(30);
    Rectangle label = ui::takeLeft(row, 92.0f, 8.0f);
    Rectangle value = ui::takeRight(row, 40.0f, 8.0f);
    ui::textIn(label, "Arrow scale", ui::FontRole::Body, t.text);
    float scale = static_cast<float>(r.arrowScale());
    if (ui::slider(row, scale, 0.1f, 2.0f)) {
        r.setProp("scale", std::round(static_cast<double>(scale) * 20.0) / 20.0);
        m_sliderDirty = true;
    }
    char buf[160];
    snprintf(buf, sizeof(buf), "%.2f×", r.arrowScale());
    ui::textIn(value, buf, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
    ui::tooltip({label.x, row.y, value.x + value.width - label.x, row.height},
                "Draw the arrows shorter or longer (the values don't change).", "Arrow scale");
    if (!r.error().empty()) return;
    auto v = r.velocity();
    double speed = geometry::length(v), at = geometry::length(r.tangential()), an = geometry::length(r.normal());
    snprintf(buf, sizeof(buf), "speed |r'| = %.4f    |r''| = %.4f", speed, geometry::length(r.acceleration()));
    note(c.col, buf, t.text);
    double along = speed > 1e-12 ? geometry::dot(r.acceleration(), v) / speed : 0.0;
    double accel = geometry::length(r.acceleration());
    if (std::fabs(along) < 1e-4 * std::max(1.0, accel))
        snprintf(buf, sizeof(buf), "constant speed here    turning %.3f", an);
    else
        snprintf(buf, sizeof(buf), "%s %.3f    turning %.3f", along > 0 ? "speeding up" : "slowing down", at, an);
    note(c.col, buf, t.textMuted);
    note(c.col, "Constant speed on a circle: all the acceleration turns (it points to the center). Use t in "
                "\"At u\" to ride along.",
         t.textFaint);
}

namespace {
// "0.841 + 0.54(x − 1) − 0.421(x − 1)² …": the first terms of a Taylor polynomial
std::string polynomialText(const taylor::Series& s, int maxTerms) {
    static const char* kPowers[] = {"", "", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹"};
    std::string base = std::fabs(s.center) < 1e-12 ? std::string("x")
                                                   : "(x " + std::string(s.center < 0 ? "+ " : "− ") +
                                                         panel::fmt(std::fabs(s.center)) + ")";
    std::string out;
    int shown = 0;
    for (size_t k = 0; k < s.coeffs.size(); k++) {
        double c = s.coeffs[k];
        if (std::fabs(c) < 1e-10) continue;
        if (shown == maxTerms) {
            out += " + …";
            break;
        }
        std::string mag = panel::fmt(std::fabs(c));
        if (k > 0 && mag == "1") mag = "";
        out += shown == 0 ? (c < 0 ? "−" : "") : (c < 0 ? " − " : " + ");
        out += mag;
        if (k > 0) out += base + (k < 10 ? kPowers[k] : ("^" + std::to_string(k)).c_str());
        shown++;
    }
    return out.empty() ? "0" : out;
}
}  // namespace

void ScenePanel::inspectTaylor(InspectorContext& c) {
    ui::Column& col = c.col;
    scene::Scene& sc = c.sc;
    const ui::Theme& t = ui::theme();
    auto& tp = static_cast<scene::TaylorObject&>(c.o);
    col.setY(fieldRow(col.x(), col.y(), col.width(), tp, "f", sc, 52.0f));
    ui::anchor("taylor.f", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    col.setY(fieldRow(col.x(), col.y(), col.width(), tp, "a", sc, 64.0f));
    ui::anchor("taylor.a", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    col.setY(fieldRow(col.x(), col.y(), col.width(), tp, "N", sc, 64.0f));
    ui::anchor("taylor.N", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    rangeRow(c, "xmin", "xmax", "x");
    toggleProp(c, "Graph of f", "function", tp.showFunction(), "Draw f itself (gray) to compare.");
    toggleProp(c, "Previous degree", "previous", tp.showPrevious(),
               "Draw the polynomial one degree lower, faint: the difference is what the newest term adds.");
    toggleProp(c, "Separate terms", "terms", tp.showTerms(), "Draw each term c_k (x − a)^k on its own (the first 6).");
    ui::anchor("taylor.terms", {col.x(), col.y() - ui::px(34), col.width(), ui::px(30)});
    if (!tp.error().empty()) return;
    const taylor::Series& se = tp.series();
    char buf[160];
    snprintf(buf, sizeof(buf), "T%d(x) = %s", static_cast<int>(se.coeffs.size()) - 1, polynomialText(se, 4).c_str());
    note(c.col, buf, t.text);
    if (std::isfinite(tp.maxError())) {
        snprintf(buf, sizeof(buf), "largest gap to f on the range: %.3g", tp.maxError());
        note(c.col, buf, t.textMuted);
    }
    note(c.col, se.note, se.complexMethod && static_cast<int>(se.coeffs.size()) > se.requested ? t.textFaint : t.warning);
    note(c.col, "Each term matches one more derivative of f at the center: degree 1 is the tangent line. Far "
                "from the center, or past a singularity, the polynomial can't follow f.",
         t.textFaint);
}

void ScenePanel::inspectNewton(InspectorContext& c) {
    ui::Column& col = c.col;
    scene::Scene& sc = c.sc;
    const ui::Theme& t = ui::theme();
    auto& nw = static_cast<scene::NewtonObject&>(c.o);
    col.setY(fieldRow(col.x(), col.y(), col.width(), nw, "f", sc, 52.0f));
    ui::anchor("newton.f", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    col.setY(fieldRow(col.x(), col.y(), col.width(), nw, "x0", sc, 72.0f));
    ui::anchor("newton.x0", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    col.setY(fieldRow(col.x(), col.y(), col.width(), nw, "steps", sc, 72.0f));
    ui::anchor("newton.steps", {col.x(), col.y() - ui::px(40), col.width(), ui::px(34)});
    rangeRow(c, "xmin", "xmax", "x");
    toggleProp(c, "Graph of f", "function", nw.showFunction(), "Draw y = f(x).");
    if (!nw.error().empty()) return;
    const auto& run = nw.run();
    char buf[160];
    size_t rows = std::min<size_t>(run.steps.size(), 12);
    for (size_t k = 0; k < rows; k++) {
        const auto& s = run.steps[k];
        if (std::isfinite(s.fx))
            snprintf(buf, sizeof(buf), "x%-2zu = %-12.8g f = %.3g", k, s.x, std::fabs(s.fx) < 1e-300 ? 0.0 : s.fx);
        else
            snprintf(buf, sizeof(buf), "x%-2zu = %-12.8g", k, s.x);
        note(c.col, buf, k + 1 == run.steps.size() ? t.text : t.textMuted, ui::FontRole::Mono);
    }
    if (run.steps.size() > rows) note(c.col, "…", t.textMuted);
    const char* why = nullptr;
    switch (run.stop) {
        case taylor::NewtonStop::Converged: why = "Converged: the steps stopped moving. Near a root the number of correct digits about doubles each step."; break;
        case taylor::NewtonStop::FlatTangent: why = "Stopped: the tangent is flat (f' = 0), so it never meets the x axis. Try another start."; break;
        case taylor::NewtonStop::Undefined: why = "Stopped: f is undefined where the step landed."; break;
        case taylor::NewtonStop::Diverging: why = "Diverging: each step overshoots farther. Start closer to the root."; break;
        case taylor::NewtonStop::Steps: why = "Each step follows the tangent at (x, f(x)) down to the x axis: x − f(x)/f'(x)."; break;
    }
    note(c.col, why, run.stop == taylor::NewtonStop::Steps || run.stop == taylor::NewtonStop::Converged ? t.textFaint : t.warning);
}

} // namespace linalg
