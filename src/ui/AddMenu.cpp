#include "ui/AddMenu.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <algorithm>

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

std::vector<std::pair<std::string, AddChoice>> AddMenu::templates(scene::Scene& sc, scene::SpaceKind kind) {
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

bool AddMenu::draw(scene::Scene& sc, scene::SpaceKind kind, AddChoice& choice) {
    if (!m_open) return false;
    const ui::Theme& t = ui::theme();

    float w = ui::px(310.0f);
    float maxH = GetScreenHeight() - m_anchor.y - m_anchor.height - ui::px(20);
    float h = std::min(m_contentHeight + ui::px(16), maxH);
    m_bounds = {m_anchor.x + m_anchor.width - w, m_anchor.y + m_anchor.height + ui::px(6), w, h};
    if (m_bounds.x < ui::px(8)) m_bounds.x = ui::px(8);

    if (!m_justOpened && ui::mousePressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(GetMousePosition(), m_bounds)) {
        m_open = false;
        return false;
    }
    m_justOpened = false;

    ui::card(m_bounds);
    float maxScroll = std::max(0.0f, m_contentHeight + ui::px(16) - m_bounds.height);
    if (CheckCollisionPointRec(GetMousePosition(), m_bounds)) m_scroll -= GetMouseWheelMove() * ui::px(40);
    m_scroll = std::clamp(m_scroll, 0.0f, maxScroll);
    BeginScissorMode(static_cast<int>(m_bounds.x), static_cast<int>(m_bounds.y), static_cast<int>(m_bounds.width),
                     static_cast<int>(m_bounds.height));
    ui::setClip(m_bounds);

    float x = m_bounds.x + ui::px(8), width = m_bounds.width - ui::px(16);
    float y = m_bounds.y + ui::px(8) - m_scroll;
    float startY = y;
    bool picked = false;

    auto heading = [&](const char* text) {
        ui::textIn({x + ui::px(8), y, width, ui::px(26)}, ui::upper(text).c_str(), ui::FontRole::Caption, t.textMuted);
        y += ui::px(28);
    };
    auto entry = [&](const std::string& label, const char* description, const AddChoice& c) {
        float rowH = description ? ui::px(46) : ui::px(32);
        Rectangle row = {x, y, width, rowH};
        bool hover = ui::hovered(row);
        if (hover) ui::fillRounded(row, ui::px(ui::kRadius), t.hover);
        ui::text(label.c_str(), row.x + ui::px(10), row.y + ui::px(6), ui::FontRole::BodyStrong, t.text);
        if (description) {
            std::string d = ui::ellipsize(description, ui::FontRole::Small, width - ui::px(20));
            ui::text(d.c_str(), row.x + ui::px(10), row.y + ui::px(26), ui::FontRole::Small, t.textMuted);
        }
        if (hover && ui::mousePressed(MOUSE_BUTTON_LEFT)) {
            choice = c;
            picked = true;
        }
        y += rowH + ui::px(2);
    };

    auto tmpl = templates(sc, kind);
    if (!tmpl.empty()) {
        heading("From selection");
        for (const auto& [label, c] : tmpl) entry(label, nullptr, c);
        y += ui::px(6);
    }

    const char* lastCategory = "";
    for (const auto& type : scene::objectTypes()) {
        if (!(type.spaces & scene::spaceBit(kind))) continue;
        if (std::string(type.category) != lastCategory) {
            heading(type.category);
            lastCategory = type.category;
        }
        AddChoice c;
        c.typeId = type.id;
        entry(type.label, type.description, c);
    }
    if (tmpl.empty()) {
        y += ui::px(4);
        float th = ui::textWrapped({x + ui::px(8), y, width - ui::px(16), 0},
                                   "Tip: select objects first (Ctrl+click for several) to get templates such as "
                                   "a + b, cross(a, b) or M·v.",
                                   ui::FontRole::Small, t.textFaint);
        y += th + ui::px(4);
    }
    m_contentHeight = y - startY;

    ui::clearClip();
    EndScissorMode();
    if (picked) m_open = false;
    return picked;
}

} // namespace linalg
