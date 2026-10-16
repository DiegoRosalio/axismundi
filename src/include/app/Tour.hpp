#pragma once

#include <functional>
#include <string>
#include <vector>

namespace linalg {

class App;

// Scripted walk through the UI that saves a PNG per step and then exits.
// Used with a virtual display (xvfb-run) to review the UI without a screen:
//   linalg_viz --tour out/ --size 1280x720
class Tour {
public:
    explicit Tour(std::string outputDir);

    // Called once per frame after the frame was drawn but before EndDrawing()
    void capture();
    // Called once per frame before update; runs step actions. Returns false when done.
    bool advance(App& app);

private:
    struct Step {
        std::string name;
        std::function<void(App&)> action;
        int frames;  // frames to wait before capturing
    };

    void buildSteps();

    std::string m_dir;
    std::vector<Step> m_steps;
    size_t m_index = 0;
    int m_framesLeft = -1;
    bool m_captureNow = false;
};

} // namespace linalg
