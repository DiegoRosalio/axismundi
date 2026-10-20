#pragma once

// Canvas3D drawn with raylib (rlgl, rmodels): the only place view/ and rendering/ drawing reaches raylib.

#include "gfx/Canvas3D.hpp"
#include <map>

namespace linalg::gfx {

class RaylibCanvas : public Canvas3D {
public:
    void line(Vector3 a, Vector3 b, Color color) override;
    void cylinder(Vector3 a, Vector3 b, float r1, float r2, int sides, Color color) override;
    void sphere(Vector3 center, float radius, int rings, int slices, Color color) override;
    void triangle(Vector3 a, Vector3 b, Vector3 c, Color color) override;
    void cube(Vector3 center, Vector3 size, Color color) override;
    void cubeWires(Vector3 center, Vector3 size, Color color) override;

    void begin(Primitive primitive) override;
    void color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) override;
    void vertex(float x, float y, float z) override;
    void end() override;

    void flush() override;
    void setBackfaceCulling(bool on) override;
    void setDepthWrite(bool on) override;

    MeshId uploadMesh(const float* positions, const unsigned char* colors, int vertexCount,
                      const unsigned short* indices, int indexCount) override;
    void drawMesh(MeshId mesh, unsigned char alpha) override;
    void freeMesh(MeshId mesh) override;

    ShaderId loadShader(const char* vertex, const char* fragment) override;
    int uniformLocation(ShaderId shader, const char* name) override;
    void setUniform(ShaderId shader, int location, const void* value, UniformType type) override;
    void beginShader(ShaderId shader) override;
    void endShader() override;
    void unloadShader(ShaderId shader) override;

private:
    std::map<MeshId, Mesh> m_meshes;
    std::map<ShaderId, Shader> m_shaders;
    MeshId m_nextMesh = 1;
};

} // namespace linalg::gfx
