#include "gtk/LessonCard.hpp"
#include <cstring>

namespace linalg::gtk {

// The lessons (edu/Tutorial.cpp) were written for the raylib app: its top bar, floating panels and one-key
// shortcuts. Here are the steps whose words name those controls, as they read in the GTK app. When the raylib
// app goes away, these become the lessons' own texts.
const LessonText* gtkLessonText(const char* stepTitle) {
    struct Entry {
        const char* title;
        LessonText text;
    };
    static const Entry entries[] = {
        {"Welcome",
         {"axismundi turns linear algebra, complex numbers and functions into 3D pictures. The sidebar lists the "
          "objects in the scene and, under the list, edits the selected one. The pane on the right (F9) explains "
          "the selection: its analysis, worked steps and the breakdown of a matrix.",
          nullptr, nullptr}},
        {"Move the camera",
         {"Drag the empty view to turn around the scene, right-drag (or Shift+drag) to slide it, and use the wheel "
          "to zoom; a click without dragging still selects. The eye menu in the header bar has camera views and "
          "Focus on the selection.",
          nullptr, "Press the left button on an empty part of the view and move the mouse."}},
        {"Two spaces",
         {"The switch at the top of the window moves between the 3D space (vectors, matrices, plots) and the "
          "complex plane. The gallery has a ready-made example for each idea.",
          nullptr, "Click the grid button above the objects list."}},
        {"The objects list",
         {"Everything you see is an object: the matrix M, the vectors a and b, and vectors computed from them such "
          "as s = a + b. Click a row to select it; the inspector under the list shows its settings.",
          nullptr, nullptr}},
        {"Add a vector",
         {"The + button above the list has every kind of object, plus templates built from the selection.", nullptr,
          "Click + (or press Ctrl+Shift+A) and choose Vector."}},
        {"The dot product",
         {"a · b multiplies matching components and adds them up; it is zero exactly when the vectors are "
          "perpendicular. Select a and b together (Ctrl+click) to see it in the Analysis pane (F9).",
          nullptr, nullptr}},
        {"A rotation", {nullptr, nullptr, "Identity resets the matrix; Rotate 45° then turns it."}},
        {"Presets add up",
         {nullptr, nullptr, "Click Rotate 45° again. Scale 2× or Reflect now would act on the turned matrix."}},
        {"Flip it", {nullptr, nullptr, "The Reflect preset does it, or put −1 in one diagonal cell."}},
        {"Animate with the clock",
         {"Set to Animate, a parameter follows the scene clock from min to max. Ping-pong runs back and forth, and "
          "the easing curve shapes the motion (Ease in-out starts and stops gently, Bounce bounces at the end). The "
          "clock under the view plays, pauses and scrubs the time for everything at once.",
          "Switch theta to Animate, choose Ping-pong and an easing, then press Play under the view (or Space in "
          "the view).",
          nullptr}},
        {"A formula of time",
         {"A parameter can also be a formula of t, the scene time in seconds. pi/4*sin(2t) swings back and forth "
          "between ±45°; t/2 grows as long as the clock runs. t works in every formula too: rotz(t), "
          "[cos(t), sin(t), 1], sin(x - t). Drag the time slider under the view to see any moment.",
          nullptr, nullptr}},
        {"Watch the steps",
         {nullptr, nullptr, "Open the pane on the right (the button at the top right, or F9) and pick Breakdown."}},
        {"Order matters",
         {"Turning about z then x is not the same as x then z. The Analysis pane compares q·p and p·q.", nullptr,
          nullptr}},
        {"Slide along", {nullptr, nullptr, "Type mod(t, 4pi) in \"At u\", then press Play under the view."}},
        {"Draw with circles",
         {nullptr, nullptr,
          "Switch to the complex plane at the top (Alt+2), then + → Fourier epicycles. Press Play under the view."}},
    };
    for (const Entry& e : entries)
        if (std::strcmp(e.title, stepTitle) == 0) return &e.text;
    return nullptr;
}

} // namespace linalg::gtk
