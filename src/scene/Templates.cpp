#include "scene/Templates.hpp"

namespace linalg {

using scene::SceneObject;

namespace {

AddChoice vectorFormula(const std::string& name, const std::string& formula) {
    AddChoice c;
    c.typeId = "vector";
    c.name = name;
    c.props["mode"] = "formula";
    c.fields["value"] = formula;
    return c;
}

AddChoice matrixFormula(const std::string& name, const std::string& formula) {
    AddChoice c;
    c.typeId = "matrix";
    c.name = name;
    c.props["mode"] = "formula";
    c.fields["value"] = formula;
    return c;
}

AddChoice quaternionFormula(const std::string& formula) {
    AddChoice c;
    c.typeId = "quaternion";
    c.fields["value"] = formula;
    return c;
}

// lerp-based morph driven by a new slider s (0..1)
AddChoice morph(const std::string& typeId, const std::string& formula) {
    AddChoice c;
    c.typeId = typeId;
    if (typeId != "complex" && typeId != "quaternion") c.props["mode"] = "formula";
    c.fields["value"] = formula;
    c.helperParameter = "s";
    return c;
}

} // namespace

std::vector<std::pair<std::string, AddChoice>> addTemplates(scene::Scene& sc, scene::SpaceKind kind) {
    std::vector<std::pair<std::string, AddChoice>> out;
    scene::Space& space = sc.space(kind);
    std::vector<const SceneObject*> vectors, matrices, numbers, quats, surfaces, curves, fields, functions;
    for (scene::ObjectId id : sc.selection(kind).ids) {
        const SceneObject* o = space.find(id);
        if (!o) continue;
        if (o->typeId() == "vector") vectors.push_back(o);
        else if (o->typeId() == "matrix") matrices.push_back(o);
        else if (o->typeId() == "quaternion") quats.push_back(o);
        else if (o->typeId() == "surface") surfaces.push_back(o);
        else if (o->typeId() == "function") functions.push_back(o);
        else if (o->typeId() == "curve") curves.push_back(o);
        else if (o->typeId() == "field") fields.push_back(o);
        else if (o->typeId() == "complex" || o->typeId() == "product") numbers.push_back(o);
    }

    if (kind == scene::SpaceKind::Euclidean3D) {
        if (vectors.size() >= 2) {
            const std::string& a = vectors[0]->name();
            const std::string& b = vectors[1]->name();
            out.push_back({"Sum " + a + " + " + b, vectorFormula("", a + " + " + b)});
            out.push_back({"Difference " + a + " − " + b, vectorFormula("", a + " - " + b)});
            out.push_back({"Cross product " + a + " × " + b, vectorFormula("", "cross(" + a + ", " + b + ")")});
            out.push_back({"Projection of " + a + " on " + b,
                           vectorFormula("", "dot(" + a + ", " + b + ")/dot(" + b + ", " + b + ") * " + b)});
            out.push_back({"Blend " + a + " → " + b + " (slider)", morph("vector", "lerp(" + a + ", " + b + ", $s)")});
        }
        if (vectors.size() == 1) {
            const std::string& a = vectors[0]->name();
            out.push_back({"Unit vector along " + a, vectorFormula("", "normalize(" + a + ")")});
            out.push_back({"Opposite −" + a, vectorFormula("", "-" + a)});
            out.push_back({"Grow " + a + " from zero (slider)", morph("vector", "lerp(0, " + a + ", $s)")});
        }
        if (!matrices.empty() && !vectors.empty()) {
            const std::string& m = matrices[0]->name();
            for (const SceneObject* v : vectors)
                out.push_back({"Transform " + m + "·" + v->name(), vectorFormula(m + v->name(), m + "*" + v->name())});
        }
        if (matrices.size() >= 2) {
            const std::string& a = matrices[0]->name();
            const std::string& b = matrices[1]->name();
            out.push_back({"Product " + a + "·" + b, matrixFormula("", a + "*" + b)});
            out.push_back({"Blend " + a + " → " + b + " (slider)", morph("matrix", "lerp(" + a + ", " + b + ", $s)")});
        }
        auto text = [](const SceneObject* o, const char* key) {
            const scene::Field* f = o->field(key);
            return f ? f->text : std::string();
        };
        for (const SceneObject* fn : functions) {
            AddChoice c;
            c.typeId = "tangent";
            for (const char* k : {"f", "xmin", "xmax"}) c.fields[k] = text(fn, k);
            out.push_back({"Tangent line on " + fn->name() + " (slope f')", c});
            AddChoice tp = c;
            tp.typeId = "taylor";
            out.push_back({"Taylor polynomial of " + fn->name(), tp});
            AddChoice nw = c;
            nw.typeId = "newton";
            out.push_back({"Newton's method on " + fn->name() + " (a root)", nw});
        }
        for (const SceneObject* s : surfaces) {
            AddChoice c;
            c.typeId = "doubleintegral";
            for (const char* k : {"f", "xmin", "xmax", "ymin", "ymax"}) c.fields[k] = text(s, k);
            out.push_back({"Volume under " + s->name() + " (boxes)", c});
            AddChoice tp;
            tp.typeId = "tangentplane";
            for (const char* k : {"f", "xmin", "xmax", "ymin", "ymax"}) tp.fields[k] = text(s, k);
            tp.fields["x0"] = "(" + text(s, "xmin") + " + " + text(s, "xmax") + ")/2 + 0.5";
            tp.fields["y0"] = "(" + text(s, "ymin") + " + " + text(s, "ymax") + ")/2 + 0.5";
            out.push_back({"Tangent plane on " + s->name() + " (gradient)", tp});
        }
        for (const SceneObject* cv : curves) {
            AddChoice c;
            c.typeId = "frame";
            for (const char* k : {"x", "y", "z", "umin", "umax"}) c.fields[k] = text(cv, k);
            c.fields["u0"] = "(" + text(cv, "umin") + " + " + text(cv, "umax") + ")/2";
            out.push_back({"Moving frame on " + cv->name() + " (curvature)", c});
            AddChoice v = c;
            v.typeId = "velocity";
            out.push_back({"Velocity on " + cv->name() + " (r', r'')", v});
        }
        if (!curves.empty() && !fields.empty()) {
            AddChoice c;
            c.typeId = "lineintegral";
            c.fields["F"] = text(fields[0], "F");
            for (const char* k : {"x", "y", "z", "umin", "umax"}) c.fields[k] = text(curves[0], k);
            out.push_back({"Work of " + fields[0]->name() + " along " + curves[0]->name(), c});
        }
        if (quats.size() >= 2) {
            const std::string& a = quats[0]->name();
            const std::string& b = quats[1]->name();
            out.push_back({"Compose " + a + "·" + b + " (" + b + " first)", quaternionFormula(a + "*" + b)});
            out.push_back({"Compose " + b + "·" + a + " (" + a + " first)", quaternionFormula(b + "*" + a)});
            out.push_back({"Turn from " + a + " to " + b + " (slerp, slider)",
                           morph("quaternion", "slerp(" + a + ", " + b + ", $s)")});
        }
        if (!quats.empty() && !vectors.empty()) {
            const std::string& q = quats[0]->name();
            for (const SceneObject* v : vectors)
                out.push_back({"Rotate " + v->name() + " by " + q, vectorFormula("", "rotate(" + q + ", " + v->name() + ")")});
        }
        if (quats.size() == 1) {
            const std::string& q = quats[0]->name();
            out.push_back({"Matrix of " + q, matrixFormula("", "mat(" + q + ")")});
            out.push_back({"Inverse rotation " + q + "⁻¹", quaternionFormula("inv(" + q + ")")});
            out.push_back({"Turn " + q + " from no rotation (slider)", morph("quaternion", "slerp(quat(1, 0, 0, 0), " + q + ", $s)")});
        }
        if (matrices.size() == 1) {
            const std::string& m = matrices[0]->name();
            out.push_back({"Quaternion of " + m + " (rotations only)", quaternionFormula("quat(" + m + ")")});
            AddChoice e;
            e.typeId = "eigen";
            e.fields["of"] = m;
            out.push_back({"Eigenvectors of " + m, e});
            out.push_back({"Inverse " + m + "⁻¹", matrixFormula(m + "inv", "inv(" + m + ")")});
            out.push_back({"Transpose of " + m, matrixFormula(m + "T", "transpose(" + m + ")")});
            bool big = matrices[0]->propNumber("size", 3) == 4;
            out.push_back({"Morph from identity to " + m + " (slider)",
                           morph("matrix", std::string("lerp(eye(") + (big ? "4" : "3") + "), " + m + ", $s)")});
        }
    } else {
        if (numbers.size() >= 2) {
            const std::string& a = numbers[0]->name();
            const std::string& b = numbers[1]->name();
            AddChoice p;
            p.typeId = "product";
            p.fields["a"] = a;
            p.fields["b"] = b;
            out.push_back({"Product " + a + "·" + b, p});
            AddChoice s;
            s.typeId = "complex";
            s.fields["value"] = a + " + " + b;
            out.push_back({"Sum " + a + " + " + b, s});
            AddChoice q;
            q.typeId = "complex";
            q.fields["value"] = a + " / " + b;
            out.push_back({"Quotient " + a + " / " + b, q});
            out.push_back({"Blend " + a + " → " + b + " (slider)", morph("complex", "lerp(" + a + ", " + b + ", $s)")});
        }
        if (numbers.size() == 1) {
            const std::string& a = numbers[0]->name();
            AddChoice c;
            c.typeId = "complex";
            c.fields["value"] = "conj(" + a + ")";
            out.push_back({"Conjugate of " + a, c});
            AddChoice sq;
            sq.typeId = "complex";
            sq.fields["value"] = a + "^2";
            out.push_back({"Square " + a + "²", sq});
            out.push_back({"Grow " + a + " from zero (slider)", morph("complex", "lerp(0, " + a + ", $s)")});
        }
    }
    return out;
}

scene::SceneObject* applyAddChoice(scene::Scene& sc, scene::SpaceKind kind, const AddChoice& choice) {
    std::string helperName;
    if (!choice.helperParameter.empty()) {
        // A static slider from 0 to 1 that drives the new object
        SceneObject* h = sc.add(kind, "parameter", choice.helperParameter);
        if (!h) return nullptr;
        h->setProp("min", 0.0);
        h->setProp("max", 1.0);
        h->setProp("value", 0.5);
        helperName = h->name();
    }
    SceneObject* o = sc.add(kind, choice.typeId, choice.name);
    if (!o) return nullptr;
    for (const auto& [k, v] : choice.props) o->setProp(k, v);
    for (const auto& [k, v] : choice.fields) {
        std::string text = v;
        for (size_t at = text.find("$s"); !helperName.empty() && at != std::string::npos; at = text.find("$s", at))
            text.replace(at, 2, helperName);
        o->setField(k, text);
    }
    sc.selection(kind).set(o->id());
    sc.commit();
    return o;
}

const std::vector<PlotExample>& plotExamples(const std::string& type) {
    static const std::vector<PlotExample> function = {
        {"sin(x)", "", "", ""}, {"x^3/10 - x", "", "", ""}, {"3exp(-x^2/2)", "", "", ""}, {"tan(x)", "", "", ""}};
    static const std::vector<PlotExample> surface = {{"sin(sqrt(x^2 + y^2))", "", "", ""},
                                                     {"(x^2 - y^2)/5", "", "", ""},
                                                     {"3exp(-(x^2 + y^2)/4)", "", "", ""},
                                                     {"sin(x) cos(y)", "", "", ""}};
    static const std::vector<PlotExample> implicit = {{"x^2 + y^2 - 9", "", "", ""},
                                                      {"x^2/16 + y^2/4 - 1", "", "", ""},
                                                      {"y^2 - x^3 + 3x", "", "", ""},
                                                      {"(x^2 + y^2)^2 - 8(x^2 - y^2)", "", "", ""}};
    static const std::vector<PlotExample> curve = {{"", "2cos(u)", "2sin(u)", "u/4"},
                                                   {"", "sin(u) + 2sin(2u)", "cos(u) - 2cos(2u)", "-sin(3u)"},
                                                   {"", "3sin(2u)", "3sin(3u)", "0"},
                                                   {"", "2cos(u + t)", "2sin(u + t)", "sin(3u - 2t)"}};
    if (type == "surface") return surface;
    if (type == "implicit") return implicit;
    if (type == "curve") return curve;
    return function;
}

const std::vector<const char*>& epicycleExamples() {
    static const std::vector<const char*> shapes = {
        "(16sin(u)^3 + i*(13cos(u) - 5cos(2u) - 2cos(3u) - cos(4u)))/8",  // heart
        "2(cos(u) + i*sin(u))/max(abs(cos(u)), abs(sin(u)))",             // square
        "2.5exp(i*u)*(1 + 0.35cos(5u))",                                    // flower
        "2sin(u) + i*sin(2u)",                                              // figure eight
        "1.6exp(i*u) + 0.8exp(6i*u)",                                       // epitrochoid
        "2exp(i*u)",                                                        // a circle: one circle
    };
    return shapes;
}

const std::vector<const char*>& fourierExamples() {
    static const std::vector<const char*> fns = {
        "sign(x)",      // square wave
        "x",            // sawtooth
        "abs(x)",       // triangle
        "abs(sin(x))",  // rectified sine
        "exp(-x^2)",    // a bump
    };
    return fns;
}

} // namespace linalg
