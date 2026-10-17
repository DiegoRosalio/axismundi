#include "rendering/Grid.hpp"
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <algorithm>
#include <cmath>
#include "ui/core/Theme.hpp"

namespace linalg {

Grid::Grid(int size, float spacing)
    : m_size(size), m_spacing(spacing),
      m_gridColor({200, 200, 200, 100}),
      m_showGrid(true), m_showAxes(true) {}

namespace {

// Lines every unit, stronger every 5 units, fading toward the edge.
// toWorld maps grid coordinates (u, v) to a world point.
template <typename ToWorld>
void drawFadingGrid(int size, float spacing, ToWorld toWorld, bool paper) {
    const Color base = paper ? Color{120, 120, 120, 70} : ui::theme().grid;
    const float extent = size * spacing;
    // Minor lines fade out sooner, so a large grid shows only major lines far away
    const float minorReach = std::min(extent, 25.0f * spacing);
    const int pieces = std::clamp(size * 2, 8, 80);  // segments per line (for the fade)

    // Full strength near the middle, smooth fade over the outer part
    auto fade = [](float u) {
        float t = std::clamp((u - 0.55f) / 0.45f, 0.0f, 1.0f);
        return 1.0f - t * t * (3.0f - 2.0f * t);
    };
    auto alphaAt = [&](float u, float v, bool major) {
        float r = std::sqrt(u * u + v * v);
        float a = major ? std::min(255.0f, base.a * 1.7f) * fade(r / extent) : base.a * fade(r / minorReach);
        return static_cast<unsigned char>(std::clamp(a, 0.0f, 255.0f));
    };

    rlBegin(RL_LINES);
    for (int i = -size; i <= size; i++) {
        bool major = (i % 5) == 0;
        float c = i * spacing;
        if (!major && std::fabs(c) > minorReach) continue;
        float reach = major ? extent : std::sqrt(std::max(0.0f, minorReach * minorReach - c * c));
        for (int dir = 0; dir < 2; dir++) {
            for (int k = 0; k < pieces; k++) {
                float s0 = -reach + 2.0f * reach * k / pieces, s1 = -reach + 2.0f * reach * (k + 1) / pieces;
                float u0 = dir ? c : s0, v0 = dir ? s0 : c, u1 = dir ? c : s1, v1 = dir ? s1 : c;
                unsigned char a0 = alphaAt(u0, v0, major), a1 = alphaAt(u1, v1, major);
                if (a0 == 0 && a1 == 0) continue;
                Vector3 p0 = toWorld(u0, v0), p1 = toWorld(u1, v1);
                rlColor4ub(base.r, base.g, base.b, a0);
                rlVertex3f(p0.x, p0.y, p0.z);
                rlColor4ub(base.r, base.g, base.b, a1);
                rlVertex3f(p1.x, p1.y, p1.z);
            }
        }
    }
    rlEnd();
}

} // namespace

void Grid::draw() const {
    if (!m_showGrid) return;
    // The floor: world XZ (the math xy-plane)
    drawFadingGrid(m_size, m_spacing, [](float u, float v) { return Vector3{u, 0.0f, v}; }, m_paper);
}

void Grid::drawPlaneXY() const {
    if (!m_showGrid) return;
    drawFadingGrid(m_size, m_spacing, [](float u, float v) { return Vector3{u, v, 0.0f}; }, m_paper);
}

void Grid::drawAxes(float axisLength) const {
    if (!m_showAxes) return;

    const ui::Theme& t = ui::theme();
    DrawCylinderEx({0, 0, 0}, {axisLength, 0, 0}, 0.02f, 0.02f, 8, t.axisX);
    DrawCylinderEx({0, 0, 0}, {0, axisLength, 0}, 0.02f, 0.02f, 8, t.axisY);
    DrawCylinderEx({0, 0, 0}, {0, 0, axisLength}, 0.02f, 0.02f, 8, t.axisZ);
    DrawSphere({axisLength, 0, 0}, 0.08f, t.axisX);
    DrawSphere({0, axisLength, 0}, 0.08f, t.axisY);
    DrawSphere({0, 0, axisLength}, 0.08f, t.axisZ);
}

} // namespace linalg
