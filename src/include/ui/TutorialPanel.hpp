#pragma once

#include "edu/Tutorial.hpp"
#include "ui/SceneIO.hpp"
#include <raylib.h>
#include <string>

namespace linalg {

class App;

// Content of the "Tutorial" panel: the lesson list, then one step at a time
// with its task, checked live against the app.
class TutorialPanel {
public:
    // Per-frame logic: step setup and task checks
    void update(App& app);
    // Returns the content height in pixels
    float drawContent(Rectangle area, App& app);

    // Anchor id to highlight now (empty: none)
    std::string currentAnchor() const;
    bool inLesson() const { return m_lesson >= 0; }

    void startLesson(int lesson);
    void goToStep(int step) { m_step = step; m_needsSetup = true; }
    void saveState(SceneData& prefs) const;
    void loadState(const SceneData& prefs);

private:
    void enterStep(App& app);
    bool stepDone() const;

    int m_lesson = -1;   // -1: lesson list
    int m_step = 0;
    bool m_needsSetup = false;
    bool m_done = false;          // current task completed
    TutorialContext m_context;
    unsigned int m_completed = 0;  // bit per finished lesson
    std::string m_finishedMessage;
};

} // namespace linalg
