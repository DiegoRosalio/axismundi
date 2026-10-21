#pragma once

#include "gfx/Canvas3D.hpp"
#include "math/GridMath.hpp"
#include <raylib.h>

// The endless, smooth grid: one large square per plane, colored by a GPU shader (a small GLSL program
// that runs once per pixel). The shader draws thin anti-aliased lines at the adaptive spacing (see
// math/GridMath), fades the finest lines in and out as you zoom, fades everything with distance, and can
// draw a polar grid (circles and rays). If the GPU can't compile it, valid() is false and App falls back to
// the line grid (rendering/Grid).
namespace linalg {

// Which world plane to draw (raylib's world is y-up; math z-up — see WorldMapping.hpp)
enum class GridPlane {
    WorldXZ,  // the 3D floor: math xy
    WorldXY,  // the complex plane, or the math xz wall in 3D
    WorldYZ,  // the math yz wall in 3D
};

struct GridSpec {
    Color color{0, 0, 0, 0};       // the view style's grid color (alpha: the minor lines' strength)
    Vector3 center{0, 0, 0};       // world point under the view (the square is centered here)
    float radius = 20.0f;          // the grid fades out toward this distance from the center
    grid::Level level;             // spacing, fade, ratio (math/GridMath.hpp)
    bool polar = false;            // circles and rays instead of squares
};

class GridShader {
public:
    GridShader() = default;
    ~GridShader();
    // Free the shader now (the GL context is about to go); the next valid() loads it again
    void release();
    GridShader(const GridShader&) = delete;             // owns a GPU resource: not copyable
    GridShader& operator=(const GridShader&) = delete;

    // Loads the shader the first time (needs the window); false when it can't be used
    bool valid();
    // Inside BeginMode3D. Doesn't write depth, so objects on the floor never flicker against it.
    void draw(GridPlane plane, const GridSpec& spec);

private:
    gfx::ShaderId m_shader = 0;
    bool m_tried = false;
    bool m_ok = false;
    int m_locColor = -1, m_locCenter = -1, m_locRadius = -1, m_locSpacing = -1, m_locFade = -1, m_locRatio = -1,
        m_locPlane = -1, m_locPolar = -1;
};

} // namespace linalg
