#pragma once

#include <functional>
#include <string>
#include <vector>

namespace linalg {

class AppApi;

// What a step's check can look at besides the app itself
struct TutorialContext {
    bool cameraModeUsed = false;  // the camera was moved (orbit drag/zoom) or fly mode entered during this step
};

struct TutorialStep {
    const char* title;
    const char* body;
    std::function<void(AppApi&)> setup;                               // runs when the step starts (optional)
    const char* task;                                              // nullptr: nothing to do, just read
    std::function<bool(AppApi&, const TutorialContext&)> check;      // task done? (optional)
    const char* hint;                                              // shown until the task is done
    const char* anchor;                                            // UI element to highlight (optional)
};

struct Lesson {
    const char* title;
    const char* summary;
    std::vector<TutorialStep> steps;
};

const std::vector<Lesson>& lessons();

// Do the task of a step the scripted way (edu/TutorialSolutions.cpp); false: no script for it. For lesson walks.
bool solveTutorialStep(AppApi& app, int lesson, int step);

// Where the user is in the tutorial, and the live check of the current task. Both front ends' tutorial panels
// show it; they call update() often (every frame or tick) while it is shown.
class TutorialRunner {
public:
    void startLesson(int lesson);  // its first step's setup runs at the next update()
    void goToStep(int step);
    void leave() { m_lesson = -1; }  // back to the lesson list
    // A new step's setup, or else the task check (once done, a task stays done while the user keeps experimenting).
    // The check waits for the update after the setup, so it sees the scene evaluated.
    void update(AppApi& app);

    bool inLesson() const { return m_lesson >= 0; }
    int lessonIndex() const { return m_lesson; }
    int stepIndex() const { return m_step; }
    const Lesson* lesson() const;
    const TutorialStep* step() const;
    bool setupPending() const { return m_needsSetup; }
    bool taskDone() const { return m_done; }
    bool stepDone() const;  // nothing to do, or the task is done (or skipped)
    std::string anchor() const;  // the UI element the step points at ("" = none)

    void skip() { m_done = true; }  // Next becomes possible
    void back();
    // Next, or Finish on the last step: then the lesson counts as finished, the list shows again and it returns true
    bool next();

    unsigned int completed() const { return m_completed; }  // a bit per finished lesson, by position
    void setCompleted(unsigned int bits) { m_completed = bits; }
    bool lessonCompleted(int lesson) const { return (m_completed >> lesson) & 1u; }

private:
    int m_lesson = -1;  // -1: the lesson list
    int m_step = 0;
    bool m_needsSetup = false;
    bool m_done = false;
    TutorialContext m_context;
    unsigned int m_completed = 0;
};

} // namespace linalg
