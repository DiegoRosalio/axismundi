#pragma once

#include "math/Matrix3D.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"
#include <raylib.h>
#include <vector>

namespace linalg {

// Represents a basis (set of 3 vectors) to visualize transformations
struct Basis {
    Vector3D origin;
    Vector3D xAxis;
    Vector3D yAxis;
    Vector3D zAxis;
    Color xColor, yColor, zColor;

    Basis() : origin(0, 0, 0),
              xAxis(1, 0, 0), yAxis(0, 1, 0), zAxis(0, 0, 1),
              xColor(RED), yColor(GREEN), zColor(BLUE) {}
};

class TransformVisualizer {
public:
    TransformVisualizer();

    // Set the transformation matrix
    void setTransform3D(const Matrix3D& mat);
    void setTransform4D(const Matrix4D& mat);

    // Draw original and transformed bases
    void draw() const;

    // Show/hide transformed basis
    void setShowTransformed(bool show) { m_showTransformed = show; }
    bool getShowTransformed() const { return m_showTransformed; }

    // Get current matrices
    const Matrix3D& getMatrix3D() const { return m_matrix3D; }
    const Matrix4D& getMatrix4D() const { return m_matrix4D; }

private:
    void drawBasis(const Basis& basis, float alpha = 1.0f) const;

    Matrix3D m_matrix3D;
    Matrix4D m_matrix4D;
    bool m_showTransformed;
    bool m_use4D;
};

} // namespace linalg
