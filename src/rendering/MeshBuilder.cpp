#include "rendering/MeshBuilder.hpp"
#include "rendering/GpuContext.hpp"
#include <raymath.h>
#include <rlgl.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace linalg::mesh {

namespace {

Material& vertexColorMaterial() {
    static Material material = LoadMaterialDefault();  // vertex colors, white texture
    return material;
}

// Two unit vectors perpendicular to d (and to each other)
void basisAround(Vector3 d, Vector3& u, Vector3& v) {
    Vector3 up = std::fabs(d.y) < 0.9f ? Vector3{0, 1, 0} : Vector3{1, 0, 0};
    u = Vector3Normalize(Vector3CrossProduct(d, up));
    v = Vector3CrossProduct(d, u);
}

} // namespace

void MeshData::add(Vector3 p, Color c) {
    positions.insert(positions.end(), {p.x, p.y, p.z});
    colors.insert(colors.end(), {c.r, c.g, c.b, c.a});
}

Color shaded(Color c, float shade) {
    shade = std::clamp(shade, 0.0f, 1.2f);
    auto ch = [&](unsigned char v) { return static_cast<unsigned char>(std::min(255.0f, v * shade)); };
    return {ch(c.r), ch(c.g), ch(c.b), c.a};
}

Vector3 lightDir() {
    return Vector3Normalize({0.35f, 0.85f, 0.4f});
}

void addTube(MeshData& mesh, Vector3 a, Vector3 b, float radius, Color color, int sides) {
    Vector3 d = Vector3Subtract(b, a);
    float len = Vector3Length(d);
    if (len < 1e-6f) return;
    d = Vector3Scale(d, 1.0f / len);
    Vector3 u, v;
    basisAround(d, u, v);
    Vector3 light = lightDir();
    for (int k = 0; k < sides; k++) {
        float a0 = 2.0f * PI * k / sides, a1 = 2.0f * PI * (k + 1) / sides;
        Vector3 n0 = Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0)));
        Vector3 n1 = Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1)));
        Vector3 nMid = Vector3Normalize(Vector3Add(n0, n1));
        Color c = shaded(color, 0.78f + 0.3f * Vector3DotProduct(nMid, light));
        Vector3 p0 = Vector3Add(a, Vector3Scale(n0, radius)), p1 = Vector3Add(a, Vector3Scale(n1, radius));
        Vector3 q0 = Vector3Add(b, Vector3Scale(n0, radius)), q1 = Vector3Add(b, Vector3Scale(n1, radius));
        mesh.add(p0, c);
        mesh.add(q0, c);
        mesh.add(q1, c);
        mesh.add(p0, c);
        mesh.add(q1, c);
        mesh.add(p1, c);
    }
}

void addCone(MeshData& mesh, Vector3 base, Vector3 tip, float radius, Color color, int sides) {
    Vector3 d = Vector3Subtract(tip, base);
    float len = Vector3Length(d);
    if (len < 1e-6f) return;
    d = Vector3Scale(d, 1.0f / len);
    Vector3 u, v;
    basisAround(d, u, v);
    Vector3 light = lightDir();
    for (int k = 0; k < sides; k++) {
        float a0 = 2.0f * PI * k / sides, a1 = 2.0f * PI * (k + 1) / sides;
        Vector3 n0 = Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0)));
        Vector3 n1 = Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1)));
        Vector3 nMid = Vector3Normalize(Vector3Add(Vector3Add(n0, n1), Vector3Scale(d, 0.6f)));
        Color side = shaded(color, 0.8f + 0.3f * Vector3DotProduct(nMid, light));
        Vector3 p0 = Vector3Add(base, Vector3Scale(n0, radius)), p1 = Vector3Add(base, Vector3Scale(n1, radius));
        mesh.add(p0, side);
        mesh.add(tip, side);
        mesh.add(p1, side);
        Color bottom = shaded(color, 0.6f);  // base disc
        mesh.add(base, bottom);
        mesh.add(p1, bottom);
        mesh.add(p0, bottom);
    }
}

void MeshSet::upload(MeshData& data) {
    if (data.count() < 3 || (!data.indices.empty() && data.indices.size() < 3)) {
        data = MeshData{};
        return;
    }
    Mesh mesh = {};
    mesh.vertexCount = static_cast<int>(data.count());
    mesh.triangleCount = static_cast<int>(data.indices.empty() ? data.count() / 3 : data.indices.size() / 3);
    mesh.vertices = static_cast<float*>(MemAlloc(static_cast<unsigned>(data.positions.size() * sizeof(float))));
    mesh.colors = static_cast<unsigned char*>(MemAlloc(static_cast<unsigned>(data.colors.size())));
    std::memcpy(mesh.vertices, data.positions.data(), data.positions.size() * sizeof(float));
    std::memcpy(mesh.colors, data.colors.data(), data.colors.size());
    if (!data.indices.empty()) {
        mesh.indices = static_cast<unsigned short*>(
            MemAlloc(static_cast<unsigned>(data.indices.size() * sizeof(unsigned short))));
        std::memcpy(mesh.indices, data.indices.data(), data.indices.size() * sizeof(unsigned short));
    }
    UploadMesh(&mesh, false);
    m_meshes.push_back(mesh);
    data = MeshData{};
}

void MeshSet::release() {
    for (Mesh& mesh : m_meshes) {
        if (gpuContextReady()) {
            UnloadMesh(mesh);
        } else {
            // The GL context is gone (app shutdown): only free the CPU copies
            MemFree(mesh.vertices);
            MemFree(mesh.colors);
            MemFree(mesh.indices);
            MemFree(mesh.vboId);
        }
    }
    m_meshes.clear();
}

void MeshSet::draw(float fade) const {
    if (m_meshes.empty()) return;
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();  // surfaces are visible from above and below
    Material& material = vertexColorMaterial();
    unsigned char alpha = static_cast<unsigned char>(255.0f * std::clamp(fade, 0.0f, 1.0f));
    material.maps[MATERIAL_MAP_DIFFUSE].color = {255, 255, 255, alpha};
    for (const Mesh& mesh : m_meshes) DrawMesh(mesh, material, MatrixIdentity());
    material.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    rlEnableBackfaceCulling();
}

size_t MeshSet::vertexCount() const {
    size_t n = 0;
    for (const Mesh& m : m_meshes) n += static_cast<size_t>(m.vertexCount);
    return n;
}

} // namespace linalg::mesh
