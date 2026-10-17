#include "scene/Scene.hpp"
#include <algorithm>
#include <cstdio>
#include <string>

namespace linalg::scene {

namespace {

constexpr size_t kMaxHistory = 100;

std::string num(float v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6g", v);
    return buf;
}

} // namespace

const char* spaceKey(SpaceKind kind) {
    return kind == SpaceKind::Euclidean3D ? "3d" : "complex";
}

// --- Selection --------------------------------------------------------------------

bool Selection::contains(ObjectId id) const {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

void Selection::set(ObjectId id) {
    ids.clear();
    if (id != kNoObject) ids.push_back(id);
}

void Selection::toggle(ObjectId id) {
    if (contains(id)) remove(id);
    else ids.push_back(id);
}

void Selection::remove(ObjectId id) {
    ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
}

// --- Scene ------------------------------------------------------------------------

Scene::Scene() {
    resetToDefault();
}

void Scene::evaluate() {
    for (auto& s : m_spaces) {
        for (const auto& o : s.objects()) o->setTime(m_timeline.time);
        s.evaluate();
    }
}

double Scene::autoLength() const {
    double longest = 0.0;
    for (const auto& s : m_spaces) {
        for (const auto& o : s.objects()) {
            const auto* p = dynamic_cast<const ParameterObject*>(o.get());
            if (!p || p->mode() != ParameterObject::Mode::Animate) continue;
            Playback pb = p->playback();
            if (pb.repeat == Repeat::Loop) continue;  // loops have no end of their own
            double runs = pb.repeat == Repeat::PingPong ? 2.0 : 1.0;
            longest = std::max(longest, p->startDelay() + runs * pb.duration);
        }
    }
    return longest > 0.0 ? longest : 10.0;
}

double Scene::timelineLength() const {
    return m_timeline.length > 0.0 ? m_timeline.length : autoLength();
}

void Scene::setTime(double seconds) {
    m_timeline.time = std::clamp(seconds, 0.0, timelineLength());
}

void Scene::advance(double dt) {
    if (!m_timeline.playing) return;
    double end = timelineLength();
    double t = m_timeline.time + dt * m_timeline.speed;
    if (t >= end) {
        if (m_timeline.loop && end > 0.0) {
            t = std::fmod(t, end);
        } else {
            t = end;
            m_timeline.playing = false;
        }
    }
    m_timeline.time = t;
}

SceneObject* Scene::add(SpaceKind kind, const std::string& typeId, const std::string& name) {
    const ObjectTypeInfo* info = findType(typeId);
    if (!info || !(info->spaces & spaceBit(kind))) return nullptr;
    std::unique_ptr<SceneObject> obj = info->create();
    // Palette slot: how many objects of this type the space already has
    int count = 0;
    for (const auto& o : space(kind).objects())
        if (o->typeId() == typeId) count++;
    obj->paletteIndex = count;
    return space(kind).add(std::move(obj), name.empty() ? info->baseName : name);
}

void Scene::resetToDefault() {
    for (auto& s : m_spaces) s.clear();
    for (auto& s : m_selections) s.clear();

    // 3D space: the picture the app always started with
    SpaceKind d = SpaceKind::Euclidean3D;
    add(d, "matrix", "M");
    SceneObject* a = add(d, "vector", "a");
    SceneObject* b = add(d, "vector", "b");
    a->setField("x", "1");
    b->setField("x", "0");
    b->setField("y", "1");
    auto derived = [&](const char* name, const char* formula, int palette) {
        auto* v = static_cast<VectorObject*>(add(d, "vector", name));
        v->setByFormula(true);
        v->setField("value", formula);
        v->paletteIndex = palette;
        return v;
    };
    derived("s", "a + b", 2);
    derived("c", "cross(a, b)", 3);
    derived("Ma", "M*a", 4);
    derived("Mb", "M*b", 5);
    add(d, "eigen", "E");
    selection(d).set(space(d).find("M")->id());

    // Complex plane
    SpaceKind c = SpaceKind::Complex;
    add(c, "complex", "z1")->setField("value", "1 + i");
    add(c, "complex", "z2")->setField("value", "1 + 0.5i");
    add(c, "product", "w");
    auto* theta = static_cast<ParameterObject*>(add(c, "parameter", "theta"));
    theta->setNumber(1.0);
    SceneObject* e = add(c, "complex", "u");
    e->setField("value", "exp(i*theta)");
    e->setProp("circle", true);
    e->setProp("projections", true);
    e->setProp("helix", true);
    selection(c).set(space(c).find("z1")->id());

    evaluate();
    m_undo.clear();
    m_redo.clear();
    m_committed.clear();
    save(m_committed);
}

void Scene::save(SceneData& data, bool withTimeline) const {
    putInt(data, "scene.version", 2);
    if (withTimeline) {
        putFloat(data, "time.t", static_cast<float>(m_timeline.time));
        putBool(data, "time.playing", m_timeline.playing);
        putFloat(data, "time.speed", m_timeline.speed);
        putBool(data, "time.loop", m_timeline.loop);
        putFloat(data, "time.length", static_cast<float>(m_timeline.length));
    }
    for (int s = 0; s < kSpaceCount; s++) {
        const Space& sp = m_spaces[s];
        std::string base = std::string("space.") + spaceKey(sp.kind()) + ".";
        putInt(data, base + "count", static_cast<int>(sp.objects().size()));
        int i = 0;
        for (const auto& o : sp.objects()) {
            std::string p = base + "object." + std::to_string(i++) + ".";
            data[p + "type"] = o->typeId();
            data[p + "name"] = o->name();
            putBool(data, p + "visible", o->visible);
            putInt(data, p + "palette", o->paletteIndex);
            if (o->customColor) {
                char hex[16];
                snprintf(hex, sizeof(hex), "#%02x%02x%02x", o->color.r, o->color.g, o->color.b);
                data[p + "color"] = hex;
            }
            for (const auto& f : o->fields()) data[p + "field." + f.key] = f.text;
            for (const auto& [k, v] : o->props()) data[p + "prop." + k] = v;
        }
    }
}

bool Scene::load(const SceneData& data, std::string& error) {
    int version = 1;
    getInt(data, "scene.version", version);
    if (version > 2) {
        error = "scene file is from a newer version";
        return false;
    }
    if (version < 2) {
        loadV1(data);
        evaluate();
        convertOldPlots();
    } else {
        restore(data);
    }
    commit();
    return true;
}

void Scene::restore(const SceneData& data) {
    bool anyPlaying = false;
    for (int s = 0; s < kSpaceCount; s++) {
        Space& sp = m_spaces[s];
        // Keep the selection across undo/redo by name (ids change on reload)
        std::vector<std::string> selectedNames;
        for (ObjectId id : m_selections[s].ids)
            if (const SceneObject* o = sp.find(id)) selectedNames.push_back(o->name());
        sp.clear();
        m_selections[s].clear();
        std::string base = std::string("space.") + spaceKey(sp.kind()) + ".";
        int count = 0;
        getInt(data, base + "count", count);
        for (int i = 0; i < count; i++) {
            std::string p = base + "object." + std::to_string(i) + ".";
            std::string type, name;
            if (!getString(data, p + "type", type)) continue;
            std::unique_ptr<SceneObject> obj = createObject(type);
            if (!obj) continue;  // unknown type from a newer version: skip it
            getString(data, p + "name", name);
            getBool(data, p + "visible", obj->visible);
            getInt(data, p + "palette", obj->paletteIndex);
            std::string hex;
            unsigned r, g, b;
            if (getString(data, p + "color", hex) && std::sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b) == 3) {
                obj->customColor = true;
                obj->color = {static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b),
                              255};
            }
            std::string fieldPrefix = p + "field.", propPrefix = p + "prop.";
            for (auto it = data.lower_bound(p); it != data.end() && it->first.compare(0, p.size(), p) == 0; ++it) {
                if (it->first.compare(0, fieldPrefix.size(), fieldPrefix) == 0)
                    obj->setField(it->first.substr(fieldPrefix.size()), it->second);
                else if (it->first.compare(0, propPrefix.size(), propPrefix) == 0)
                    obj->setProp(it->first.substr(propPrefix.size()), it->second);
            }
            if (type == "parameter") {
                auto* p = static_cast<ParameterObject*>(obj.get());
                bool wasPlaying = p->propBool("playing", false);
                if (!data.count(propPrefix + "mode")) {
                    // Files from before parameter modes: speed (range widths per second) + loop
                    Playback pb = p->playback();
                    double speed = p->propNumber("speed", 0.15);
                    pb.duration = static_cast<float>(1.0 / std::max(0.01, speed));
                    pb.repeat = p->propBool("loop", true) ? Repeat::Loop : Repeat::Once;
                    p->setPlayback(pb);
                    p->setMode(wasPlaying ? ParameterObject::Mode::Animate : ParameterObject::Mode::Slider);
                }
                // Parameters used to have their own clocks; the scene clock replaces them
                if (wasPlaying && p->mode() != ParameterObject::Mode::Slider) anyPlaying = true;
                for (const char* old : {"playing", "clock", "period", "speed", "loop", "anim.phase", "anim.direction"})
                    p->removeProp(old);
            }
            sp.add(std::move(obj), name);
        }
        for (const auto& n : selectedNames)
            if (const SceneObject* o = sp.find(n)) m_selections[s].ids.push_back(o->id());
    }
    // The clock (files only; undo snapshots leave it out and keep the current one)
    if (data.count("time.t")) {
        float v = 0.0f;
        if (getFloat(data, "time.t", v)) m_timeline.time = std::max(0.0f, v);
        getBool(data, "time.playing", m_timeline.playing);
        if (getFloat(data, "time.speed", v)) m_timeline.speed = std::clamp(v, 0.1f, 4.0f);
        getBool(data, "time.loop", m_timeline.loop);
        if (getFloat(data, "time.length", v)) m_timeline.length = std::max(0.0f, v);
    } else if (anyPlaying) {
        m_timeline.playing = true;
    }
    evaluate();
    convertOldPlots();
}

void Scene::convertOldPlots() {
    bool any = false;
    for (auto& sp : m_spaces)
        for (const auto& o : sp.objects())
            if (auto* p = dynamic_cast<PlotObject*>(o.get()); p && p->props().count("resolution")) {
                p->convertOldResolution();
                any = true;
            }
    if (any) evaluate();
}

void Scene::loadV1(const SceneData& data) {
    resetToDefault();
    Space& d = space(SpaceKind::Euclidean3D);

    // Matrix
    auto* m = static_cast<MatrixObject*>(d.find("M"));
    bool use4x4 = false;
    getBool(data, "matrix.use4x4", use4x4);
    m->setSize(use4x4 ? 4 : 3);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::string text;
            std::string key = (use4x4 ? "matrix4." : "matrix3.") + std::to_string(i) + std::to_string(j);
            if ((use4x4 || (i < 3 && j < 3)) && getString(data, key, text)) m->setField(MatrixObject::cellKey(i, j), text);
        }
    }
    // Vectors A and B
    const char* axes[3] = {"x", "y", "z"};
    for (int k = 0; k < 3; k++) {
        std::string text;
        if (getString(data, "vector_a." + std::to_string(k), text)) d.find("a")->setField(axes[k], text);
        if (getString(data, "vector_b." + std::to_string(k), text)) d.find("b")->setField(axes[k], text);
    }

    // Plot (v1 had one, of a selectable type)
    int type = -1;
    if (getInt(data, "plot.type", type)) {
        static const char* types[4] = {"function", "curve", "surface", "implicit"};
        static const char* exprKeys[4] = {"plot.function2d", "", "plot.surface", "plot.implicit"};
        if (type >= 0 && type < 4) {
            SceneObject* p = add(SpaceKind::Euclidean3D, types[type]);
            std::string text;
            if (type == 1) {
                if (getString(data, "plot.param.x", text)) p->setField("x", text);
                if (getString(data, "plot.param.y", text)) p->setField("y", text);
                if (getString(data, "plot.param.z", text)) p->setField("z", text);
            } else if (getString(data, exprKeys[type], text)) {
                p->setField("f", text);
            }
            float v;
            const char* ranges[6] = {"x_min", "x_max", "y_min", "y_max", "t_min", "t_max"};
            const char* keys[6] = {"xmin", "xmax", "ymin", "ymax", "tmin", "tmax"};
            for (int r = 0; r < 6; r++)
                if (getFloat(data, std::string("plot.") + ranges[r], v) && p->field(keys[r])) p->setField(keys[r], num(v));
            if (getFloat(data, "plot.resolution", v)) p->setProp("resolution", static_cast<double>(v));
            bool heat;
            if (getBool(data, "plot.heatmap", heat)) p->setProp("heatmap", heat);
        }
    }

    // Complex plane
    Space& c = space(SpaceKind::Complex);
    float re, im;
    if (getFloat(data, "complex.z1.re", re) && getFloat(data, "complex.z1.im", im))
        c.find("z1")->setField("value", num(re) + " + " + num(im) + "i");
    if (getFloat(data, "complex.z2.re", re) && getFloat(data, "complex.z2.im", im))
        c.find("z2")->setField("value", num(re) + " + " + num(im) + "i");
    float theta;
    if (getFloat(data, "complex.theta", theta)) static_cast<ParameterObject*>(c.find("theta"))->setNumber(theta);
    evaluate();
}

// --- Undo -------------------------------------------------------------------------

void Scene::commit() {
    SceneData now;
    save(now, false);
    if (now == m_committed) return;
    m_undo.push_back(m_committed);
    if (m_undo.size() > kMaxHistory) m_undo.erase(m_undo.begin());
    m_redo.clear();
    m_committed = now;
}

void Scene::undo() {
    if (m_undo.empty()) return;
    m_redo.push_back(m_committed);
    m_committed = m_undo.back();
    m_undo.pop_back();
    restore(m_committed);
}

void Scene::redo() {
    if (m_redo.empty()) return;
    m_undo.push_back(m_committed);
    m_committed = m_redo.back();
    m_redo.pop_back();
    restore(m_committed);
}

} // namespace linalg::scene
