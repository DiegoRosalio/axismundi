#include "rendering/FieldRenderer.hpp"
#include "gfx/Canvas3D.hpp"
#include "rendering/PlotRenderer.hpp"
#include "rendering/WorldMapping.hpp"
#include <raymath.h>
#include <algorithm>
#include <cmath>

namespace linalg {

namespace {

Vector3 world(const Vector3D& p) { return mathToWorld(p); }

} // namespace

bool FieldRenderer::rebuild(const FieldSpec& spec, const expr::Resolver* resolver) {
    m_spec = spec;
    m_meshes.release();
    m_lines.clear();
    m_error.clear();
    m_arrowCount = 0;
    m_minMagnitude = m_maxMagnitude = 0.0;

    expr::Program program;
    if (!program.compile(spec.formula, expr::kVarZ)) {
        m_error = program.error();
        return false;
    }
    field::FieldFunction f(program, resolver, spec.time, spec.plane);

    // Arrows: a shaft and a cone, from the grid point along the scaled vector
    field::ArrowSet set = field::sampleArrows(f, spec.box, spec.nx, spec.ny, spec.nz, spec.sameLength);
    m_arrowCount = set.arrows.size();
    m_minMagnitude = set.minMagnitude;
    m_maxMagnitude = set.maxMagnitude;
    PlotSpec heat;  // heat colors with the style's softening
    heat.heatSoften = spec.heatSoften;
    heat.heatSoftenTo = spec.heatSoftenTo;
    mesh::MeshData arrows;
    const float cell = static_cast<float>(set.cell);
    for (const field::Arrow& a : set.arrows) {
        float len = a.vec.magnitude();
        if (len < 1e-4f * cell) continue;
        float range = static_cast<float>(set.maxMagnitude - set.minMagnitude);
        float t = range > 1e-12f ? static_cast<float>((a.magnitude - set.minMagnitude) / range) : 1.0f;
        Color c = spec.heatmap ? PlotRenderer::heatColor(heat, t) : spec.color;
        float head = std::min(0.38f * len, 0.3f * cell);
        float headR = std::min(0.42f * head, (spec.plane ? 0.11f : 0.075f) * cell) * std::sqrt(spec.lineScale);
        float shaftR = std::min(0.035f * cell * spec.lineScale, headR * 0.45f);
        Vector3D dir = a.vec * (1.0f / len);
        Vector3D neck = a.base + dir * (len - head);
        if (len - head > 1e-4f) mesh::addTube(arrows, world(a.base), world(neck), shaftR, c, 5);
        mesh::addCone(arrows, world(neck), world(a.base + a.vec), headR, c, 7);
        // Several moderate meshes rather than one huge buffer
        if (arrows.count() > 60000) m_meshes.upload(arrows);
    }
    m_meshes.upload(arrows);

    // Flow lines (also needed for particles)
    if (spec.lines || spec.particles) {
        m_lines = field::fieldFlowLines(f, spec.box, spec.seeds);
        double span = std::max(spec.box.max.x - spec.box.min.x, spec.box.max.y - spec.box.min.y);
        double step = span / 160.0;  // arrowhead size along the lines
        if (spec.lines) {
            mesh::MeshData tubes;
            float r = 0.016f * spec.lineScale;
            for (const field::FlowLine& line : m_lines) {
                // Every other point is plenty for drawing
                size_t stride = line.points.size() > 200 ? 3 : 2;
                Vector3D prev = line.points.front();
                for (size_t i = stride; i < line.points.size() + stride - 1; i += stride) {
                    size_t j = std::min(i, line.points.size() - 1);
                    mesh::addTube(tubes, world(prev), world(line.points[j]), r, spec.lineColor, 4);
                    prev = line.points[j];
                }
                // An arrowhead halfway shows the direction
                size_t mid = line.points.size() / 2;
                if (mid + 1 < line.points.size()) {
                    Vector3D a = line.points[mid], b = line.points[mid + 1];
                    Vector3D d = b - a;
                    float dl = d.magnitude();
                    if (dl > 1e-6f) {
                        float h = static_cast<float>(step) * 3.0f * std::sqrt(spec.lineScale);
                        Vector3D tip = a + d * (h / dl);
                        mesh::addCone(tubes, world(a), world(tip), h * 0.45f, spec.lineColor, 7);
                    }
                }
                if (tubes.count() > 60000) m_meshes.upload(tubes);
            }
            m_meshes.upload(tubes);
        }
    }
    return true;
}

void FieldRenderer::draw(double time, float fade) const {
    m_meshes.draw(fade);
    if (!m_spec.particles || m_lines.empty()) return;
    Color c = m_spec.particleColor;
    c.a = static_cast<unsigned char>(c.a * std::clamp(fade, 0.0f, 1.0f));
    const int perLine = m_spec.plane ? 3 : 2;
    float r = 0.055f * std::sqrt(m_spec.lineScale);
    for (const field::FlowLine& line : m_lines) {
        for (int k = 0; k < perLine; k++) {
            double tau = field::particleTime(line, k, perLine, time, m_spec.speed);
            gfx::sphere(world(field::particleAt(line, tau)), r, 5, 6, c);
        }
    }
}

} // namespace linalg
