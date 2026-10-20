#pragma once

// Canvas3D: every drawing operation the 3D view needs, behind one interface, so the code in view/ and
// rendering/ never calls a graphics library directly. RaylibCanvas implements it with raylib's GL layer
// today; a plain-OpenGL implementation can replace raylib later without touching the drawing code
// (docs/gtk-port.md). The value types (Vector3, Color, Matrix...) are raylib's plain structs: they come with
// its headers and need nothing from the library itself.
//
// The free functions below (gfx::line, gfx::cylinder...) draw on the current canvas, which is RaylibCanvas
// unless a front end sets another with setCanvas.

#include <raylib.h>

namespace linalg::gfx {

enum class Primitive { Lines, Triangles, Quads };
enum class UniformType { Float, Int, Vec3, Vec4 };

using MeshId = unsigned;    // 0 = none
using ShaderId = unsigned;  // 0 = none / failed

class Canvas3D {
public:
    virtual ~Canvas3D() = default;

    // Shapes (world coordinates, inside the 3D camera)
    virtual void line(Vector3 a, Vector3 b, Color color) = 0;
    // A tube from a to b, radius r1 at a and r2 at b (r2 = 0: a cone), `sides` around
    virtual void cylinder(Vector3 a, Vector3 b, float r1, float r2, int sides, Color color) = 0;
    virtual void sphere(Vector3 center, float radius, int rings, int slices, Color color) = 0;
    virtual void triangle(Vector3 a, Vector3 b, Vector3 c, Color color) = 0;  // counter-clockwise = front
    virtual void cube(Vector3 center, Vector3 size, Color color) = 0;
    virtual void cubeWires(Vector3 center, Vector3 size, Color color) = 0;

    // Immediate mode: begin, then color/vertex pairs, then end
    virtual void begin(Primitive primitive) = 0;
    virtual void color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) = 0;
    virtual void vertex(float x, float y, float z) = 0;
    virtual void end() = 0;

    // State. flush draws what is batched so far (before changing state the batch doesn't record)
    virtual void flush() = 0;
    virtual void setBackfaceCulling(bool on) = 0;
    virtual void setDepthWrite(bool on) = 0;

    // GPU meshes: positions xyz, colors rgba (one per vertex), optional 16-bit indices
    virtual MeshId uploadMesh(const float* positions, const unsigned char* colors, int vertexCount,
                              const unsigned short* indices, int indexCount) = 0;
    // Drawn with vertex colors times (255, 255, 255, alpha)
    virtual void drawMesh(MeshId mesh, unsigned char alpha) = 0;
    // Frees the GPU copy while the GL context exists, otherwise only the CPU side (app shutdown)
    virtual void freeMesh(MeshId mesh) = 0;

    // Shaders (GLSL 330 source)
    virtual ShaderId loadShader(const char* vertex, const char* fragment) = 0;
    virtual int uniformLocation(ShaderId shader, const char* name) = 0;
    virtual void setUniform(ShaderId shader, int location, const void* value, UniformType type) = 0;
    virtual void beginShader(ShaderId shader) = 0;  // flushes what was drawn before
    virtual void endShader() = 0;                    // draws what was batched with the shader
    virtual void unloadShader(ShaderId shader) = 0;
};

Canvas3D& canvas();
void setCanvas(Canvas3D* canvas);  // nullptr: back to RaylibCanvas

// Shortcuts on the current canvas
inline void line(Vector3 a, Vector3 b, Color c) { canvas().line(a, b, c); }
inline void cylinder(Vector3 a, Vector3 b, float r1, float r2, int sides, Color c) {
    canvas().cylinder(a, b, r1, r2, sides, c);
}
inline void sphere(Vector3 center, float radius, Color c) { canvas().sphere(center, radius, 16, 16, c); }
inline void sphere(Vector3 center, float radius, int rings, int slices, Color c) {
    canvas().sphere(center, radius, rings, slices, c);
}
inline void triangle(Vector3 a, Vector3 b, Vector3 c, Color color) { canvas().triangle(a, b, c, color); }
inline void cube(Vector3 center, Vector3 size, Color c) { canvas().cube(center, size, c); }
inline void cubeWires(Vector3 center, Vector3 size, Color c) { canvas().cubeWires(center, size, c); }
inline void begin(Primitive p) { canvas().begin(p); }
inline void color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) { canvas().color(r, g, b, a); }
inline void vertex(float x, float y, float z) { canvas().vertex(x, y, z); }
inline void end() { canvas().end(); }
inline void flush() { canvas().flush(); }
inline void setBackfaceCulling(bool on) { canvas().setBackfaceCulling(on); }
inline void setDepthWrite(bool on) { canvas().setDepthWrite(on); }

} // namespace linalg::gfx
