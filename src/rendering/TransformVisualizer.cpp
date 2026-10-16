#include "rendering/TransformVisualizer.hpp"
#include <raylib.h>
#include <raymath.h>

namespace linalg {

TransformVisualizer::TransformVisualizer()
    : m_matrix3D(Matrix3D::identity()),
      m_matrix4D(Matrix4D::identity()),
      m_showTransformed(true),
      m_use4D(false) {}

void TransformVisualizer::setTransform3D(const Matrix3D& mat) {
    m_matrix3D = mat;
    m_use4D = false;
}

void TransformVisualizer::setTransform4D(const Matrix4D& mat) {
    m_matrix4D = mat;
    m_use4D = true;
}

void TransformVisualizer::draw() const {
    // Draw original basis (semi-transparent)
    Basis original;
    drawBasis(original, 0.5f);

    if (!m_showTransformed) return;

    // Draw transformed basis
    Basis transformed;

    if (m_use4D) {
        transformed.xAxis = m_matrix4D.transformDirection(Vector3D(1, 0, 0));
        transformed.yAxis = m_matrix4D.transformDirection(Vector3D(0, 1, 0));
        transformed.zAxis = m_matrix4D.transformDirection(Vector3D(0, 0, 1));
        transformed.origin = m_matrix4D.transform(Vector3D(0, 0, 0));
    } else {
        transformed.xAxis = m_matrix3D.transform(Vector3D(1, 0, 0));
        transformed.yAxis = m_matrix3D.transform(Vector3D(0, 1, 0));
        transformed.zAxis = m_matrix3D.transform(Vector3D(0, 0, 1));
    }

    // Use different colors for transformed basis
    transformed.xColor = ColorAlpha(ORANGE, 1.0f);
    transformed.yColor = ColorAlpha(LIME, 1.0f);
    transformed.zColor = ColorAlpha(SKYBLUE, 1.0f);

    drawBasis(transformed, 1.0f);
}

void TransformVisualizer::drawBasis(const Basis& basis, float alpha) const {
    Color xCol = ColorAlpha(basis.xColor, alpha);
    Color yCol = ColorAlpha(basis.yColor, alpha);
    Color zCol = ColorAlpha(basis.zColor, alpha);

    Vector3 origin = {basis.origin.x, basis.origin.y, basis.origin.z};

    // Draw X axis
    Vector3D xEnd = basis.origin + basis.xAxis;
    Vector3 xEndVec = {xEnd.x, xEnd.y, xEnd.z};
    DrawCylinderEx(origin, xEndVec, 0.03f, 0.03f, 8, xCol);
    DrawSphere(xEndVec, 0.1f, xCol);

    // Draw Y axis
    Vector3D yEnd = basis.origin + basis.yAxis;
    Vector3 yEndVec = {yEnd.x, yEnd.y, yEnd.z};
    DrawCylinderEx(origin, yEndVec, 0.03f, 0.03f, 8, yCol);
    DrawSphere(yEndVec, 0.1f, yCol);

    // Draw Z axis
    Vector3D zEnd = basis.origin + basis.zAxis;
    Vector3 zEndVec = {zEnd.x, zEnd.y, zEnd.z};
    DrawCylinderEx(origin, zEndVec, 0.03f, 0.03f, 8, zCol);
    DrawSphere(zEndVec, 0.1f, zCol);

    // Draw origin sphere
    DrawSphere(origin, 0.08f, ColorAlpha(WHITE, alpha));
}

} // namespace linalg
