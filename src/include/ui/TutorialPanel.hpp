#pragma once

#include "edu/Tutorial.hpp"
#include "ui/SceneIO.hpp"
#include <raylib.h>
#include <string>

namespace linalg {

class App;

// Content of the "Tutorial" panel: the lesson list, then one step at a time
// with its task, checked live against the app (by edu/TutorialRunner).
class TutorialPanel {
public:
    // Per-frame logic: step setup and task checks
    void update(App& app);
    // Returns the content height in pixels
    float drawContent(Rectangle area, App& app);

    // Anchor id to highlight now (empty: none)
    std::string currentAnchor() const { return m_run.anchor(); }
    bool inLesson() const { return m_run.inLesson(); }

    void startLesson(int lesson) { m_run.startLesson(lesson); }
    void goToStep(int step) { m_run.goToStep(step); }
    void saveState(SceneData& prefs) const;
    void loadState(const SceneData& prefs);

private:
    TutorialRunner m_run;  // the lesson, the step and the task check (shared with the GTK app)
    std::string m_finishedMessage;
};

} // namespace linalg
