#pragma once

#include "math/Explain.hpp"
#include "math/Complex.hpp"
#include "ui/AppMode.hpp"
#include <raylib.h>

namespace linalg {

// Everything the worked calculations need
struct StepsInput {
    AppMode mode;
    Matrix4D matrix;
    bool is4x4;
    Vector3D a, b;
    Complex z1, z2;
    float theta;
};

// Content of the "Calculation steps" panel: a topic picker and the worked
// calculation for the current inputs, updated live.
class StepsPanel {
public:
    // Returns the content height in pixels
    float drawContent(Rectangle area, const StepsInput& input);

private:
    int m_matrixTopic = 3;   // determinant
    int m_complexTopic = 0;  // product
};

// Wrapping row of selectable chips; returns the height used (pixels)
float chips(Rectangle area, const char* const* labels, int count, int& active);

} // namespace linalg
