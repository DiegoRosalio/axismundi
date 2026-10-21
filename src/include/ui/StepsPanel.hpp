#pragma once

#include "math/Explain.hpp"
#include "math/Complex.hpp"
#include <raylib.h>

namespace linalg {

// Content of the "Calculation steps" panel: a topic picker and the worked
// calculation for the current inputs, updated live.
class StepsPanel {
public:
    // Returns the content height in pixels (the input: Session::stepsInput)
    float drawContent(Rectangle area, const StepsInput& input);

private:
    int m_matrixTopic = kDefaultMatrixTopic;
    int m_complexTopic = 0;  // product
};

// Wrapping row of selectable chips; returns the height used (pixels)
float chips(Rectangle area, const char* const* labels, int count, int& active);

} // namespace linalg
