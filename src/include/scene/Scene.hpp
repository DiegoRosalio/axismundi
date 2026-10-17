#pragma once

#include "scene/Space.hpp"
#include "scene/Types.hpp"
#include "ui/SceneIO.hpp"
#include <vector>

namespace linalg::scene {

// Selected objects in one space; the last one is the primary (shown in the inspector)
struct Selection {
    std::vector<ObjectId> ids;

    bool empty() const { return ids.empty(); }
    ObjectId primary() const { return ids.empty() ? kNoObject : ids.back(); }
    bool contains(ObjectId id) const;
    void set(ObjectId id);       // select only this one
    void toggle(ObjectId id);    // add or remove (Ctrl+click)
    void remove(ObjectId id);
    void clear() { ids.clear(); }
};

// Both spaces, their selections and an undo history of whole-scene snapshots
class Scene {
public:
    Scene();

    Space& space(SpaceKind kind) { return m_spaces[static_cast<int>(kind)]; }
    const Space& space(SpaceKind kind) const { return m_spaces[static_cast<int>(kind)]; }
    Selection& selection(SpaceKind kind) { return m_selections[static_cast<int>(kind)]; }

    void evaluate();

    // Add an object of a registered type, named after the type (or `name`)
    SceneObject* add(SpaceKind kind, const std::string& typeId, const std::string& name = "");

    // --- Files ------------------------------------------------------------------
    void save(SceneData& data) const;
    // Loads v2 data, or converts a v1 file (one matrix, vectors A and B, ...)
    bool load(const SceneData& data, std::string& error);
    void resetToDefault();

    // --- Undo -------------------------------------------------------------------
    // Record the current state as a step (call after each user change)
    void commit();
    bool canUndo() const { return !m_undo.empty(); }
    bool canRedo() const { return !m_redo.empty(); }
    void undo();
    void redo();

private:
    void restore(const SceneData& data);
    void loadV1(const SceneData& data);

    Space m_spaces[kSpaceCount] = {Space(SpaceKind::Euclidean3D), Space(SpaceKind::Complex)};
    Selection m_selections[kSpaceCount];
    SceneData m_committed;
    std::vector<SceneData> m_undo, m_redo;
};

const char* spaceKey(SpaceKind kind);  // "3d" / "complex"

} // namespace linalg::scene
