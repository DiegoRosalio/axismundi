#include "gfx/RaylibCanvas.hpp"
#include "rendering/GpuContext.hpp"
#include <raymath.h>
#include <rlgl.h>
#include <cstring>

namespace linalg::gfx {

void RaylibCanvas::line(Vector3 a, Vector3 b, Color c) { DrawLine3D(a, b, c); }
void RaylibCanvas::cylinder(Vector3 a, Vector3 b, float r1, float r2, int sides, Color c) {
    DrawCylinderEx(a, b, r1, r2, sides, c);
}
void RaylibCanvas::sphere(Vector3 center, float radius, int rings, int slices, Color c) {
    DrawSphereEx(center, radius, rings, slices, c);
}
void RaylibCanvas::triangle(Vector3 a, Vector3 b, Vector3 c, Color color) { DrawTriangle3D(a, b, c, color); }
void RaylibCanvas::cube(Vector3 center, Vector3 size, Color c) { DrawCubeV(center, size, c); }
void RaylibCanvas::cubeWires(Vector3 center, Vector3 size, Color c) { DrawCubeWiresV(center, size, c); }

void RaylibCanvas::begin(Primitive p) {
    rlBegin(p == Primitive::Lines ? RL_LINES : p == Primitive::Triangles ? RL_TRIANGLES : RL_QUADS);
}
void RaylibCanvas::color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) { rlColor4ub(r, g, b, a); }
void RaylibCanvas::vertex(float x, float y, float z) { rlVertex3f(x, y, z); }
void RaylibCanvas::end() { rlEnd(); }

void RaylibCanvas::flush() { rlDrawRenderBatchActive(); }
void RaylibCanvas::setBackfaceCulling(bool on) {
    if (on) rlEnableBackfaceCulling();
    else rlDisableBackfaceCulling();
}
void RaylibCanvas::setDepthWrite(bool on) {
    if (on) rlEnableDepthMask();
    else rlDisableDepthMask();
}

MeshId RaylibCanvas::uploadMesh(const float* positions, const unsigned char* colors, int vertexCount,
                                const unsigned short* indices, int indexCount) {
    Mesh mesh = {};
    mesh.vertexCount = vertexCount;
    mesh.triangleCount = indexCount > 0 ? indexCount / 3 : vertexCount / 3;
    mesh.vertices = static_cast<float*>(MemAlloc(static_cast<unsigned>(vertexCount * 3 * sizeof(float))));
    mesh.colors = static_cast<unsigned char*>(MemAlloc(static_cast<unsigned>(vertexCount * 4)));
    std::memcpy(mesh.vertices, positions, static_cast<size_t>(vertexCount) * 3 * sizeof(float));
    std::memcpy(mesh.colors, colors, static_cast<size_t>(vertexCount) * 4);
    if (indexCount > 0) {
        mesh.indices = static_cast<unsigned short*>(MemAlloc(static_cast<unsigned>(indexCount * sizeof(unsigned short))));
        std::memcpy(mesh.indices, indices, static_cast<size_t>(indexCount) * sizeof(unsigned short));
    }
    UploadMesh(&mesh, false);
    MeshId id = m_nextMesh++;
    m_meshes[id] = mesh;
    return id;
}

void RaylibCanvas::drawMesh(MeshId id, unsigned char alpha) {
    auto it = m_meshes.find(id);
    if (it == m_meshes.end()) return;
    static Material material = LoadMaterialDefault();  // vertex colors, white texture
    material.maps[MATERIAL_MAP_DIFFUSE].color = {255, 255, 255, alpha};
    DrawMesh(it->second, material, MatrixIdentity());
    material.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
}

void RaylibCanvas::freeMesh(MeshId id) {
    auto it = m_meshes.find(id);
    if (it == m_meshes.end()) return;
    Mesh& mesh = it->second;
    if (gpuContextReady()) {
        UnloadMesh(mesh);
    } else {
        // The GL context is gone (app shutdown): only free the CPU copies
        MemFree(mesh.vertices);
        MemFree(mesh.colors);
        MemFree(mesh.indices);
        MemFree(mesh.vboId);
    }
    m_meshes.erase(it);
}

ShaderId RaylibCanvas::loadShader(const char* vertex, const char* fragment) {
    Shader shader = LoadShaderFromMemory(vertex, fragment);
    if (!IsShaderValid(shader) || shader.id == rlGetShaderIdDefault()) return 0;  // raylib's fallback on errors
    m_shaders[shader.id] = shader;
    return shader.id;
}

int RaylibCanvas::uniformLocation(ShaderId id, const char* name) {
    auto it = m_shaders.find(id);
    return it == m_shaders.end() ? -1 : GetShaderLocation(it->second, name);
}

void RaylibCanvas::setUniform(ShaderId id, int location, const void* value, UniformType type) {
    auto it = m_shaders.find(id);
    if (it == m_shaders.end()) return;
    int t = type == UniformType::Float ? SHADER_UNIFORM_FLOAT
            : type == UniformType::Int ? SHADER_UNIFORM_INT
            : type == UniformType::Vec3 ? SHADER_UNIFORM_VEC3
                                        : SHADER_UNIFORM_VEC4;
    SetShaderValue(it->second, location, value, t);
}

void RaylibCanvas::beginShader(ShaderId id) {
    auto it = m_shaders.find(id);
    if (it != m_shaders.end()) BeginShaderMode(it->second);
}

void RaylibCanvas::endShader() { EndShaderMode(); }

void RaylibCanvas::unloadShader(ShaderId id) {
    auto it = m_shaders.find(id);
    if (it == m_shaders.end()) return;
    if (gpuContextReady()) UnloadShader(it->second);
    m_shaders.erase(it);
}

} // namespace linalg::gfx
