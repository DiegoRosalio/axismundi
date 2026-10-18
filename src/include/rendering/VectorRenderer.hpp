#pragma once

// Draws arrows (a cylinder shaft plus a cone head) between two world points: vectors, frames, gradients...

#include "math/Vector3D.hpp"
#include <raylib.h>
#include <vector>
#include <string>

namespace linalg {

struct VectorObject {
    Vector3D position;    // Starting point
    Vector3D direction;   // The vector itself
    Color color;
    std::string label;
    bool showLabel;

    VectorObject(const Vector3D& pos, const Vector3D& dir, Color col = YELLOW,
                 const std::string& lbl = "", bool showLbl = true)
        : position(pos), direction(dir), color(col), label(lbl), showLabel(showLbl) {}
};

class VectorRenderer {
public:
    VectorRenderer();

    void addVector(const VectorObject& vec);
    void clearVectors();
    void draw() const;

    // Name and magnitude at each arrow tip (call after EndMode3D())
    void drawLabels(const Camera3D& camera) const;

    // Arrow from start to end: a shaft plus a cone head
    static void drawArrow(const Vector3D& start, const Vector3D& end, Color color,
                          float radius = 0.02f, float headSize = 0.2f);

    std::vector<VectorObject>& getVectors() { return m_vectors; }
    const std::vector<VectorObject>& getVectors() const { return m_vectors; }

private:
    std::vector<VectorObject> m_vectors;
};

} // namespace linalg
