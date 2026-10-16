#pragma once

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

    std::vector<VectorObject>& getVectors() { return m_vectors; }
    const std::vector<VectorObject>& getVectors() const { return m_vectors; }

private:
    void drawArrow(const Vector3D& start, const Vector3D& end, Color color) const;

    std::vector<VectorObject> m_vectors;
    float m_arrowHeadSize;
};

} // namespace linalg
