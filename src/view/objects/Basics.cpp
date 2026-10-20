// The basic objects: vectors (arrows), matrices (the moved basis), eigenvectors, plots, vector fields
#include "view/ObjectViews.hpp"
#include "gfx/Canvas3D.hpp"
#include "math/Decompositions.hpp"
#include "rendering/VectorRenderer.hpp"
#include "rendering/WorldMapping.hpp"
#include "ui/core/Theme.hpp"
#include "view/SceneViewDraw.hpp"
#include "view/ViewStyle.hpp"
#include <raymath.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace linalg::view {

using scene::SceneObject;
using namespace draw;

namespace {

void drawVector(const SceneObject& o, const DrawContext& c) {
    const expr::Value& v = o.value();
    if (v.type != expr::Type::Vector) return;
    Vector3D tail = static_cast<const scene::VectorObject&>(o).tail();
    if (c.print) printArrow(mathToWorld(tail), mathToWorld(tail + v.vector), c.color, dashOf(c.printStyle), c.selected);
    else arrow(mathToWorld(tail), mathToWorld(tail + v.vector), c.color, c.selected);
}

void drawMatrix(const SceneObject& o, const DrawContext& c) {
    if (o.value().type != expr::Type::Matrix) return;
    Matrix4D m = SceneView::displayedMatrix(o, c.options);
    if (!c.print) {
        basis(m, 1.0f, c.selected, c.dim);
        return;
    }
    // Like the export: original basis dotted, image of the unit cube in light gray, moved basis
    const Color faintGray = {150, 150, 150, 255}, cubeGray = {185, 185, 185, 255};
    Vector3D origin = m.transform(Vector3D::zero());
    Vector3D cols[3] = {m.transformDirection({1, 0, 0}), m.transformDirection({0, 1, 0}), m.transformDirection({0, 0, 1})};
    for (int k = 0; k < 3; k++)
        printLine({0, 0, 0}, mathToWorld(k == 0 ? 1 : 0, k == 1 ? 1 : 0, k == 2 ? 1 : 0), faintGray, 2, 0.008f);
    auto corner = [&](int a, int b, int cc) { return mathToWorld(origin + cols[0] * a + cols[1] * b + cols[2] * cc); };
    for (int a = 0; a < 2; a++)
        for (int b = 0; b < 2; b++)
            for (int cc = 0; cc < 2; cc++) {
                if (a == 0) gfx::line(corner(a, b, cc), corner(1, b, cc), cubeGray);
                if (b == 0) gfx::line(corner(a, b, cc), corner(a, 1, cc), cubeGray);
                if (cc == 0) gfx::line(corner(a, b, cc), corner(a, b, 1), cubeGray);
            }
    int dash = dashOf(c.printStyle);
    for (int k = 0; k < 3; k++) printArrow(mathToWorld(origin), mathToWorld(origin + cols[k]), c.color, dash, c.selected);
}

void drawEigen(const SceneObject& o, const DrawContext& c) {
    const expr::Value& v = o.value();
    if (v.type != expr::Type::Matrix) return;
    EigenResult e = eigen3(v.matrix3());
    float len = c.options.view->axisLength;
    for (const auto& ev : e.values) {
        if (!ev.isReal() || !ev.hasVector) continue;
        if (c.print) {
            float l = std::fmax(2.0f, std::fabs(ev.re) * 1.3f);
            printLine(mathToWorld(ev.vector * -l), mathToWorld(ev.vector * l), c.color, 1, 0.008f);
            dot(mathToWorld(ev.vector * ev.re), 0.05f, c.color);
            continue;
        }
        dashed(mathToWorld(ev.vector * -len), mathToWorld(ev.vector * len), withAlpha(c.color, c.selected ? 255 : 180),
               0.25f);
        VectorRenderer::drawArrow(Vector3D::zero(), toVec(mathToWorld(ev.vector)), withAlpha(c.color, 210), 0.015f, 0.15f);
        dot(mathToWorld(ev.vector * ev.re), c.selected ? 0.09f : 0.07f, c.color);
    }
}

// One label per real eigenvalue, at λv (values with 2 decimals; Paper: 3 significant digits)
void eigenLabels(const SceneObject& o, bool print, std::vector<LabelSpot>& out) {
    EigenResult e = eigen3(o.value().matrix3());
    for (const auto& ev : e.values) {
        if (!ev.isReal() || !ev.hasVector) continue;
        char text[48];
        snprintf(text, sizeof(text), print ? "λ = %.3g" : "λ = %.2f", ev.re);
        out.push_back({mathToWorld(ev.vector * ev.re), text, {8.0f, 14.0f}});
    }
}

// Plots and fields are built once into GPU meshes (SceneView's caches); here they are only drawn
void drawPlot(const SceneObject& o, const DrawContext& c) {
    const PlotRenderer* r = c.view.plot(o.id());
    if (r && r->error().empty()) r->draw(c.dim ? 0.3f : 1.0f);
}

void drawField(const SceneObject& o, const DrawContext& c) {
    if (!o.error().empty()) return;
    const FieldRenderer* f = c.view.fieldRenderer(o.id());
    if (f && f->error().empty()) f->draw(o.time(), c.dim ? 0.3f : 1.0f);
}

} // namespace

void registerBasicViews(ObjectViewTable& t) {
    ObjectView vector;
    vector.palette = Palette::Vectors;
    vector.draw = drawVector;
    vector.anchor = [](const SceneObject& o, const DrawOptions&, Vector3& out) {
        if (o.value().type != expr::Type::Vector) return false;
        out = mathToWorld(static_cast<const scene::VectorObject&>(o).tail() + o.value().vector);
        return true;
    };
    vector.label = [](const SceneObject& o) -> std::string {
        char text[96];
        snprintf(text, sizeof(text), "%s  %.2f", o.name().c_str(), o.value().vector.magnitude());
        return text;
    };
    vector.pick = [](const SceneObject& o, const DrawOptions&, std::vector<Segment>& out) {
        Vector3D tail = static_cast<const scene::VectorObject&>(o).tail();
        out.push_back({mathToWorld(tail), mathToWorld(tail + o.value().vector)});
    };
    t["vector"] = vector;

    ObjectView matrix;
    matrix.draw = drawMatrix;
    matrix.anchor = [](const SceneObject& o, const DrawOptions& options, Vector3& out) {
        if (o.value().type != expr::Type::Matrix) return false;
        out = mathToWorld(SceneView::displayedMatrix(o, options).transform(Vector3D(1, 0, 0)));
        return true;
    };
    matrix.plainLabel = true;
    matrix.pick = [](const SceneObject& o, const DrawOptions& options, std::vector<Segment>& out) {
        Matrix4D m = SceneView::displayedMatrix(o, options);
        Vector3D origin = m.transform(Vector3D::zero());
        for (Vector3D e : {Vector3D(1, 0, 0), Vector3D(0, 1, 0), Vector3D(0, 0, 1)})
            out.push_back({mathToWorld(origin), mathToWorld(origin + m.transformDirection(e))});
    };
    t["matrix"] = matrix;

    ObjectView eigen;
    eigen.palette = Palette::Eigen;
    eigen.draw = drawEigen;
    eigen.labels = eigenLabels;
    t["eigen"] = eigen;

    ObjectView plot;
    plot.palette = Palette::Plots;
    plot.draw = drawPlot;
    for (const char* id : {"function", "curve", "surface", "implicit"}) t[id] = plot;

    ObjectView field;
    field.palette = Palette::Plots;
    field.draw = drawField;
    t["field"] = field;
}

} // namespace linalg::view
