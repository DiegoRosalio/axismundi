#include "scene/SceneObject.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace linalg::scene {

namespace {

bool sameValue(const expr::Value& a, const expr::Value& b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case expr::Type::Scalar:  return a.re == b.re;
        case expr::Type::Complex: return a.re == b.re && a.im == b.im;
        case expr::Type::Vector:  return a.vector.x == b.vector.x && a.vector.y == b.vector.y && a.vector.z == b.vector.z;
        case expr::Type::Matrix:
            if (a.size != b.size) return false;
            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                    if (a.matrix.m[i][j] != b.matrix.m[i][j]) return false;
            return true;
        case expr::Type::Error:   return a.error == b.error;
    }
    return false;
}

} // namespace

std::string checkExpect(Expect expect, const expr::Value& v) {
    if (!v.ok()) return v.error;
    switch (expect) {
        case Expect::Any:
            return "";
        case Expect::Number:
        case Expect::Plot:
            return v.isRealNumber() ? "" : "should be a real number, not " + v.typeName();
        case Expect::ComplexNum:
            return v.isNumber() ? "" : "should be a number, not " + v.typeName();
        case Expect::Vector:
            return v.type == expr::Type::Vector ? "" : "should be a vector, not " + v.typeName();
        case Expect::Matrix:
            return v.type == expr::Type::Matrix ? "" : "should be a matrix, not " + v.typeName();
    }
    return "";
}

SceneObject::SceneObject(std::string typeId) : m_typeId(std::move(typeId)) {
    m_value = expr::Value::fail("no value");
}

SceneObject::~SceneObject() = default;

void SceneObject::addField(const std::string& key, const std::string& label, Expect expect, const std::string& text) {
    Field f;
    f.key = key;
    f.label = label;
    f.expect = expect;
    m_fields.push_back(std::move(f));
    setField(key, text);
}

Field* SceneObject::field(const std::string& key) {
    for (auto& f : m_fields)
        if (f.key == key) return &f;
    return nullptr;
}

const Field* SceneObject::field(const std::string& key) const {
    for (const auto& f : m_fields)
        if (f.key == key) return &f;
    return nullptr;
}

bool SceneObject::setField(const std::string& key, const std::string& text) {
    Field* f = field(key);
    if (!f) return false;
    if (f->text == text && (f->program.valid() || !f->error.empty())) return f->program.valid();
    f->text = text;
    m_dirty = true;
    if (!f->program.compile(text)) {
        f->error = f->program.error();
        return false;
    }
    f->error.clear();
    return true;
}

bool SceneObject::fieldActive(const Field&) const {
    return true;
}

bool SceneObject::provideSlots(expr::Slots&) const {
    return false;
}

std::string SceneObject::prop(const std::string& key, const std::string& fallback) const {
    auto it = m_props.find(key);
    return it == m_props.end() ? fallback : it->second;
}

double SceneObject::propNumber(const std::string& key, double fallback) const {
    auto it = m_props.find(key);
    if (it == m_props.end()) return fallback;
    char* end = nullptr;
    double v = std::strtod(it->second.c_str(), &end);
    return (end == it->second.c_str() || !std::isfinite(v)) ? fallback : v;
}

bool SceneObject::propBool(const std::string& key, bool fallback) const {
    auto it = m_props.find(key);
    if (it == m_props.end()) return fallback;
    return it->second == "1" || it->second == "true";
}

void SceneObject::setProp(const std::string& key, const std::string& value) {
    auto it = m_props.find(key);
    if (it != m_props.end() && it->second == value) return;
    m_props[key] = value;
    m_dirty = true;
}

void SceneObject::removeProp(const std::string& key) {
    if (m_props.erase(key)) m_dirty = true;
}

void SceneObject::setProp(const std::string& key, double value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.9g", value);
    setProp(key, std::string(buf));
}

void SceneObject::setProp(const std::string& key, bool value) {
    setProp(key, std::string(value ? "1" : "0"));
}

std::set<std::string> SceneObject::dependencies() const {
    std::set<std::string> deps;
    for (const auto& f : m_fields) {
        if (!fieldActive(f) || !f.program.valid()) continue;
        deps.insert(f.program.names().begin(), f.program.names().end());
    }
    return deps;
}

void SceneObject::evaluate(const expr::Resolver& resolver, std::uint64_t epoch, bool inputsChanged) {
    std::string firstError;
    for (auto& f : m_fields) {
        if (!fieldActive(f)) continue;
        if (!f.program.valid()) {
            if (firstError.empty()) firstError = f.label + ": " + f.error;
            continue;
        }
        expr::Value v;
        if (f.expect == Expect::Plot) {
            // Check names and type with sample values of x, y, t
            expr::Slots slots;
            slots.x = 0.37;
            slots.y = 0.29;
            slots.t = 0.41;
            slots.hasX = slots.hasY = slots.hasT = true;
            v = f.program.eval(&resolver, &slots);
        } else {
            expr::Slots own;
            v = provideSlots(own) ? f.program.eval(&resolver, &own) : f.program.eval(&resolver);
        }
        f.error = checkExpect(f.expect, v);
        f.value = v;
        if (!f.error.empty() && firstError.empty()) firstError = f.label + ": " + f.error;
    }

    expr::Value next = firstError.empty() ? computeValue() : expr::Value::fail(firstError);
    // "no value" marks objects that are drawn but not used in formulas (plots): not an error
    if (firstError.empty() && !next.ok() && next.error != "no value") firstError = next.error;
    bool changed = m_dirty || inputsChanged || !sameValue(next, m_value) || firstError != m_error;
    m_value = next;
    m_error = firstError;
    if (changed) m_changedAt = epoch;
    m_dirty = false;
}

expr::Value SceneObject::computeValue() const {
    const Field* f = field("value");
    if (!f) return expr::Value::fail("no value");
    return f->value;
}

std::unique_ptr<SceneObject> SceneObject::clone() const {
    std::unique_ptr<SceneObject> copy = makeEmpty();
    for (const auto& f : m_fields) copy->setField(f.key, f.text);
    for (const auto& [k, v] : m_props) copy->setProp(k, v);
    copy->visible = visible;
    copy->customColor = customColor;
    copy->color = color;
    copy->paletteIndex = paletteIndex;
    return copy;
}

} // namespace linalg::scene
