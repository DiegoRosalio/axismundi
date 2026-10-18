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

    // Arrow from start to end: a shaft plus a cone head. radius and headSize are world sizes, but the shaft stays
    // between about 1 and 3.5 pixels wide on screen and the head is at most 35% of the length (and ~18 px), so
    // zooming in never makes arrows fat and short arrows aren't all head.
    static void drawArrow(const Vector3D& start, const Vector3D& end, Color color,
                          float radius = 0.02f, float headSize = 0.2f);

    // The look of vectors (and of a matrix's basis): a ~2 px shaft, a head about 30% of the length up to ~16 px,
    // and, when selected, a slightly thicker shaft with a narrow faint outline. lineScale: the view style's.
    static void drawVectorArrow(Vector3 from, Vector3 to, Color color, bool selected, float lineScale = 1.0f);

    // Where the camera is and how big a pixel is at distance 1 (2·tan(fovy/2) / screen height). Set once per
    // frame (SceneView::draw3D); before that, pixel sizes fall back to fixed world sizes.
    static void setScreenScale(Vector3 cameraPosition, float worldPerPixelAtOne);
    // The world length that covers `pixels` on screen at this point
    static float pixelsAt(Vector3 point, float pixels);

    std::vector<VectorObject>& getVectors() { return m_vectors; }
    const std::vector<VectorObject>& getVectors() const { return m_vectors; }

private:
    std::vector<VectorObject> m_vectors;
};

} // namespace linalg
