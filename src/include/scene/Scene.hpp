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

// The scene clock: one time for everything that moves (Animate and Formula parameters)
struct Timeline {
    double time = 0.0;     // seconds
    bool playing = false;
    float speed = 1.0f;    // 0.1x to 4x
    bool loop = true;      // at the end: start again (or stop)
    double length = 0.0;   // seconds; 0 = automatic (see Scene::timelineLength)
};

// Both spaces, their selections, the clock and an undo history of whole-scene snapshots
class Scene {
public:
    Scene();

    Space& space(SpaceKind kind) { return m_spaces[static_cast<int>(kind)]; }
    const Space& space(SpaceKind kind) const { return m_spaces[static_cast<int>(kind)]; }
    Selection& selection(SpaceKind kind) { return m_selections[static_cast<int>(kind)]; }

    void evaluate();

    // --- Time -------------------------------------------------------------------
    Timeline& timeline() { return m_timeline; }
    const Timeline& timeline() const { return m_timeline; }
    // Length used for the end of the timeline: the set length, or the longest
    // Once / Ping-pong animation (start delay included), or 10 s
    double timelineLength() const;
    double autoLength() const;
    void advance(double deltaSeconds);  // moves the clock while playing
    void togglePlay();                  // play or pause; at the end without loop, play starts from 0
    void setTime(double seconds);
    // Add an object of a registered type, named after the type (or `name`)
    SceneObject* add(SpaceKind kind, const std::string& typeId, const std::string& name = "");

    // --- Files ------------------------------------------------------------------
    // withTimeline: include the clock (files); undo snapshots leave it out
    void save(SceneData& data, bool withTimeline = true) const;
    // Loads v2 data, or converts a v1 file (one matrix, vectors A and B, ...)
    bool load(const SceneData& data, std::string& error);
    void resetToDefault();     // demo objects; the history baseline becomes this state
    void loadDefaultScene();   // demo objects as an undoable step

    // --- Undo -------------------------------------------------------------------
    // Record the current state as a step (call after each user change)
    void commit();
    bool canUndo() const { return !m_undo.empty(); }
    bool canRedo() const { return !m_redo.empty(); }
    void undo();
    void redo();
    // Forget the history and make the current state the starting point
    void clearHistory();
    // Remove every object from both spaces (one undo step when committed)
    void clearAll();

private:
    void restore(const SceneData& data);
    void loadV1(const SceneData& data);
    void convertOldPlots();

    Space m_spaces[kSpaceCount] = {Space(SpaceKind::Euclidean3D), Space(SpaceKind::Complex)};
    Selection m_selections[kSpaceCount];
    Timeline m_timeline;
    SceneData m_committed;
    std::vector<SceneData> m_undo, m_redo;
};

const char* spaceKey(SpaceKind kind);  // "3d" / "complex"

} // namespace linalg::scene
