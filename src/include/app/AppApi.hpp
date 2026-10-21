#pragma once

// AppApi: what the tutorial lessons and the gallery examples (edu/) may do to an app, whichever front end it is.
// Scene work goes to the shared Session; the few things that touch a front end's own UI (panels, the
// breakdown, toasts, the mouse mode) are virtual, and each front end implements them.

#include "app/Session.hpp"
#include <map>
#include <string>

namespace linalg {

class AppApi {
public:
    virtual ~AppApi() = default;

    virtual Session& session() = 0;
    const Session& session() const { return const_cast<AppApi*>(this)->session(); }

    // --- Spaces and objects -----------------------------------------------------------
    scene::Scene& scene() { return session().scene(); }
    const scene::Scene& scene() const { return session().scene(); }
    virtual AppMode mode() const { return session().mode(); }
    // Front ends close editors and popovers first, so these are virtual
    virtual void setMode(AppMode mode) = 0;
    virtual void clearSpace(AppMode mode) { session().clearSpace(mode); }
    virtual void resetScene() { session().resetScene(); }
    scene::Space& space(AppMode mode) { return session().space(mode); }
    scene::SceneObject* object(const std::string& name, AppMode mode = AppMode::Space3D) {
        return session().object(name, mode);
    }
    // Add an object with preset fields and properties; it becomes selected. One undo step.
    scene::SceneObject* addObject(AppMode mode, const std::string& typeId, const std::string& name,
                                  const std::map<std::string, std::string>& fields = {},
                                  const std::map<std::string, std::string>& props = {}) {
        return session().addObject(mode, typeId, name, fields, props);
    }
    void select(const std::string& name, AppMode mode = AppMode::Space3D, bool add = false) {
        session().select(name, mode, add);
    }
    void setMatrix3(const Matrix3D& m) { session().setMatrix3(m); }
    void setMatrix4(const Matrix4D& m) { session().setMatrix4(m); }
    void setVectors(const Vector3D& a, const Vector3D& b) { session().setVectors(a, b); }
    void applyMatrixPreset(const Matrix3D& m) { session().applyMatrixPreset(m); }

    // --- View and time ------------------------------------------------------------------
    void setCameraView(Vector3 position, Vector3 target) { session().setCameraView(position, target); }
    virtual void resetCamera() { session().resetCamera(); }
    void restartTime(bool play) { session().restartTime(play); }  // back to t = 0, then play or stay paused

    // --- Values (lesson checks) ---------------------------------------------------------------
    const scene::SceneObject* targetMatrix() const { return session().targetMatrix(); }
    Matrix4D matrix() const { return session().matrix(); }
    bool using4x4() const { return session().using4x4(); }
    Vector3D vectorValue(const std::string& name) const { return session().vectorValue(name); }
    Vector3D vectorA() const { return vectorValue("a"); }
    Vector3D vectorB() const { return vectorValue("b"); }
    Complex complexValue(const std::string& name) const { return session().complexValue(name); }
    double parameterValue(const std::string& name, AppMode mode) const { return session().parameterValue(name, mode); }
    EigenResult eigen() const { return session().eigen(); }

    // --- The front end's UI -----------------------------------------------------------------
    // Panels by id: "analysis", "animation" (the Time panel), "breakdown", "gallery", ...
    virtual void showPanel(const std::string& id, bool visible) = 0;
    virtual bool panelVisible(const std::string& id) const = 0;
    virtual bool uiMode() const = 0;                // the mouse is free (not captured by the fly camera)
    virtual bool cameraMovedThisFrame() const = 0;  // orbit drag, pan or zoom
    virtual void restartBreakdown() = 0;            // the transformation breakdown starts over
    virtual bool breakdownFinished() const = 0;     // and has played to its end
    virtual void toast(const std::string& message, bool error = false) = 0;
};

} // namespace linalg
