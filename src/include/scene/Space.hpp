#pragma once

#include "scene/SceneObject.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace linalg::scene {

// A container of objects that can refer to each other by name (one per tab:
// the 3D space and the complex plane). Evaluates objects in dependency order,
// reports circular references and missing names as object errors.
class Space : public expr::Resolver {
public:
    explicit Space(SpaceKind kind) : m_kind(kind) {}

    SpaceKind kind() const { return m_kind; }

    // Add an object; its name becomes `name` (or a free variant such as "a2").
    SceneObject* add(std::unique_ptr<SceneObject> object, const std::string& name);
    bool remove(ObjectId id);
    SceneObject* duplicate(ObjectId id);
    void clear();

    SceneObject* find(ObjectId id);
    const SceneObject* find(ObjectId id) const;
    SceneObject* find(const std::string& name);
    const SceneObject* find(const std::string& name) const;
    const std::vector<std::unique_ptr<SceneObject>>& objects() const { return m_objects; }
    // Move an object to a new position in the list
    void move(ObjectId id, int newIndex);

    // Rename; fails (with a reason) for invalid or taken names. References in
    // other objects' fields are rewritten to the new name.
    bool rename(ObjectId id, const std::string& name, std::string& error);
    // A free name based on `base` ("a" → "a", "a2", "a3", ...)
    std::string uniqueName(const std::string& base) const;

    // Evaluate everything in dependency order. Cheap enough to run every frame.
    void evaluate();
    std::uint64_t epoch() const { return m_epoch; }
    // Order of the last evaluation (objects in a cycle are left out)
    const std::vector<ObjectId>& evaluationOrder() const { return m_order; }

    // expr::Resolver: names of objects in this space
    bool lookup(const std::string& name, expr::Value& out) const override;

private:
    SpaceKind m_kind;
    std::vector<std::unique_ptr<SceneObject>> m_objects;
    ObjectId m_nextId = 1;
    std::uint64_t m_epoch = 0;
    std::vector<ObjectId> m_order;
};

// Replace whole-word occurrences of `from` with `to` in an expression text
std::string renameInText(const std::string& text, const std::string& from, const std::string& to);

} // namespace linalg::scene
