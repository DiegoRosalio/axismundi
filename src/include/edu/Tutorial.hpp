#pragma once

#include <functional>
#include <vector>

namespace linalg {

class App;

// What a step's check can look at besides the app itself
struct TutorialContext {
    bool cameraModeUsed = false;  // the camera was moved (orbit drag/zoom) or fly mode entered during this step
};

struct TutorialStep {
    const char* title;
    const char* body;
    std::function<void(App&)> setup;                               // runs when the step starts (optional)
    const char* task;                                              // nullptr: nothing to do, just read
    std::function<bool(App&, const TutorialContext&)> check;      // task done? (optional)
    const char* hint;                                              // shown until the task is done
    const char* anchor;                                            // UI element to highlight (optional)
};

struct Lesson {
    const char* title;
    const char* summary;
    std::vector<TutorialStep> steps;
};

const std::vector<Lesson>& lessons();

} // namespace linalg
