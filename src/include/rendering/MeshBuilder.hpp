#pragma once

#include <raylib.h>
#include <vector>

namespace linalg::mesh {

// Triangles collected on the CPU, then uploaded as one GPU mesh (vertex colors)
struct MeshData {
    std::vector<float> positions;
    std::vector<unsigned char> colors;
    std::vector<unsigned short> indices;  // empty: plain triangle list
    void add(Vector3 p, Color c);
    size_t count() const { return positions.size() / 3; }
};

// A color darkened/brightened by a shading factor (0..1.2)
Color shaded(Color c, float shade);
// Light from above and slightly in front, in world space (y up)
Vector3 lightDir();

// A shaded tube (prism) from a to b
void addTube(MeshData& mesh, Vector3 a, Vector3 b, float radius, Color color, int sides = 6);
// A shaded cone from a base disc to a tip (arrowheads)
void addCone(MeshData& mesh, Vector3 base, Vector3 tip, float radius, Color color, int sides = 8);

// GPU meshes owned together: uploaded from MeshData, drawn with vertex colors, freed on release
class MeshSet {
public:
    MeshSet() = default;
    ~MeshSet() { release(); }
    MeshSet(const MeshSet&) = delete;
    MeshSet& operator=(const MeshSet&) = delete;

    // Upload (when it has at least one triangle) and clear the CPU data
    void upload(MeshData& data);
    void release();
    // Both faces, `fade` < 1 translucent
    void draw(float fade = 1.0f) const;
    size_t vertexCount() const;
    bool empty() const { return m_meshes.empty(); }

private:
    std::vector<Mesh> m_meshes;
};

} // namespace linalg::mesh
