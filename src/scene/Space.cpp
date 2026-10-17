#include "scene/Space.hpp"
#include <algorithm>
#include <cctype>
#include <map>

namespace linalg::scene {

namespace {

bool isNameChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

} // namespace

std::string renameInText(const std::string& text, const std::string& from, const std::string& to) {
    std::string out;
    size_t i = 0;
    while (i < text.size()) {
        // A name starts here unless it continues a longer name (a number before
        // it, as in "2a", is implicit multiplication and still counts)
        bool boundaryBefore = true;
        if (i > 0) {
            if (text[i - 1] == '.') {
                boundaryBefore = false;  // member access such as v.x
            } else if (isNameChar(text[i - 1])) {
                size_t j = i;
                while (j > 0 && isNameChar(text[j - 1])) j--;
                bool allDigits = true;
                for (size_t k = j; k < i; k++)
                    if (!std::isdigit(static_cast<unsigned char>(text[k]))) allDigits = false;
                boundaryBefore = allDigits;
            }
        }
        if (boundaryBefore && text.compare(i, from.size(), from) == 0) {
            size_t end = i + from.size();
            bool boundaryAfter = end >= text.size() || !isNameChar(text[end]);
            // A number directly before ("2a") is implicit multiplication, so still a name
            if (boundaryAfter) {
                out += to;
                i = end;
                continue;
            }
        }
        out += text[i++];
    }
    return out;
}

SceneObject* Space::add(std::unique_ptr<SceneObject> object, const std::string& name) {
    object->m_id = m_nextId++;
    object->m_name = uniqueName(name.empty() ? std::string("obj") : name);
    object->m_dirty = true;
    m_objects.push_back(std::move(object));
    return m_objects.back().get();
}

bool Space::remove(ObjectId id) {
    auto it = std::find_if(m_objects.begin(), m_objects.end(), [id](const auto& o) { return o->id() == id; });
    if (it == m_objects.end()) return false;
    m_objects.erase(it);
    // Dependents re-evaluate and report the missing name
    for (auto& o : m_objects) o->m_dirty = true;
    return true;
}

SceneObject* Space::duplicate(ObjectId id) {
    SceneObject* src = find(id);
    if (!src) return nullptr;
    return add(src->clone(), src->name());
}

void Space::clear() {
    m_objects.clear();
    m_order.clear();
}

SceneObject* Space::find(ObjectId id) {
    for (auto& o : m_objects)
        if (o->id() == id) return o.get();
    return nullptr;
}

const SceneObject* Space::find(ObjectId id) const {
    for (const auto& o : m_objects)
        if (o->id() == id) return o.get();
    return nullptr;
}

SceneObject* Space::find(const std::string& name) {
    for (auto& o : m_objects)
        if (o->name() == name) return o.get();
    return nullptr;
}

const SceneObject* Space::find(const std::string& name) const {
    for (const auto& o : m_objects)
        if (o->name() == name) return o.get();
    return nullptr;
}

void Space::move(ObjectId id, int newIndex) {
    auto it = std::find_if(m_objects.begin(), m_objects.end(), [id](const auto& o) { return o->id() == id; });
    if (it == m_objects.end()) return;
    std::unique_ptr<SceneObject> obj = std::move(*it);
    m_objects.erase(it);
    newIndex = std::clamp(newIndex, 0, static_cast<int>(m_objects.size()));
    m_objects.insert(m_objects.begin() + newIndex, std::move(obj));
}

std::string Space::uniqueName(const std::string& base) const {
    std::string clean;
    for (char c : base)
        if (isNameChar(c)) clean += c;
    if (clean.empty() || std::isdigit(static_cast<unsigned char>(clean[0]))) clean = "obj" + clean;
    if (expr::isValidName(clean) && !find(clean)) return clean;
    // Strip a trailing number and count up from 2
    size_t end = clean.size();
    while (end > 1 && std::isdigit(static_cast<unsigned char>(clean[end - 1]))) end--;
    std::string stem = clean.substr(0, end);
    if (!expr::isValidName(stem + "2")) stem = "obj";
    for (int n = 2;; n++) {
        std::string candidate = stem + std::to_string(n);
        if (!find(candidate)) return candidate;
    }
}

bool Space::rename(ObjectId id, const std::string& name, std::string& error) {
    SceneObject* obj = find(id);
    if (!obj) {
        error = "no such object";
        return false;
    }
    if (name == obj->name()) return true;
    if (!expr::isValidName(name)) {
        error = expr::isReservedName(name) ? "'" + name + "' is a built-in name"
                                           : "names start with a letter and use letters, digits or _";
        return false;
    }
    if (find(name)) {
        error = "'" + name + "' is already used";
        return false;
    }
    std::string old = obj->name();
    obj->m_name = name;
    // Keep references working
    for (auto& o : m_objects) {
        for (auto& f : o->fields()) {
            if (f.program.valid() && f.program.names().count(old)) o->setField(f.key, renameInText(f.text, old, name));
        }
        o->m_dirty = true;
    }
    return true;
}

bool Space::lookup(const std::string& name, expr::Value& out) const {
    const SceneObject* o = find(name);
    if (!o) return false;
    if (!o->value().ok()) {
        if (o->value().error == "no value") out = expr::Value::fail(name + " cannot be used in formulas");
        else out = expr::Value::fail(name + " has an error");
        return true;
    }
    out = o->value();
    return true;
}

void Space::evaluate() {
    m_epoch++;
    m_order.clear();

    std::map<std::string, SceneObject*> byName;
    for (auto& o : m_objects) byName[o->name()] = o.get();

    // Kahn's algorithm over "depends on" edges between existing objects
    std::map<SceneObject*, int> indegree;
    std::map<SceneObject*, std::vector<SceneObject*>> dependents;
    for (auto& o : m_objects) {
        indegree[o.get()] = 0;
    }
    for (auto& o : m_objects) {
        for (const auto& dep : o->dependencies()) {
            auto it = byName.find(dep);
            if (it == byName.end() || it->second == o.get()) {
                if (it != byName.end()) indegree[o.get()]++;  // self reference: keeps it in the cycle set
                continue;
            }
            dependents[it->second].push_back(o.get());
            indegree[o.get()]++;
        }
    }

    std::vector<SceneObject*> queue;
    for (auto& o : m_objects)
        if (indegree[o.get()] == 0) queue.push_back(o.get());

    size_t head = 0;
    while (head < queue.size()) {
        SceneObject* o = queue[head++];
        // Inputs changed if any dependency changed during this pass
        bool inputsChanged = false;
        for (const auto& dep : o->dependencies()) {
            auto it = byName.find(dep);
            if (it != byName.end() && it->second->changedAt() == m_epoch) inputsChanged = true;
        }
        o->evaluate(*this, m_epoch, inputsChanged);
        m_order.push_back(o->id());
        for (SceneObject* d : dependents[o]) {
            if (--indegree[d] == 0) queue.push_back(d);
        }
    }

    // Whatever is left is part of (or depends on) a cycle
    for (auto& o : m_objects) {
        if (std::find(m_order.begin(), m_order.end(), o->id()) != m_order.end()) continue;
        std::string msg = "circular reference";
        for (const auto& dep : o->dependencies()) {
            auto it = byName.find(dep);
            if (it == byName.end()) continue;
            bool inCycle = std::find(m_order.begin(), m_order.end(), it->second->id()) == m_order.end();
            if (inCycle) {
                msg = "circular reference through " + dep;
                break;
            }
        }
        bool changed = o->m_error != msg;
        o->m_value = expr::Value::fail(msg);
        o->m_error = msg;
        if (changed) o->m_changedAt = m_epoch;
        o->m_dirty = false;
    }
}

} // namespace linalg::scene
